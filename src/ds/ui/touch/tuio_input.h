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

protected:
	ci::vec2 transformEventPosition(const ci::vec2& pos, const bool doWindowScale = false);

private:
	ds::ui::SpriteEngine& mEngine;

	std::shared_ptr<ci::osc::ReceiverUdp>
			  mTuioUdpSocket;
	std::shared_ptr<ci::tuio::Receiver>
			  mTuioReceiver;

	int       mFingerIdOffset;
	glm::mat4 mTransform;
	ci::Rectf mAllowedRect;
};


} // ! namespace ui
}  // !namespace ds

#endif  // !_DS_UI_TOUCH_TUIO_INPUT_H_
