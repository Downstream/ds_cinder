#pragma once
#ifndef DS_UI_TOUCH_TOUCH_EVENT
#define DS_UI_TOUCH_TOUCH_EVENT

#include <cinder/app/TouchEvent.h>

namespace ds {
namespace ui {

/**
* \class TouchEvent
* \brief Wraps ci::app::TouchEvent to provide additional data members
*/
class TouchEvent : public ci::app::TouchEvent {
public:
	TouchEvent() = delete;
	TouchEvent(ci::app::TouchEvent& cinderEvent)
		: ci::app::TouchEvent(cinderEvent){}
	TouchEvent(ci::app::WindowRef win, const std::vector<ci::app::TouchEvent::Touch> &touches, const bool inWorldSpace = false)
		: ci::app::TouchEvent(win, touches), mInWorldSpace(inWorldSpace){}

	/// If this touch event is known to be in world space co-ordinates already
	const bool			getInWorldSpace() const {	return mInWorldSpace;	}

	/// If a piece of the system is sure that the touch event co-ordinates are in world space and require no further translation
	void				setInWorldSpace(const bool inWorldSpace){ mInWorldSpace = inWorldSpace; }


private:
	bool				mInWorldSpace = false;
};

} // namespace ui
} // namespace ds

#endif
