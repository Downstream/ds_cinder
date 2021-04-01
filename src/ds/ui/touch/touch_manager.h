#pragma once
#ifndef DS_UI_TOUCH_MANAGER_H
#define DS_UI_TOUCH_MANAGER_H

#include <map>
#include <memory>
#include <cinder/app/TouchEvent.h>
#include <cinder/app/MouseEvent.h>
#include <cinder/Color.h>
#include <cinder/Rect.h>
#include "touch_mode.h"
#include "touch_info.h"

namespace ds {
class Engine;

namespace ui {
class RotationTranslator;
class Sprite;
class TouchEvent;

class TouchManager {
public:
class Capture {
public:
	Capture()				{ }
	virtual ~Capture()		{ }
	virtual void							touchBegin(const TouchInfo&) = 0;
	virtual void							touchMoved(const TouchInfo&) = 0;
	virtual void							touchEnd(const TouchInfo&) = 0;
};

public:
	typedef enum { kInputNormal = 0, kInputTranslate, kInputScale } InputMode;

	TouchManager(Engine&, const TouchMode::Enum&);

	/// Sets the input mode. kInputNormal is the most common and passes touch and mouse to the app.
	///		Translate will move the src rect around
	///		Scale will scale the src rect
	void									setInputMode(const InputMode& theMode);
	const InputMode&						getInputMode() const { return mInputMode; }
	void									setTouchMode(const TouchMode::Enum&);

	void									mouseTouchBegin(const ci::app::MouseEvent&, int id);
	void									mouseTouchMoved(const ci::app::MouseEvent&, int id);
	void									mouseTouchEnded(const ci::app::MouseEvent&, int id);

	void									touchesBegin(const ds::ui::TouchEvent&);
	void									touchesMoved(const ds::ui::TouchEvent&);
	void									touchesEnded(const ds::ui::TouchEvent&);

	void									clearFingers(const std::vector<int> &fingers);

	void									setSpriteForFinger(const int fingerId, ui::Sprite* theSprite);
	Sprite*									getSpriteForFinger(const int fingerId);

	void									clearFingersForSprite(ui::Sprite* theSprite);

	void									setOverrideTranslation(const bool doOverride){ mOverrideTranslation = doOverride; }
	void									setOverrideDimensions(const ci::vec2& dimensions){ mTouchDimensions = dimensions; }
	void									setOverrideOffset(const ci::vec2& offset){ mTouchOffset = offset; }
	void									setTouchFilterRect(const ci::Rectf &filterRect){ mTouchFilterRect = filterRect; }
	void									setTouchFilterFunc(const std::function<bool(const ci::vec2& p)> &func){ mTouchFilterFunc = func; }

	bool									getOverrideEnabled(){ return mOverrideTranslation; }

	/// If you've set the override for translation, actually do that translation
	void									overrideTouchTranslation(ci::vec2& inOutPoint);

	/// If we have a rect defined to discard touches, discard that shit!
	bool									shouldDiscardTouch(const ci::vec2& p);

	void									setCapture(Capture*);

	std::map<int, ci::vec3>&				getPreviousTouchPoints(){ return mTouchPreviousPoint; }

	void									setTouchSmoothing(const bool doSmoothing);
	const bool								getTouchSmoothing(){ return mSmoothEnabled; }

	void									setTouchSmoothFrames(const int smoothFrames);

private:
	/// Utility to get the hit sprite in either the orthogonal or perspective root sprites
	Sprite* 								getHit(const ci::vec3 &point);

	/// If the window is stretched, the mouse points will be off. Fix that shit!
	ci::vec2								translateMousePoint(const ci::ivec2);

	void									inputBegin(const int fingerId, const ci::vec2& globalPos);
	void									inputMoved(const int fingerId, const ci::vec2& globalPos);
	void									inputEnded(const int fingerId, const ci::vec2& globalPos);

	std::map<int, std::vector<ci::vec3>>	mTouchSmoothPoints;
	bool									mSmoothEnabled;
	int										mFramesToSmooth;

	Engine&									mEngine;

	std::map<int, ui::Sprite*>				mFingerDispatcher;
	std::map<int, ci::vec3>					mTouchStartPoint;
	std::map<int, ci::vec3>					mTouchPreviousPoint;
	std::map<int, bool>						mDiscardTouchMap;

	ci::vec2								mTouchDimensions;
	ci::vec2								mTouchOffset;
	bool									mOverrideTranslation;
	ci::Rectf								mTouchFilterRect;
	std::function<bool(const ci::vec2& p)>	mTouchFilterFunc;

	InputMode								mInputMode;
	int										mTransScaleFingId;
	ci::vec2								mTransScaleOrigin;
	ci::Rectf								mStartSrcRect;

	TouchMode::Enum							mTouchMode;
	Capture*								mCapture;

	/// This is overkill but done this way so I can make changes to
	/// the rotation translator without causing a recompile.
	std::shared_ptr<RotationTranslator>		mRotationTranslatorPtr;
	RotationTranslator&						mRotationTranslator;
};

} // namespace ui
} // namespace ds

#endif//DS_UI_TOUCH_MANAGER_H
