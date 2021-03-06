#include "stdafx.h"

#include "scroll_area.h"
#include <glm/gtx/matrix_decompose.hpp>


namespace ds{
namespace ui{

ScrollArea::ScrollArea(ds::ui::SpriteEngine& engine, const float startWidth, const float startHeight, const bool vertical)
	: Sprite(engine)
	, mSpriteMomentum(engine)
	, mScroller(nullptr)
	, mScrollable(false)
	, mReturnAnimateTime(0.3f)
	, mWillSnapAfterDelay(false)
	, mTopFade(nullptr)
	, mBottomFade(nullptr)
	, mTopFadeActive(false)
	, mBottomFadeActive(false)
	, mFadeHeight(30.0f)
	, mFadeFullColor(0, 0, 0, 255)
	, mFadeTransColor(0, 0, 0, 0)
	, mScrollUpdatedFunction(nullptr)
	, mTweenCompleteFunction(nullptr)
	, mSnapToPositionFunction(nullptr)
	, mVertical(vertical)
	, mScrollPercent(0.0f)
	, mHandleRotatedTouches(false)
{

	setSize(startWidth, startHeight);
	mSpriteMomentum.setMass(8.0f);
	mSpriteMomentum.setFriction(0.5f);
	mSpriteMomentum.setMomentumParent(this);

	enable(false);
	setClipping(true);

	mScroller = new Sprite(mEngine);
	if(mScroller){
		mScroller->mExportWithXml = false;
// 		mScroller->setTransparent(false);
// 		mScroller->setColor(ci::Color(0.1f, 0.56f, 0.3f));
		mScroller->setSize(startWidth, startHeight);
		mScroller->enable(true);
		mScroller->enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
		mScroller->setProcessTouchCallback([this](Sprite* bs, const ds::ui::TouchInfo& ti){handleScrollTouch(bs, ti); });
		mSpriteMomentum.setMomentumParent(mScroller);
		addChildPtr(mScroller);
	}
}

void ScrollArea::setVertical(bool vertical){
	mVertical = vertical;
	// do a layout-type thing to move the fade sprites if needed, and to call checkBounds
	setScrollSize(getWidth(), getHeight());
}

void ScrollArea::setScrollSize(const float newWidth, const float newHeight){
	// onSizeChanged only triggers if the size is the same, so manually force the check bounds/fade stuff in case other things have changed
	if(getWidth() == newWidth && getHeight() == newHeight){
		onSizeChanged();
	} else {
		setSize(newWidth, newHeight);
	}
}

void  ScrollArea::enableScrolling(bool enable){
	if(mScroller){
		mScroller->enable(enable);
	}
}

void ScrollArea::stopScrollMomentum() {
	mSpriteMomentum.deactivate();
}

void ScrollArea::onSizeChanged(){
	const float newWidth = getWidth();
	const float newHeight = getHeight();

	if(mTopFade){
		if(mVertical){
			mTopFade->setSize(newWidth, mFadeHeight);
		} else {
			mTopFade->setSize(mFadeHeight, newHeight);
		}
	}
	if(mBottomFade){
		if(mVertical){
			mBottomFade->setSize(newWidth, mFadeHeight);
			mBottomFade->setPosition(0.0f, newHeight - mFadeHeight);
		} else {
			mBottomFade->setSize(mFadeHeight, newHeight);
			mBottomFade->setPosition(newWidth - mFadeHeight, 0.0f);
		}
	}

	if(mScroller){
		mScroller->setSize(0.0f, 0.0f);
		mScroller->sizeToChildBounds();
	}
	checkBounds();
}

void ScrollArea::addSpriteToScroll(ds::ui::Sprite* bs){
	if(mScroller && bs){
		mScroller->addChildPtr(bs);
		mScroller->sizeToChildBounds();
		checkBounds();
	}
}

Sprite* ScrollArea::getSpriteToPassTo(){
	return mScroller;
}

void ScrollArea::checkBounds(){
	if(!mScroller) return;
	bool doTween = true;
	ci::vec3 tweenDestination = mScroller->getPosition();

	bool snapped = callSnapToPositionCallback(doTween, tweenDestination);

	if(!snapped){
		float scrollWindow(0.0f);
		float scrollerSize(0.0f);

		if(mVertical){
			scrollWindow = getHeight();
			scrollerSize = mScroller->getHeight();
		} else {
			scrollWindow = getWidth();
			scrollerSize = mScroller->getWidth();
		}

		bool canKeepAllScrollerInWindow = false;
		if(scrollerSize <= scrollWindow){
			mScrollable = false;
			canKeepAllScrollerInWindow = true;

			// only allowable position is zero
			tweenDestination= ci::vec3(0.0f, 0.0f, 0.0f);
		} else {
			float scrollerPos(0.0f);
			if(mVertical){
				scrollerPos = mScroller->getPosition().y;
			} else {
				scrollerPos = mScroller->getPosition().x;
			}

			// find the limits
			float minPos(0.0f);
			float maxPos(0.0f);

			if(canKeepAllScrollerInWindow){
				maxPos = scrollWindow - scrollerSize;
			} else {
				minPos = scrollWindow - scrollerSize;
			}

			if(scrollerPos < minPos){
				// Can't scroll down any more
				if(mVertical){
					tweenDestination=ci::vec3(0.0f, minPos, 0.0f);
				} else {
					tweenDestination=ci::vec3(minPos, 0.0f, 0.0f);
				}
			} else if(scrollerPos > maxPos){
				// Can't scroll up any more
				if(mVertical){
					tweenDestination=ci::vec3(0.0f, maxPos, 0.0f);
				} else {
					tweenDestination=ci::vec3(maxPos, 0.0f, 0.0f);
				}
			} else {
				// In bounds
				doTween = false;
			}
		}
	}

	if(doTween){
		// respond after a delay, in case this call is cancelled in a swipe callback
		mWillSnapAfterDelay = true;
		callAfterDelay([this, doTween, tweenDestination](){
			mWillSnapAfterDelay = false;
			mSpriteMomentum.deactivate();
			mScroller->tweenPosition(tweenDestination, mReturnAnimateTime, 0.0f, ci::EaseOutQuint(), [this](){ tweenComplete(); }, [this](){ scrollerTweenUpdated(); });
			scrollerUpdated(ci::vec2(tweenDestination));
		}, 0.0f);
	} else {
		// nothing special to do here
		mScroller->animStop();
		mScroller->setPosition(tweenDestination);
		scrollerUpdated(ci::vec2(tweenDestination));
		tweenComplete();
	}
}

void ScrollArea::onUpdateServer(const ds::UpdateParams& p){
	if(mSpriteMomentum.recentlyMoved()){
		checkBounds();
	}
}

void decompose(ci::mat4 matrix, ci::vec3& scaling, ci::quat& rotation,
			   ci::vec3& position){

	ci::vec3 skew;
	ci::vec4 perspective;
	glm::decompose(matrix, scaling, rotation, position, skew, perspective);
}

void ScrollArea::handleScrollTouch(ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti){
	if(ti.mPhase == ds::ui::TouchInfo::Added){
		mSpriteMomentum.deactivate();
	} else if(ti.mPhase == ds::ui::TouchInfo::Removed && ti.mNumberFingers == 0){
		mSpriteMomentum.activate();
		checkBounds();
	} else if(ti.mPhase == ds::ui::TouchInfo::Moved && ti.mNumberFingers > 0){
		auto deltaPoint = ti.mDeltaPoint;
		if(mHandleRotatedTouches){
			auto globalTrans = getGlobalTransform();
			ci::vec3 scaley;
			ci::quat rotty;
			ci::vec3 poss;
			decompose(globalTrans, scaley, rotty, poss);
			deltaPoint = glm::rotateZ(deltaPoint, glm::roll(rotty));
		}

		if(mScroller){
			if(mVertical){
				float yDelta = deltaPoint.y / ti.mNumberFingers;
				if(getPerspective()){
					yDelta = -yDelta;
				}
				mScroller->move(0.0f, yDelta);
			} else {
				mScroller->move(deltaPoint.x / ti.mNumberFingers, 0.0f);
			}
			scrollerUpdated(ci::vec2(mScroller->getPosition()));
		}
	}

	// notify anyone (like lists) that I was touched so they can prevent action, if they want
	if(mScrollerTouchedFunction){
		mScrollerTouchedFunction();
	}
}

bool ScrollArea::callSnapToPositionCallback(bool& doTween, ci::vec3& tweenDestination){
	bool output = false;

	if(mSnapToPositionFunction){
		mSnapToPositionFunction(this, mScroller, doTween, tweenDestination);
		output = true;
	}

	return output;
}

void ScrollArea::setUseFades(const bool doFading){
	if(doFading){
		float fadeWiddy = getWidth();
		float fadeHiddy = mFadeHeight;
		if(!mVertical){
			fadeWiddy = mFadeHeight;
			fadeHiddy = getHeight();
		}
		if(!mTopFade){
			mTopFade = new ds::ui::GradientSprite(mEngine);
			if(mTopFade){
				mTopFade->mExportWithXml = false;
				mTopFade->setSize(fadeWiddy, fadeHiddy);
				mTopFade->setTransparent(false);
				mTopFade->enable(false);
				mTopFade->setOpacity(0.0f);
				addChildPtr(mTopFade);
			}
		}

		if(!mBottomFade){
			mBottomFade = new ds::ui::GradientSprite(mEngine);
			if(mBottomFade){
				mBottomFade->mExportWithXml = false;
				mBottomFade->setSize(fadeWiddy, fadeHiddy);
				mBottomFade->setTransparent(false);
				mBottomFade->enable(false);
				mBottomFade->setOpacity(0.0f);
				addChildPtr(mBottomFade);
			}
		}

		setFadeColors(mFadeFullColor, mFadeTransColor);
		setFadeHeight(mFadeHeight);
		setScrollSize(getWidth(), getHeight());
	} else {
		if(mTopFade){
			mTopFade->remove();
			mTopFade = nullptr;
		}

		if(mBottomFade){
			mBottomFade->remove();
			mBottomFade = nullptr;
		}
	}

	onSizeChanged();
}

void ScrollArea::setFadeHeight(const float fadeHeight){
	mFadeHeight = fadeHeight;
	float fadeWiddy = getWidth();
	float fadeHiddy = mFadeHeight;
	if(!mVertical){
		fadeWiddy = mFadeHeight;
		fadeHiddy = getHeight();
	}
	if(mTopFade){
		mTopFade->setSize(fadeWiddy, fadeHiddy);
	}

	if(mBottomFade){
		mBottomFade->setSize(fadeWiddy, fadeHiddy);
	}

	onSizeChanged();
}

void ScrollArea::recalculateSizes() {
	onSizeChanged();
}

void ScrollArea::setFadeColors(ci::ColorA fadeColorFull, ci::ColorA fadeColorTrans){
	mFadeFullColor = fadeColorFull;
	mFadeTransColor = fadeColorTrans;

	if(mTopFade){
		if(mVertical){
			mTopFade->setColorsAll(fadeColorFull, fadeColorFull, fadeColorTrans, fadeColorTrans);
		} else {
			mTopFade->setColorsAll(fadeColorFull, fadeColorTrans, fadeColorTrans, fadeColorFull);
		}
	}

	if(mBottomFade){
		if(mVertical){
			mBottomFade->setColorsAll(fadeColorTrans, fadeColorTrans, fadeColorFull, fadeColorFull);
		} else {
			mBottomFade->setColorsAll(fadeColorTrans, fadeColorFull, fadeColorFull, fadeColorTrans);
		}
	}
}

void ScrollArea::scrollerUpdated(const ci::vec2 scrollPos){
	float scrollerSize = mScroller->getHeight();
	float scrollWindow = getHeight();
	float scrollerPossy = scrollPos.y;

	if(!mVertical){
		scrollerSize = mScroller->getWidth();
		scrollWindow = getWidth();
		scrollerPossy = scrollPos.x;
	}
	
	const float theTop = scrollWindow - scrollerSize;

	if(theTop == 0.0f){
		mScrollPercent = 0.0f;
	} else {
		mScrollPercent = scrollerPossy / theTop;
	}
	if(mScrollPercent > 1.0f) mScrollPercent = 1.0f;
	if(mScrollPercent < 0.0f) mScrollPercent = 0.0f;

	if(mTopFade){
		if(scrollerPossy < 0.0f){
			if(!mTopFadeActive){
				mTopFade->tweenOpacity(1.0f, mReturnAnimateTime, 0.0f);
				mTopFadeActive = true;
			}
		} else {
			if(mTopFadeActive){
				mTopFade->tweenOpacity(0.0f, mReturnAnimateTime, 0.0f);
				mTopFadeActive = false;
			}
		}
	}

	if(mBottomFade){
		if(scrollerPossy > theTop){
			if(!mBottomFadeActive){
				mBottomFade->tweenOpacity(1.0f, mReturnAnimateTime, 0.0f);
				mBottomFadeActive = true;
			}
		} else if(mBottomFadeActive){
			mBottomFade->tweenOpacity(0.0f, mReturnAnimateTime, 0.0f);
			mBottomFadeActive = false;
		}
	}

	if(mScrollUpdatedFunction) mScrollUpdatedFunction(this);
}

void ScrollArea::setScrollUpdatedCallback(const std::function<void(ds::ui::ScrollArea* thisThing)> &func){
	mScrollUpdatedFunction = func;
}

void ScrollArea::setTweenCompleteCallback(const std::function<void(ds::ui::ScrollArea* thisThing)> &func){
	mTweenCompleteFunction = func;
}

void ScrollArea::setSnapToPositionCallback(const std::function<void(ScrollArea*, Sprite*, bool&, ci::vec3&)>& func){
	mSnapToPositionFunction = func;
}

void ScrollArea::setScrollerTouchedCallback(const std::function<void()>& func) {
	mScrollerTouchedFunction = func;
}

const ci::vec2 ScrollArea::getScrollerPosition(){
	if(mScroller){
		return ci::vec2(mScroller->getPosition());
	}

	return ci::vec2();
}

void ScrollArea::setScrollerPosition(ci::vec2 pos)
{
	if (mScroller)
	{
		mScroller->animStop();
		mScroller->setPosition(pos);
	}
	checkBounds();
}

void ScrollArea::resetScrollerPosition() {
	if(mScroller){
		mScroller->animStop();
		if(getPerspective() && mVertical){
			const float theTop = getHeight() - mScroller->getHeight();
			mScroller->setPosition(0.0f, getHeight() - mScroller->getHeight());
		} else {
			mScroller->setPosition(0.0f, 0.0f);
		}
		checkBounds();
	}
}

void ScrollArea::scrollerTweenUpdated(){
	if(mScrollUpdatedFunction){
		mScrollUpdatedFunction(this);
	}
}

void ScrollArea::tweenComplete(){
	if(mTweenCompleteFunction){
		mTweenCompleteFunction(this);
	}
}

float ScrollArea::getScrollPercent(){
	return mScrollPercent;
}

void ScrollArea::setScrollPercent(const float percenty){
	if(!mScroller) return;

	if(mScrollPercent == percenty) return;

	float scrollerSize = mScroller->getHeight();
	float scrollWindow = getHeight();

	if(!mVertical){
		scrollerSize = mScroller->getWidth();
		scrollWindow = getWidth();
	}

	const float theTop = scrollWindow - scrollerSize;

	float scrollerPossy = percenty * theTop;

	if(mVertical){
		if(getPerspective()){
			scrollerPossy = theTop - scrollerPossy;
		}
		mScroller->setPosition(0.0f, scrollerPossy);
	} else {
		mScroller->setPosition(scrollerPossy, 0.0f);
	}
	
	scrollerUpdated(ci::vec2(mScroller->getPosition()));
}

void ScrollArea::tweenScrollPercent(const float percenty){
	if (!mScroller) return;

	if (mScrollPercent == percenty) return;

	float scrollerSize = mScroller->getHeight();
	float scrollWindow = getHeight();

	if (!mVertical){
		scrollerSize = mScroller->getWidth();
		scrollWindow = getWidth();
	}

	const float theTop = scrollWindow - scrollerSize;

	float scrollerPossy = percenty * theTop;


	if (mVertical){
		if (getPerspective()){
			scrollerPossy = theTop - scrollerPossy;
		}
		mScroller->tweenPosition(ci::vec3(0.0f, scrollerPossy, 0.0f), mReturnAnimateTime, 0.0f, ci::easeOutQuint);
	}
	else {
		mScroller->tweenPosition(ci::vec3(scrollerPossy, 0.0f, 0.0f), mReturnAnimateTime, 0.0f, ci::easeOutQuint);
	}

	tweenNormalized(mReturnAnimateTime, 0.0f, ci::easeOutQuint, [this]{
			scrollerUpdated(ci::vec2(mScroller->getPosition()));
		}, [this]{
			scrollerUpdated(ci::vec2(mScroller->getPosition()));
		});

}

float ScrollArea::getVisiblePercent(){
	if(!mScroller) return 0.0f;

	if(mVertical){
		if(mScroller->getHeight() < 1.0f) return 1.0f;
		if(mScroller->getHeight() <= getHeight()) return 1.0f;
		
		return getHeight() / mScroller->getHeight();
	} else {
		if(mScroller->getWidth() < 1.0f) return 1.0f;
		if(mScroller->getWidth() <= getWidth()) return 1.0f;

		return getWidth() / mScroller->getWidth();
	}
}

void ScrollArea::scrollPage(const bool forwards, const bool animate) {
	if(!mScroller) return;

	const float visibPerc = getVisiblePercent();
	if(visibPerc == 1.0f) return;


	float scrollerSize = mScroller->getHeight();
	float scrollWindow = getHeight();

	if(!mVertical){
		scrollerSize = mScroller->getWidth();
		scrollWindow = getWidth();
	}

	const float theTop = scrollWindow - scrollerSize;

	float visiblePixes = visibPerc * mScroller->getHeight();
	if(!mVertical)visiblePixes = visibPerc * mScroller->getWidth();

	const float scrolledPerc = getScrollPercent();
	float scrolledPixes = scrolledPerc * theTop;
	if(!mVertical) scrolledPixes = scrolledPerc * theTop;

	float destPixels = scrolledPixes;
	float fadePixels = 0.0f;
	if(mTopFade && mBottomFade){
		if(mVertical){
			fadePixels = mTopFade->getHeight();
		} else {
			fadePixels = mTopFade->getWidth();
		}
	}

	if(forwards){
		destPixels -= (visiblePixes -fadePixels);
	} else {
		destPixels += (visiblePixes - fadePixels);
	}

	if(destPixels < theTop) destPixels = theTop;
	if(destPixels > 0.0f) destPixels = 0.0f;


	ci::vec3 tweenDestination = mScroller->getPosition();
	if(mVertical){
		if(getPerspective()){
			destPixels = theTop - destPixels;
		}
		tweenDestination.y = destPixels;
	} else {
		tweenDestination.x = destPixels;
	}

	mSpriteMomentum.deactivate();
	if(animate){
		mScroller->tweenPosition(tweenDestination, mReturnAnimateTime, 0.0f, ci::EaseInOutQuint(), nullptr, [this](){ scrollerTweenUpdated(); });
		scrollerUpdated(ci::vec2(tweenDestination));
	} else {
		mScroller->animStop();
		mScroller->setPosition(tweenDestination);
		scrollerUpdated(ci::vec2(tweenDestination));
	}


}

} // namespace ui

} // namespace ds
