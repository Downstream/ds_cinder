#include "stdafx.h"

#include "tuio_input.h"

#include <cinder/tuio/Tuio.h>
#include <ds/app/engine/engine.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace {

// HACK OMG!
// Maintain a pool of deleted OscReceivers, because boost ASIO doesn't actually cancel the call to
// async_receive_from, even after the socket is closed and the cancelled.  TUIO receivers only are deleted
// at application exit, or manual restart, so this shouldn't cause a memory leak under normal conditions.
// Rather, by maintaining the deleted OscReceivers we will prevent a crash when restarting apps while they
// are actively receiving TUIO network traffic...
class CustomOscReceiverUdp;
typedef std::shared_ptr<ci::osc::ReceiverUdp> DeadReceiverRef;
static std::vector<DeadReceiverRef> sDeadReceiversPool;

/// Custom OSC receiver which will cancel the socket before closing
class CustomOscReceiverUdp : public ci::osc::ReceiverUdp {
public:
	CustomOscReceiverUdp(uint16_t port)
		: ci::osc::ReceiverUdp(port)
	{}

protected:
	virtual void closeImpl() override {
		asio::error_code ec;
		mSocket->cancel(ec);
		if (ec) {
			DS_LOG_WARNING("Error cancelling Osc UDP socket: " << ec.message());
		}
		ci::osc::ReceiverUdp::closeImpl();
	}
};

}


namespace ds {
namespace ui {

TuioInput::TuioInput(ds::ui::SpriteEngine& engine, const int port, const ci::vec2 scaley, const ci::vec2 possy,
					 const float rotty, const int fingerIdOffset, const ci::Rectf& allowedRect)
	: mEngine(engine)
	, mUdpPort(port)
	, mFingerIdOffset(fingerIdOffset)
	, mAllowedRect(allowedRect)
	, mTransform(
		  glm::translate(glm::vec3(possy, 0.0f)) 
		* glm::rotate(glm::radians(rotty), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::scale(glm::vec3(scaley, 1.0f)) )
{
}

void TuioInput::registerEvents() {
	const auto window = ci::app::getWindow();
	const auto transformCursor = [this, window](const ci::tuio::Cursor2d& tuioCursor) {
		const auto touch = tuioCursor.convertToTouch(window);
		const auto pos = transformEventPosition(touch.getPos(), true);
		const auto prevPos = transformEventPosition(touch.getPrevPos(), true);
		const ci::app::TouchEvent::Touch convertedTouch(pos, prevPos, tuioCursor.getSessionId() + mFingerIdOffset, touch.getTime(), (void *)touch.getNative());
		return ds::ui::TouchEvent(ci::app::getWindow(), { convertedTouch }, true);
	};

	const auto transformObject = [this](const ci::tuio::Object2d& o) {
		const auto pos = transformEventPosition(o.getPosition());
		// Technically, the velocity should be probably transformed too...?
		return ds::TuioObject(o.getClassId(), pos, o.getAngle(), o.getVelocity(), o.getRotationVelocity());
	};

	// Inject transformed touches, ignoring touch-add events that fall outside the allowed rectangle
	mTuioReceiver->setAddedFn   <ci::tuio::Cursor2d>( [=] (const auto& tuioCursor) {
		const ds::ui::TouchEvent& transformed = transformCursor(tuioCursor);
		const auto pos = transformed.getTouches()[0].getPos();
		if (mAllowedRect.calcArea() > 0.0f && !mAllowedRect.contains(pos)) {
			DS_LOG_VERBOSE(1, "TuioInput: discarding touch point at " << pos << " because it doesn't fit in the filter rect " << mAllowedRect);
		}
		else {
			mEngine.injectTouchesBegin(transformed);
		}
	});
	mTuioReceiver->setUpdatedFn<ci::tuio::Cursor2d>( [=] (const auto& tuioCursor) {
		mEngine.injectTouchesMoved(transformCursor(tuioCursor));
	});
	mTuioReceiver->setRemovedFn<ci::tuio::Cursor2d>( [=] (const auto& tuioCursor) {
		mEngine.injectTouchesEnded(transformCursor(tuioCursor));
	});

	// Get those TUIO Objects too
	mTuioReceiver->setAddedFn<ci::tuio::Object2d>(   [this, transformObject](const auto& tuioObject) {
		const auto transformed = transformObject(tuioObject);
		DS_LOG_INFO("TuioInput: TUIO OBJECT ADDED: pos=" << tuioObject.getPosition() 
			<< ", transformed=" << transformed.getPosition()
		);
		mEngine.injectObjectsBegin(transformed);
	});
	mTuioReceiver->setUpdatedFn<ci::tuio::Object2d>( [this, transformObject](const auto& tuioObject) {
		mEngine.injectObjectsMoved(transformObject(tuioObject));
	});
	mTuioReceiver->setRemovedFn<ci::tuio::Object2d>( [this, transformObject](const auto& tuioObject) {
		mEngine.injectObjectsEnded(transformObject(tuioObject));
	});
}

TuioInput::~TuioInput() {
	stop();
}

void TuioInput::stop() {
	if (mOscReceiver) {
		mOscReceiver->close();
		sDeadReceiversPool.push_back(mOscReceiver);
		mOscReceiver.reset();
	}

	if (mTuioReceiver) {
		mTuioReceiver->clear<ci::tuio::Cursor2d>();
		mTuioReceiver->clear<ci::tuio::Object2d>();
	}
}

void TuioInput::start(const bool _registerEvents, const int port) {
	stop();

	// Use initial port if no port parameter specified.. 
	if (port != 0)
		mUdpPort = port;

	// IMPORTANT! Need to delete the tuio::Receiver before reassigning/deleting the 
	// osc::ReceiverUdp, because the Tuio::Receiver's destructor requires 
	// the osc::Receiver to still exist when called...
	mTuioReceiver.reset();
	mOscReceiver.reset(new CustomOscReceiverUdp(mUdpPort));

	if (_registerEvents) {
		mTuioReceiver = std::make_shared<ci::tuio::Receiver>(mOscReceiver.get());
		// TuioInputs that are not the main input have their own transformations, and setup their own handlers to inject touches
		registerEvents();
	}

	else {
		// The main TuioInput can just use the default behavior of Cinder's TUIO receiver,
		// which is to call ci::App touchesBegan, touchesMoved, touchesEnded virtual methods, 
		// which DsCinder overrides in the ds::app::App class.  These callbacks are implicitly 
		// registered if you pass a Window ref to the Receiver constructor.
		mTuioReceiver = std::make_shared<ci::tuio::Receiver>(ci::app::getWindow(), mOscReceiver.get());
	}

	// Compute original offset, scale, rotation from transformation matrix
	ci::vec3 pos, scale, skew;
	ci::vec4 persp;
	ci::quat rot;
	glm::decompose(mTransform, scale, rot, pos, skew, persp);

	// Start listening on the given port
	try {
		DS_LOG_INFO("Starting TuioInput at port "
			<< mUdpPort << " with offset of " << ci::vec2(pos) << ", scale of " << ci::vec2(scale) << ", and a rotation of " << glm::degrees(glm::roll(rot))
			<< " degrees.  finger Id offset of " << mFingerIdOffset << " filter rect of " << mAllowedRect);
		mOscReceiver->bind();
	} catch(std::exception& e) {
		DS_LOG_WARNING("Couldn't connect a tuio input at port " << port << " exception: " << e.what());
	}

	mOscReceiver->listen(
		[]( asio::error_code ec, asio::ip::udp::endpoint ep ) -> bool {
			if (ec) {
				DS_LOG_VERBOSE(1,  "TuioInput Warning: Osc Receiver had error, most likely from resetting a listening UDP connection: " 
					<< ec.message() 
					<< " Error Value: " << ec.value()
				);
				return false;
			}
			else
				return true;
		} );
}

ci::vec2 TuioInput::transformEventPosition(const ci::vec2& pos, const bool doWindowScale) {
	// TUIO Touch events from cinder are scaled by the window size,
	// so we need to reverse this transformation to get world coordinates.
	const auto windowScale = doWindowScale ? mEngine.getDstRect().getSize() : ci::vec2(1.0f);
	return glm::vec2(mTransform * glm::vec4(pos / windowScale, 0.0f, 1.0f));
}

}  // !namespace ui
}  // !namespace ds
