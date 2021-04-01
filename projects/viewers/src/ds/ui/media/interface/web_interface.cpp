#include "stdafx.h"

#include "web_interface.h"

#include <ds/app/environment.h>
#include <ds/app/engine/engine_cfg.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/app/engine/engine_events.h>
#include <ds/ui/sprite/image.h>
#include <ds/debug/logger.h>
#include <ds/util/string_util.h>

#include <ds/ui/sprite/web.h>
#include <ds/ui/button/image_button.h>
#include <ds/ui/sprite/text.h>

#include <ds/ui/layout/layout_sprite.h>

#include <ds/ui/soft_keyboard/entry_field.h>
#include <ds/ui/soft_keyboard/soft_keyboard.h>
#include <ds/ui/soft_keyboard/soft_keyboard_defs.h>
#include <ds/ui/soft_keyboard/soft_keyboard_button.h>
#include <ds/ui/soft_keyboard/soft_keyboard_builder.h>

namespace ds {
namespace ui {

WebInterface::WebInterface(ds::ui::SpriteEngine& eng, const ci::vec2& sizey, const float buttonHeight, const ci::Color buttonColor, const ci::Color backgroundColor)
	: MediaInterface(eng, sizey, backgroundColor)
	, mLinkedWeb(nullptr)
	, mKeyboardArea(nullptr)
	, mKeyboard(nullptr)
	, mKeyboardShowing(false)
	, mKeyboardAbove(true)
	, mKeyboardButton(nullptr)
	, mBackButton(nullptr)
	, mForwardButton(nullptr)
	, mRefreshButton(nullptr)
	, mTouchToggle(nullptr)
	, mKeyboardKeyScale(1.0f)
	, mAbleToTouchToggle(true)
	, mKeyboardAllowed(true)
	, mKeyboardAutoDisablesTimeout(true)
	, mWebLocked(false)
	, mUserField(nullptr)
	, mAuthLayout(nullptr)
	, mPasswordField(nullptr)
	, mAuthorizing(false)
	, mEventClient(mEngine)
{
	mKeyboardArea = new ds::ui::Sprite(mEngine, 10.0f, 10.0f);
	mKeyboardArea->setTransparent(false);
	mKeyboardArea->setColor(ci::Color(0.0f, 0.0f, 0.0f));
	mKeyboardArea->setCornerRadius(15.0f);
	mKeyboardArea->setOpacity(0.0f);
	mKeyboardArea->hide();
	addChildPtr(mKeyboardArea);

	mEventClient.listenToEvents<ds::app::EntryFieldRegisteredEvent>([this](auto&) {
		auto newEntryField = mEngine.getRegisteredEntryField();
		if(newEntryField && newEntryField != mLinkedWeb) {
			mKeyboardShowing = false;
			updateWidgets();
		}
	});

	mKeyboardButton = new ds::ui::ImageButton(mEngine, "%APP%/data/images/media_interface/keyboard.png", "%APP%/data/images/media_interface/keyboard.png", (sizey.y - buttonHeight) / 2.0f);
	addChildPtr(mKeyboardButton);
	mKeyboardButton->setClickFn([this](){
		toggleKeyboard();
	});

	mKeyboardButton->getNormalImage().setColor(buttonColor);
	mKeyboardButton->getHighImage().setColor(buttonColor / 2.0f);
	mKeyboardButton->setScale(sizey.y / mKeyboardButton->getHeight());


	mBackButton = new ds::ui::ImageButton(mEngine, "%APP%/data/images/media_interface/prev.png", "%APP%/data/images/media_interface/prev.png", (sizey.y - buttonHeight) / 2.0f);
	addChildPtr(mBackButton);
	mBackButton->setClickFn([this](){
		if(mLinkedWeb){
			mLinkedWeb->goBack();
			updateWidgets();
		}
	});

	mBackButton->getNormalImage().setColor(buttonColor);
	mBackButton->getHighImage().setColor(buttonColor / 2.0f);
	mBackButton->setScale(sizey.y / mBackButton->getHeight());


	mForwardButton = new ds::ui::ImageButton(mEngine, "%APP%/data/images/media_interface/next.png", "%APP%/data/images/media_interface/next.png", (sizey.y - buttonHeight) / 2.0f);
	addChildPtr(mForwardButton);
	mForwardButton->setClickFn([this](){
		if(mLinkedWeb){
			mLinkedWeb->goForward();
			updateWidgets();
		}
	});

	mForwardButton->getNormalImage().setColor(buttonColor);
	mForwardButton->getHighImage().setColor(buttonColor / 2.0f);
	mForwardButton->setScale(sizey.y / mForwardButton->getHeight());


	mRefreshButton = new ds::ui::ImageButton(mEngine, "%APP%/data/images/media_interface/refresh.png", "%APP%/data/images/media_interface/refresh.png", (sizey.y - buttonHeight) / 2.0f);
	addChildPtr(mRefreshButton);
	mRefreshButton->setClickFn([this](){
		if(mLinkedWeb){
			mLinkedWeb->reload();
			updateWidgets();
		}
	});

	mRefreshButton->getNormalImage().setColor(buttonColor);
	mRefreshButton->getHighImage().setColor(buttonColor / 2.0f);
	mRefreshButton->setScale(sizey.y / mRefreshButton->getHeight());


	mTouchToggle = new ds::ui::ImageButton(mEngine, "%APP%/data/images/media_interface/touch_unlocked.png", "%APP%/data/images/media_interface/touch_unlocked.png", (sizey.y - buttonHeight) / 2.0f);
	addChildPtr(mTouchToggle);
	mTouchToggle->setClickFn([this](){
		toggleTouch();
	});

	mTouchToggle->getNormalImage().setColor(buttonColor);
	mTouchToggle->getHighImage().setColor(buttonColor / 2.0f);
	mTouchToggle->setScale(sizey.y / mTouchToggle->getHeight());

	if(!mAbleToTouchToggle){
		mTouchToggle->hide();
	}



	const float padding = sizey.y / 4.0f;

	mMinWidth = (
		mKeyboardButton->getScaleWidth() + padding +
		mBackButton->getScaleWidth() + padding +
		mForwardButton->getScaleWidth() + padding +
		mRefreshButton->getScaleWidth() + padding +
		mTouchToggle->getScaleWidth() + padding * 2.0f
		);
	mMaxWidth = mMinWidth;

	updateWidgets();
}

void WebInterface::setKeyboardAllow(const bool keyboardAllowed){
	mKeyboardAllowed = keyboardAllowed;
	if(mKeyboardButton){
		if(mKeyboardAllowed){
			mKeyboardButton->show();
		} else {
			mKeyboardButton->hide();
		}
		layout();
	}
}


void WebInterface::setKeyboardAbove(const bool kerboardAbove){
	mKeyboardAbove = kerboardAbove;

	layout();
}

void WebInterface::setAllowTouchToggle(const bool allowTouchToggling){
	mAbleToTouchToggle = allowTouchToggling;
	if(mTouchToggle){
		if(mAbleToTouchToggle){
			mTouchToggle->show();
		} else {
			mTouchToggle->hide();
		}

		layout();
	}
}

void WebInterface::setKeyboardDisablesTimeout(const bool doAutoTimeout){
	mKeyboardAutoDisablesTimeout = doAutoTimeout;
}


void WebInterface::startAuthCallback(const std::string& host, const std::string& realm){

	if(!mLinkedWeb || !mKeyboardArea) return;

	// If they can't type anything, then cancel the auth request
	if(!mKeyboardAllowed){
		mLinkedWeb->authCallbackCancel();
	}

	if(mAuthLayout){
		mAuthLayout->release();
		mAuthLayout = nullptr;
		mUserField = nullptr;
		mPasswordField = nullptr;
	}

	mAuthLayout = new ds::ui::LayoutSprite(mEngine);
	mAuthLayout->setShrinkToChildren(ds::ui::LayoutSprite::kShrinkHeight);
	mAuthLayout->setTransparent(false);
	mAuthLayout->setColor(ci::Color::black());
	mAuthLayout->setCornerRadius(mKeyboardArea->getCornerRadius());

	mAuthLayout->enable(true);
	mAuthLayout->enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	mAuthLayout->setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3& pos){
		if(mUserField && mPasswordField){
			if(mUserField->getIsInFocus()){
				mUserField->unfocus();
				mPasswordField->focus();
			} else {
				mUserField->focus();
				mPasswordField->unfocus();
			}
		}
	});
	addChildPtr(mAuthLayout);

	float padding = getHeight() / 4.0f;
	auto innerLayout = new ds::ui::LayoutSprite(mEngine);
	innerLayout->setSpacing(padding);
	innerLayout->setShrinkToChildren(ds::ui::LayoutSprite::kShrinkHeight);
	innerLayout->mLayoutBPad = padding * 2.0f;
	innerLayout->mLayoutRPad = padding * 2.0f;
	innerLayout->mLayoutTPad = padding * 2.0f;
	innerLayout->mLayoutLPad = padding * 2.0f;
	mAuthLayout->addChildPtr(innerLayout);

	auto authLabel = new ds::ui::Text(mEngine);
	innerLayout->addChildPtr(authLabel);
	authLabel->setTextStyle("viewer:widget");
	std::stringstream ss;
	ss << realm << " - "  << host;
	authLabel->setText(ss.str());

	auto userLabel = new ds::ui::Text(mEngine);
	innerLayout->addChildPtr(userLabel);
	userLabel->setTextStyle("viewer:widget");
	userLabel->setFontSize(userLabel->getFontSize() * 0.66667f);
	userLabel->setText("Username");
	userLabel->mLayoutTPad = padding;

	ds::ui::EntryFieldSettings efs;
	efs.mTextConfig = "viewer:widget";
	efs.mFieldSize.x = getWidth();
	efs.mFieldSize.y = mEngine.getTextStyle("viewer:widget").mSize * 1.25;
	efs.mCursorSize.y = efs.mFieldSize.y;
	mUserField = new ds::ui::EntryField(mEngine, efs);
	mUserField->focus();
	innerLayout->addChildPtr(mUserField);

	auto passLabel = new ds::ui::Text(mEngine);
	innerLayout->addChildPtr(passLabel);
	passLabel->setTextStyle("viewer:widget");
	passLabel->setFontSize(passLabel->getFontSize() * 0.66667f);
	passLabel->setText("Password");
	passLabel->mLayoutTPad = padding;

	efs.mPasswordMode = true;
	mPasswordField = new ds::ui::EntryField(mEngine, efs);
	mPasswordField->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
	mPasswordField->focus();
	mPasswordField->unfocus();
	innerLayout->addChildPtr(mPasswordField);

	auto cancelButton = new ds::ui::Text(mEngine);
	innerLayout->addChildPtr(cancelButton);
	cancelButton->setTextStyle("viewer:widget");
	cancelButton->setFontSize(cancelButton->getFontSize()); //?????
	cancelButton->setText("Cancel");
	cancelButton->mLayoutTPad = padding;
	cancelButton->enable(true);
	cancelButton->enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	cancelButton->setTapCallback([this](ds::ui::Sprite*, const ci::vec3&){
		cancelAuth();
	});

	mAuthorizing = true;
	showKeyboard(true);

	mAuthLayout->setOpacity(0.0f);
	mAuthLayout->tweenOpacity(1.0f, mAnimateDuration);

	if(mCanDisplay){
		userInputReceived();
	}
}


void WebInterface::cancelAuth(){
	if(!mAuthorizing) return;
	if(mLinkedWeb){
		mLinkedWeb->authCallbackCancel();
	}

	authComplete();
}


void WebInterface::authComplete(){
	if(!mAuthorizing || !mAuthLayout) return;
	mAuthorizing = false;
	auto aa = mAuthLayout;
	mAuthLayout->tweenOpacity(0.0f, mAnimateDuration, 0.0f, ci::easeNone, [this, aa]{ aa->release(); });
	mAuthLayout = nullptr;
	mUserField = nullptr;
	mPasswordField = nullptr;
}

void WebInterface::setKeyboardKeyScale(const float newKeyScale){
	mKeyboardKeyScale = newKeyScale;
}

void WebInterface::animateOff(){
	mIdling = false;

	cancelAuth();

	tweenOpacity(0.0f, mAnimateDuration, 0.0f, ci::EaseNone(), [this]{
		hide();
		showKeyboard(false);
	});
}

void WebInterface::linkWeb(ds::ui::Web* linkedWeb){
	if(mLinkedWeb){
		mLinkedWeb->setAuthCallback(nullptr);
		mLinkedWeb->setLoadingUpdatedCallback(nullptr);
	}

	mLinkedWeb = linkedWeb;

	if(mLinkedWeb){
		mLinkedWeb->setAuthCallback([this](ds::ui::Web::AuthCallback callback){
			startAuthCallback(callback.mHost, callback.mRealm);
		});

		mLinkedWeb->setLoadingUpdatedCallback([this](const bool isLoading) {
			updateWidgets();
		});
	}

	updateWidgets();
}

// TODO: make this into a layoutsprite
// Layout is called when the size is changed, so don't change the size in the layout
void WebInterface::onLayout(){
	const float w = getWidth();
	const float h = getHeight();
	if(mKeyboardButton && mBackButton && mForwardButton && mRefreshButton && mTouchToggle){
		const float padding = h / 4.0f;

		float componentsWidth = (
			mKeyboardButton->getScaleWidth() + padding +
			mBackButton->getScaleWidth() + padding +
			mForwardButton->getScaleWidth() + padding +
			mRefreshButton->getScaleWidth() + padding +
			mTouchToggle->getScaleWidth()
			);

		if(!mKeyboardAllowed){
			componentsWidth -= (mKeyboardButton->getScaleWidth() + padding);
		}

		if(!mAbleToTouchToggle){
			componentsWidth -= (mTouchToggle->getScaleWidth() + padding);
		}

		float margin = ((w - componentsWidth) * 0.5f);
		float xp = margin;

		if(mKeyboardAllowed){
			mKeyboardButton->setPosition(xp, (h * 0.5f) - mKeyboardButton->getScaleHeight() / 2.0f);
			xp += mKeyboardButton->getScaleWidth() + padding;
		}

		mBackButton->setPosition(xp, (h * 0.5f) - mBackButton->getScaleHeight() / 2.0f);
		xp += mBackButton->getScaleWidth() + padding;

		mForwardButton->setPosition(xp, (h * 0.5f) - mForwardButton->getScaleHeight() / 2.0f);
		xp += mForwardButton->getScaleWidth() + padding;

		mRefreshButton->setPosition(xp, (h * 0.5f) - mRefreshButton->getScaleHeight() / 2.0f);
		xp += mRefreshButton->getScaleWidth() + padding;

		if(mAbleToTouchToggle){
			mTouchToggle->setPosition(xp, (h * 0.5f) - mTouchToggle->getScaleHeight() / 2.0f);
			xp += mTouchToggle->getScaleWidth() + padding;
		}
	}

	if(mKeyboardArea){
		// center the keyboard area above this sprite
		const float keyboardW = mKeyboardArea->getWidth();
		const float keyboardH = mKeyboardArea->getHeight();
		float yp = h;
		if(mKeyboardAbove) yp = -keyboardH;

		mKeyboardArea->setPosition((w - keyboardW) * 0.5f, yp);

		if(mAuthLayout){
			mAuthLayout->setSize(keyboardW, mAuthLayout->getHeight());
			mAuthLayout->runLayout();
			mAuthLayout->setPosition(mKeyboardArea->getPosition().x, yp - mAuthLayout->getHeight());
		}
	}
}

void WebInterface::updateWidgets(){
	if(mLinkedWeb && mForwardButton && mBackButton && mTouchToggle){
		// TODO: settings / config for disabled opacity / color
		if(mLinkedWeb->canGoBack()){
			mBackButton->enable(true);
			mBackButton->setOpacity(1.0f);
		} else {
			mBackButton->enable(false);
			mBackButton->setOpacity(0.25f);
		}
		if(mLinkedWeb->canGoForward()){
			mForwardButton->enable(true);
			mForwardButton->setOpacity(1.0f);
		} else {
			mForwardButton->enable(false);
			mForwardButton->setOpacity(0.25f);
		}

		if(mLinkedWeb->isEnabled()){
			if(!mWebLocked){
				mTouchToggle->getHighImage().setImageFile("%APP%/data/images/media_interface/touch_locked.png", ds::ui::Image::IMG_CACHE_F);
				mTouchToggle->getNormalImage().setImageFile("%APP%/data/images/media_interface/touch_locked.png", ds::ui::Image::IMG_CACHE_F);
				mWebLocked = true;
			}
		} else {
			if(mWebLocked){
				mTouchToggle->getHighImage().setImageFile("%APP%/data/images/media_interface/touch_unlocked.png", ds::ui::Image::IMG_CACHE_F);
				mTouchToggle->getNormalImage().setImageFile("%APP%/data/images/media_interface/touch_unlocked.png", ds::ui::Image::IMG_CACHE_F);
				mWebLocked = false;
			}
		}
	}

	if(mKeyboardArea){
		if(mKeyboardShowing){
			if(mKeyboardAutoDisablesTimeout){
				setCanTimeout(false);
			}
			if(!mKeyboard){
				ds::ui::SoftKeyboardSettings sks;
				sks.mKeyScale = mKeyboardKeyScale;
				sks.mGraphicKeys = true;
				sks.mGraphicType = ds::ui::SoftKeyboardSettings::kBorder;
				sks.mGraphicRoundedCornerRadius = 5;
				mKeyboard = ds::ui::SoftKeyboardBuilder::buildFullKeyboard(mEngine, sks);
				mKeyboardArea->addChildPtr(mKeyboard);

				mKeyboardArea->setColor(mBackground->getColor());

				const float keyW = mKeyboard->getScaleWidth();
				const float keyH = mKeyboard->getScaleHeight();
				const float areaW = keyW + 30.0f;
				const float areaH = keyH + 30.0f;
				mKeyboardArea->setSize(areaW, areaH);
				mKeyboard->setPosition(
					(areaW - keyW) * 0.5f,
					(areaH - keyH) * 0.5f
				);

				mKeyboard->setKeyPressFunction([this](const std::wstring& character, ds::ui::SoftKeyboardDefs::KeyType keyType){
					if(mLinkedWeb){
						if(mAuthorizing && mUserField && mPasswordField && mAuthLayout){
							if(mUserField->getIsInFocus()){
								if(keyType == SoftKeyboardDefs::KeyType::kEnter || keyType == SoftKeyboardDefs::KeyType::kTab){
									mUserField->unfocus();
									mPasswordField->focus();
								} else {
									mUserField->keyPressed(character, keyType);
								}
							} else {
								if(keyType == SoftKeyboardDefs::KeyType::kEnter){
									mLinkedWeb->authCallbackContinue(ds::utf8_from_wstr(mUserField->getCurrentText()), ds::utf8_from_wstr(mPasswordField->getCurrentText()));
									authComplete();

								} else if(keyType == SoftKeyboardDefs::KeyType::kTab){
									mPasswordField->unfocus();
									mUserField->focus();
								} else {
									mPasswordField->keyPressed(character, keyType);
								}
							}
						} else {
							mLinkedWeb->keyPressed(character, keyType);
						}

					}
				});

				mEngine.registerEntryField(mLinkedWeb);

				layout();
			}

			if(!mKeyboardArea->visible()){
				mKeyboardArea->show();
				mKeyboardArea->tweenOpacity(0.98f, mAnimateDuration, 0.0f, ci::easeNone);
			}
		} else {
			if(mKeyboardAutoDisablesTimeout){
				setCanTimeout(true);
			}
			if(mKeyboardArea->visible()){
				if(mEngine.getRegisteredEntryField() == mLinkedWeb) {
					mEngine.registerEntryField(nullptr);
				}

				mKeyboardArea->tweenOpacity(0.0f, mAnimateDuration, 0.0f, ci::easeNone, [this](){
					mKeyboardArea->hide();
				});
			}
		}
	}

	layout();
}

void WebInterface::showKeyboard(bool show) {
	mKeyboardShowing = show;
	updateWidgets();
	if(mKeyboardStatusCallback) mKeyboardStatusCallback(mKeyboardShowing);
}

void WebInterface::toggleKeyboard() {
	showKeyboard(!mKeyboardShowing);
}

void WebInterface::toggleTouch() {
	if(mLinkedWeb) {
		if(mLinkedWeb->isEnabled()) {
			stopTouch();
		} else {
			startTouch();
		}
	}
}

void WebInterface::startTouch() {
	if(mLinkedWeb) {
		mLinkedWeb->enable(true);
	}
	updateWidgets();

}

void WebInterface::stopTouch() {
	if(mLinkedWeb) {
		mLinkedWeb->enable(false);
	}
	updateWidgets();

}

} // namespace ui
} // namespace ds
