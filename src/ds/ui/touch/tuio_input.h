#pragma once
#ifndef _DS_UI_TOUCH_TUIO_INPUT_H_
#define _DS_UI_TOUCH_TUIO_INPUT_H_

#include <ds/ui/sprite/sprite_engine.h>

namespace cinder {
namespace osc {
class ReceiverUdp;
}
namespace tuio {
class Receiver;
//typename Cursor2d;
}
}

namespace ds {
namespace ui {

/**
* \class TuioInput
* \brief Handles multiplexed TUIO input, with unique transformations for each input
*		 Note that this is in additional to the built-in touch mode
*/
class TuioInput {
public:
	TuioInput(ds::ui::SpriteEngine& engine, const int port, const ci::vec2 scale, const ci::vec2 offset,
			  const float rotty, const int fingerIdOffset, const ci::Rectf& allowedArea);
	~TuioInput();

	typedef std::shared_ptr<ci::osc::ReceiverUdp>
									OscReceiverRef;
	typedef std::shared_ptr<ci::tuio::Receiver>
									TuioReceiverRef;
	TuioReceiverRef					getReceiver() { return mTuioReceiver; }

	void							start(const bool registerEvents=false, const int port=0);
	void							stop();

protected:
	ci::vec2						transformEventPosition(const ci::vec2& pos, const bool doWindowScale = false);
	void							registerEvents();

private:
	ds::ui::SpriteEngine&			mEngine;

	int								mUdpPort;
	int								mFingerIdOffset;
	glm::mat4						mTransform;
	ci::Rectf						mAllowedRect;

	OscReceiverRef					mOscReceiver;
	TuioReceiverRef					mTuioReceiver;
};


} // ! namespace ui
}  // !namespace ds

#endif  // !_DS_UI_TOUCH_TUIO_INPUT_H_
