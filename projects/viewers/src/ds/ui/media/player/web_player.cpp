#include "stdafx.h"

#include "web_player.h"


#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/util/ui_utils.h>
#include <ds/util/string_util.h>

#include <ds/ui/sprite/web.h>

#include "ds/ui/media/interface/web_interface.h"
#include "ds/ui/media/media_interface_builder.h"
#include "ds/ui/media/media_viewer_settings.h"

namespace ds {
namespace ui {

WebPlayer::WebPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface)
  : ds::ui::Sprite(eng)
  , mWeb(nullptr)
  , mWebInterface(nullptr)
  , mEmbedInterface(embedInterface)
  , mShowInterfaceAtStart(true)
  , mKeyboardKeyScale(1.0f)
  , mKeyboardAllow(true)
  , mKeyboardAbove(true)
  , mAllowTouchToggle(true)
  , mStartInteractable(false)
  , mLetterbox(true)
  , mInterfaceBelowMedia(false)
{

	mLayoutFixedAspect = true;
	enable(false);
}

void WebPlayer::setMediaViewerSettings(const MediaViewerSettings& settings) {
	setWebViewSize(settings.mWebDefaultSize);
	setLetterbox(settings.mLetterBox);
	setKeyboardParams(settings.mWebKeyboardKeyScale, settings.mWebAllowKeyboard, settings.mWebKeyboardAbove);
	setAllowTouchToggle(settings.mWebAllowTouchToggle);
	setShowInterfaceAtStart(settings.mShowInterfaceAtStart);
	setStartInteractable(settings.mWebStartTouchable);
	setNativeTouches(settings.mWebNativeTouches);
	mInterfaceBelowMedia = settings.mInterfaceBelowMedia;
	mInterfaceBottomPad = settings.mInterfaceBottomPad;
	mAutoStart = !settings.mVideoAutoPlayFirstFrame;
}

void WebPlayer::setWebViewSize(const ci::vec2 webSize) {
	mWebSize = webSize;
	if (mWeb) {
		mWeb->setSize(mWebSize.x, mWebSize.y);
	}
}

void WebPlayer::setKeyboardParams(const float keyboardKeyScale, const bool keyboardAllow, const bool keyboardAbove) {
	mKeyboardKeyScale = keyboardKeyScale;
	mKeyboardAllow	= keyboardAllow;
	mKeyboardAbove	= keyboardAbove;
	if (mWebInterface) {
		mWebInterface->setKeyboardKeyScale(keyboardKeyScale);
		mWebInterface->setKeyboardAllow(keyboardAllow);
		mWebInterface->setKeyboardAbove(mKeyboardAbove);
	}
}


void WebPlayer::setKeyboardStateCallback(std::function<void(const bool onscreen)> func) {
	mKeyboardStatusCallback = func;
}

void WebPlayer::setAllowTouchToggle(const bool allowTouchToggle) {
	mAllowTouchToggle = allowTouchToggle;
	if (mWebInterface) {
		mWebInterface->setAllowTouchToggle(mAllowTouchToggle);
	}
}

void WebPlayer::setMedia(const std::string mediaPath) {
	setResource(ds::Resource(mediaPath, ds::Resource::WEB_TYPE));
}

void WebPlayer::setResource(const ds::Resource& resource) {

	static const float fractionalWidthForContent = 0.6f;

	if (mWeb) {
		if (mWebInterface) {
			mWebInterface->linkWeb(nullptr);
		}

		mWeb->release();
		mWeb = nullptr;
	}

	mWeb = new ds::ui::Web(mEngine);
	mWeb->setDragScrolling(true);
	mWeb->setDragScrollingMinimumFingers(2);
	mWeb->setDrawWhileLoading(true);

	mWeb->setNativeTouchInput(mNativeTouches);

	mWeb->setAddressChangedFn([this](const std::string& addy) {
		if (mWebInterface) {
			mWebInterface->updateWidgets();
		}
	});

	float targetW = mWebSize.x;
	float targetH = mWebSize.y;

	if ((targetW == 0.0f) || (targetH == 0.0f)) {
		targetW = mEngine.getWorldWidth() * fractionalWidthForContent;
		targetH = mEngine.getWorldHeight();
	}

	setWebViewSize(ci::vec2(targetW, targetH));

	addChildPtr(mWeb);

	setTransparent(false);
	setColor(ci::Color::white());
	mWeb->setResource(resource);	

	if (mStartInteractable) {
		mWeb->enable(true);
	} else {
		mWeb->enable(false);
	}

	if (mWebInterface) {
		mWebInterface->release();
		mWebInterface = nullptr;
	}
	if (mEmbedInterface) {
		mWebInterface = dynamic_cast<WebInterface*>(MediaInterfaceBuilder::buildMediaInterface(mEngine, this, this));

		if (mWebInterface) {
			setKeyboardParams(mKeyboardKeyScale, mKeyboardAllow, mKeyboardAbove);
			setAllowTouchToggle(mAllowTouchToggle);
			mWebInterface->setKeyboardStateCallback([this](const bool onscreen) {
				if (mKeyboardStatusCallback) {
					mKeyboardStatusCallback(onscreen);
				}
			});
			mWebInterface->sendToFront();
		}
	}


	if (mWebInterface) {
		if (mShowInterfaceAtStart) {
			mWebInterface->show();
		} else {
			mWebInterface->setOpacity(0.0f);
			mWebInterface->hide();
		}
	}

	setSize(mWeb->getWidth(), mWeb->getHeight());
}

void WebPlayer::onSizeChanged() { layout(); }

void WebPlayer::layout() {
	if(mWeb) {
		fitInside(mWeb, ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()), mLetterbox);
	}

	if(mWebInterface && mEmbedInterface) {
		mWebInterface->setSize(getWidth() * 2.0f / 3.0f, mWebInterface->getHeight());

		float yPos = getHeight() - mWebInterface->getScaleHeight() - mInterfaceBottomPad;
		if(yPos < getHeight() / 2.0f) yPos = getHeight() / 2.0f;
		if(yPos + mWebInterface->getScaleHeight() > getHeight()) yPos = getHeight() - mWebInterface->getScaleHeight();
		if(mInterfaceBelowMedia) yPos = getHeight();
		mWebInterface->setPosition(getWidth() / 2.0f - mWebInterface->getScaleWidth() / 2.0f, yPos);
	}
}

void WebPlayer::userInputReceived() {
	ds::ui::Sprite::userInputReceived();
	showInterface();
}

void WebPlayer::showInterface() {
	if(mWebInterface) {
		mWebInterface->animateOn();
	}
}

void WebPlayer::hideInterface() {
	if(mWebInterface) {
		mWebInterface->startIdling();
	}
}

void WebPlayer::setShowInterfaceAtStart(const bool showInterfaceAtStart) { mShowInterfaceAtStart = showInterfaceAtStart; }

void WebPlayer::setStartInteractable(const bool startInteractable) { mStartInteractable = startInteractable; }

void WebPlayer::setLetterbox(const bool doLetterbox) {
	mLetterbox = doLetterbox;
	layout();
}

void WebPlayer::setNativeTouches(const bool isNative) {
	mNativeTouches = isNative;
	if(mWeb){
		mWeb->setNativeTouchInput(mNativeTouches);
	}
}

void WebPlayer::sendClick(const ci::vec3& globalClickPos) {
	if (mWeb) {
		mWeb->sendMouseClick(globalClickPos);
	}
}

ds::ui::Web* WebPlayer::getWeb() { return mWeb; }


ds::ui::WebInterface* WebPlayer::getWebInterface(){
	return mWebInterface;
}
}  // namespace ui
}  // namespace ds
