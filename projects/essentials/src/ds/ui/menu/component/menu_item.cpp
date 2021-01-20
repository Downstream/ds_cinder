#include "stdafx.h"

#include "menu_item.h"

#include <boost/algorithm/string.hpp>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/app/engine/engine_cfg.h>
#include <ds/app/environment.h>
#include <ds/ui/tween/tweenline.h>
#include <ds/util/string_util.h>

namespace ds{
namespace ui{

MenuItem::MenuItem(ds::ui::SpriteEngine& enginey, const ds::ui::TouchMenu::MenuItemModel itemModel, const ds::ui::TouchMenu::TouchMenuConfig config)
	: ds::ui::Sprite(enginey)
	, mClippy(nullptr)
	, mMenuItemModel(itemModel)
	, mMenuConfig(config)
	, mHighlighted(false)
	, mRotation(0.0f)
	, mClipper(nullptr)
	, mIcon(nullptr)
	, mIconGlow(nullptr)
	, mIconsMatch(false)
	, mActive(false)
	, mLines(nullptr)
	, mTitle(nullptr)
	, mSubtitle(nullptr)
{

	if(mMenuItemModel.mIconHighlightedImage.empty()){
		mMenuItemModel.mIconHighlightedImage = mMenuItemModel.mIconNormalImage;
	}

	if(!mMenuItemModel.mIconNormalImage.empty()){
		mIcon = new ds::ui::Image(mEngine, mMenuItemModel.mIconNormalImage);
	}

	if(!mMenuItemModel.mIconHighlightedImage.empty()){
		mIconGlow = new ds::ui::Image(mEngine, mMenuItemModel.mIconHighlightedImage);
	}

	if(mIcon && mIconGlow){
		mIconsMatch = (mMenuItemModel.mIconNormalImage == mMenuItemModel.mIconHighlightedImage);
	}

	float iconHeight = mMenuConfig.mItemIconHeight;
	float padding = mMenuConfig.mItemTitlePad; 
	float titleYPercent = mMenuConfig.mItemTitleYPositionPercent;
	ci::vec2 thisSize = mMenuConfig.mItemSize;
	setSize(thisSize.x, thisSize.y);

	mClipper = new ds::ui::Sprite(mEngine, getWidth(), getHeight());
	mClipper->enable(false);

	addChild(*mClipper);

	mClippy = new ds::ui::Sprite(mEngine);
	mClippy->enable(false);
	mClipper->addChild(*mClippy);


	if(mMenuConfig.mDoClipping){
		mClipper->setClipping(true);
	} else {
		mClippy->setOpacity(0.0f);
	}

	if(!mMenuConfig.mItemTitleTextConfig.empty()){
		mTitle = new ds::ui::Text(mEngine);
		addChildPtr(mTitle);
		mTitle->setTextStyle(mMenuConfig.mItemTitleTextConfig);
	}

	if (!mMenuConfig.mItemSubtitleTextConfig.empty()) {
		mSubtitle = new ds::ui::Text(mEngine);
		addChildPtr(mSubtitle);
		mSubtitle->setTextStyle(mMenuConfig.mItemSubtitleTextConfig);
	}

	float titlePositiony = thisSize.y * titleYPercent;
	float subtitlePositiony = titlePositiony;
	if(mTitle){
		mTitle->setText(mMenuItemModel.mTitle);
		mTitle->setResizeLimit(mMenuConfig.mItemTitleResizeLimit.x, mMenuConfig.mItemTitleResizeLimit.y);
		ci::vec2 titleSize = ci::vec2(mTitle->getWidth(), mTitle->getHeight());
		mTitle->setPosition(thisSize.x * 0.5f - titleSize.x * 0.5f, titlePositiony);
		mTitle->setOpacity(mMenuConfig.mItemTitleOpacity);
		mClippy->addChildPtr(mTitle);
		subtitlePositiony += (titleSize.y * 1.1f);
	}

	if(mSubtitle){
		mSubtitle->setText(mMenuItemModel.mSubtitle);
		mSubtitle->setResizeLimit(mMenuConfig.mItemTitleResizeLimit.x, mMenuConfig.mItemTitleResizeLimit.y);
		ci::vec2 titleSize = ci::vec2(mSubtitle->getWidth(), mSubtitle->getHeight());
		mSubtitle->setPosition(thisSize.x * 0.5f - titleSize.x * 0.5f, subtitlePositiony);
		mSubtitle->setOpacity(mMenuConfig.mItemSubtitleOpacity);
		mClippy->addChildPtr(mSubtitle);
	}

	mClippy->setPosition(0.0f, -getHeight());

	if(mIcon){
		float newScale = iconHeight / mIcon->getHeight();
		mClippy->addChild(*mIcon);
		mIcon->setCenter(0.5f, 0.5f);
		mIcon->setScale(newScale, newScale);
		mIcon->setPosition(getWidth() * 0.5f, titlePositiony - (iconHeight * 0.5f * newScale) - padding);
	}
	if(mIcon && mIconGlow) {
		mClippy->addChild(*mIconGlow);
		mIconGlow->setCenter(mIcon->getCenter());
		mIconGlow->setScale(mIcon->getScale());
		mIconGlow->setPosition(mIcon->getPosition());
		mIconGlow->setOpacity(0.0f);
	}

	enable(false);
}

void MenuItem::animateOn(){
	if(!mClippy || mActive) return;

	if(mMenuConfig.mAnimationStyle != TouchMenu::TouchMenuConfig::kAnimateRadial){
		if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateRight){
			mClippy->setPosition(-getWidth(), 0.0f);
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateUp) {
			mClippy->setPosition(0.0f, -getHeight());
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateLeft){
			mClippy->setPosition(getWidth(), 0.0f);
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateDown){
			mClippy->setPosition(0.0f, getHeight());
		}
		mClippy->tweenPosition(ci::vec3(), mMenuConfig.mAnimationDuration, 0.0f, ci::easeInOutCubic);
	} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateRadial){
		mClippy->setPosition(-getWidth() / 2.0f - getPosition().x, -getHeight() / 2.0f - getPosition().y, 0.0f);
		mClippy->tweenPosition(ci::vec3(), mMenuConfig.mAnimationDuration, 0.0f, ci::easeOutQuint);
	}

	if(!mMenuConfig.mDoClipping){
		mClippy->setOpacity(0.0f);
		mClippy->tweenOpacity(1.0f, mMenuConfig.mAnimationDuration / 2.0f, 0.0f);
	}


	mActive = true;
}

void MenuItem::animateOff(){
	if(!mClippy || !mActive) return;


	if(mMenuConfig.mAnimationStyle != TouchMenu::TouchMenuConfig::kAnimateRadial){
		ci::vec3 destPos = ci::vec3();
		if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateRight){
			destPos = ci::vec3(-getWidth(), 0.0f, 0.0f);
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateUp) {
			destPos = ci::vec3(0.0f, -getHeight(), 0.0f);
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateLeft){
			destPos = ci::vec3(getWidth(), 0.0f, 0.0f);
		} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateDown){
			destPos = ci::vec3(0.0f, getHeight(), 0.0f);
		}

		mClippy->tweenPosition(destPos, mMenuConfig.mAnimationDuration, 0.0f, ci::easeInOutCubic);
	} else if(mMenuConfig.mAnimationStyle == TouchMenu::TouchMenuConfig::kAnimateRadial){
		mClippy->tweenPosition(ci::vec3(- getWidth() / 2.0f - getPosition().x, -getHeight() / 2.0f - getPosition().y, 0.0f), mMenuConfig.mAnimationDuration, 0.0f, ci::easeInQuint);
	} 

	if(!mMenuConfig.mDoClipping){
		mClippy->tweenOpacity(0.0f, mMenuConfig.mAnimationDuration / 2.0f, mMenuConfig.mAnimationDuration / 2.0f);
	}

	mActive = false;
}

void MenuItem::highlight(){
	if(!mHighlighted){
		if(mTitle){
			mTitle->tweenOpacity(1.0f, mMenuConfig.mAnimationDuration);
		}
		if(mSubtitle){
			mSubtitle->tweenOpacity(1.0f, mMenuConfig.mAnimationDuration);
		}
		if(mIcon && mIconGlow && !mIconsMatch) {
			mIcon->tweenOpacity(0.0f, mMenuConfig.mAnimationDuration);
			mIconGlow->tweenOpacity(1.0f, mMenuConfig.mAnimationDuration);
		}
	}

	mHighlighted = true;
}

void MenuItem::unhighlight(){
	if(mHighlighted){
		if(mTitle){
			mTitle->tweenOpacity(mMenuConfig.mItemTitleOpacity, mMenuConfig.mAnimationDuration);
		}
		if(mSubtitle){
			mSubtitle->tweenOpacity(mMenuConfig.mItemSubtitleOpacity, mMenuConfig.mAnimationDuration);
		}
		if(mIcon && mIconGlow && !mIconsMatch) {
			mIcon->tweenOpacity(1.0f, mMenuConfig.mAnimationDuration);
			mIconGlow->tweenOpacity(0.0f, mMenuConfig.mAnimationDuration);
		}
	}

	mHighlighted = false;
}

} // namespace ui
} // namespace ds