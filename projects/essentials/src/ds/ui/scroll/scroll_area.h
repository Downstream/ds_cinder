#pragma once

#include <ds/ui/sprite/sprite.h>
#include <ds/ui/touch/momentum.h>
#include <functional>
#include <ds/ui/sprite/gradient_sprite.h>

namespace ds {
namespace ui {

class ScrollArea : public ds::ui::Sprite {
public:

	/// Creates a ScrollArea with a Scroller. The scroller is the thing that scrolls inside this sprite
	/// Anything you add to this needs to be added using addSpriteToScroll() so children get added properly
	/// The scroller sizes itself based on the size of it's children, so be sure that your children have the correct size
	/// Call recalculate sizes if any of your children change size
	ScrollArea(ds::ui::SpriteEngine& engine, const float startWidth, const float startHeight, const bool verticalScrolling = true);

	/// If true, scrolls vertically. False will scroll horizontally
	void				setVertical(bool vertical);

	/// Sets the clippin' (visible) area of the scroll Area. 
	void				setScrollSize(const float newWidth, const float newHeight);

    /// Enable/disable touches so the user can scroll this
	void				enableScrolling(bool enable);

	/// Stop any current movement of the scrolling
	void				stopScrollMomentum();

	/// Set the position of your sprite before you add it here.
	/// The scroll area will be sized after the new sprite is passed in.
	void				addSpriteToScroll(ds::ui::Sprite* bs);

	/// if you have buttons or whatnot in the scroll area, when you start dragging
	/// you'll want to pass the touch to a sprite, the sprite returned by this function.
	Sprite*				getSpriteToPassTo();

	/// If this area can be scrolled
	const bool			getScrollable() const { return mScrollable; }

	/// Sets the fade colors on the top and bottom (vertical scrolling) or left and right (horizontal) if there is more content in that direction
	void				setFadeColors(ci::ColorA fadeColorFull, ci::ColorA fadeColorTransparent);
	/// Turns fades on or off
	void				setUseFades(const bool doFading);
	/// Sets the size of the fades in pixels
	void				setFadeHeight(const float fadeHeight);

	/// Renders the scroller to a texture and uses a shader to fade off the top and bottom instead of gradients
	/// CAVEATS! Any children cannot use blend modes; this scroll area cannot be inside of any clipping areas; this probably wont work with any rotated clipping children
	void				setUseShaderFade(const bool shaderFade);

	/// recalculates the size of the scroller and fades
	void				recalculateSizes();

	/// Callback called when the scroll position has changed
	void				setScrollUpdatedCallback(const std::function<void(ScrollArea* thisThing)> &func);

	/// Callback called when any movement tweens have finished
	void				setTweenCompleteCallback(const std::function<void(ScrollArea*)>& func);

	/// Callback called when some bounds have been reached and the scroller snaps back to the bounds
	void				setSnapToPositionCallback(const std::function<void(ScrollArea*, Sprite*, bool&, ci::vec3&)>& func);

	/// Callback called when some interaction happened with the scroller
	void				setScrollerTouchedCallback(const std::function<void()>& func); 

	/// Returns the current position of the scroller inside the area
	const ci::vec2		getScrollerPosition();

	/// Directly sets the position of the scrolling scroller. Bounds will be checked to keep the scroller from having an invalid position
	void				setScrollerPosition(ci::vec2);

	/// Resets the scroller to the top / front / beginning
	void				resetScrollerPosition();

	/// For external UI use. The 0.0 - 1.0 percent of the scroll. 0.0 == the start (top in vertical scrolls). 1.0 == the bottom (fully scrolled through the list)
	float				getScrollPercent();
		
	/// Set the percentage scroller from 0.0 - 1.0, where 0.0 is the top and 1.0 is the bottom / end.
	void				setScrollPercent(const float percenty);

	/// Tweens/animates teh scroller to this percentage from 0.0-1.0
	void				tweenScrollPercent(const float percenty);

	/// How much of the scroller is currently visible. If the scroller is smaller than the scroll area, then this will be 1.0
	float				getVisiblePercent();

	/// Move the scroll forwards or backwards by a "page", defined by the visible area minus the size of the fades (if present)
	/// May not work correctly in perspective
	void				scrollPage(const bool forwards, const bool animate = true);

	/// The duration of the animation for when the scroller reaches it's bounds and snaps back
	void				setReturnAnimateTime(const float dur){ mReturnAnimateTime = dur; }

	/// If this scroll area is rotated globally, rotate the touch delta by that amount. Default = false
	void				handleTouchesRotated(const bool doRotated) { mHandleRotatedTouches = doRotated; }

	void				checkBounds(const bool immediate = false);

protected:
	virtual void		onUpdateServer(const ds::UpdateParams& p) override;
	virtual void		onSizeChanged() override;
	void				scrollerUpdated(const ci::vec2 scrollPos);
	void				scrollerTweenUpdated();
	void				tweenComplete();
	void				handleScrollTouch(ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti);
	virtual bool		callSnapToPositionCallback(bool& doTween, ci::vec3& tweenDestination);
	virtual void		drawClient(const ci::mat4 &transformMatrix, const ds::DrawParams &drawParams) override;

	Sprite*				mScroller;
	bool				mScrollable;
	Momentum			mSpriteMomentum;
	float				mReturnAnimateTime;
	bool				mWillSnapAfterDelay;

	GradientSprite*		mTopFade;
	GradientSprite*		mBottomFade;
	float				mFadeHeight;
	ci::ColorA			mFadeTransColor;
	ci::ColorA			mFadeFullColor;
	bool				mTopFadeActive;
	bool				mBottomFadeActive;
	bool				mHandleRotatedTouches;

	bool				mShaderFade;
	ci::gl::GlslProgRef	mShaderShader;

	bool				mVertical;

	// the current 0.0 - 1.0 percent of the scroll position
	// Only used for external ui, not internally
	float				mScrollPercent;

	std::function<void(ScrollArea*)>	mScrollUpdatedFunction;
	std::function<void(ScrollArea*)>	mTweenCompleteFunction;
	std::function<void(ScrollArea*, Sprite*, bool&, ci::vec3&)>	mSnapToPositionFunction;
	std::function<void()>				mScrollerTouchedFunction;

};

} // namespace ui
} // namespace ds