#include "stdafx.h"

#include "youtube_player.h"


#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/util/ui_utils.h>
#include <ds/util/string_util.h>

#include <ds/ui/sprite/web.h>

#include "ds/ui/media/interface/youtube_interface.h"
#include "ds/ui/media/media_interface_builder.h"
#include "ds/ui/media/media_viewer_settings.h"

namespace ds {
namespace ui {

YouTubeWeb::YouTubeWeb(ds::ui::SpriteEngine& eng)
	: ds::ui::Web(eng)
{
	// TODO: change this to a system that uses cookies? see cef_cookie.h and https://magpcss.org/ceforum/viewtopic.php?f=6&t=16655
	setTitleChangedFn([this](const std::wstring& titley) {
		auto theTitle = titley; // the title string can get changed on another thread, so copy it
		if(theTitle.find(L"cur=") == 0){
			auto durPos = theTitle.find(L"dur=");
			auto titPos = theTitle.find(L"title=");
			auto volPos = theTitle.find(L"vol=");
			auto statePos = theTitle.find(L"state=");
			auto posStr = theTitle.substr(4, durPos - 4);
			auto durStr = theTitle.substr(durPos + 4, volPos - durPos - 4);
			auto volStr = theTitle.substr(volPos + 4, statePos - volPos - 4);
			mCurrentPosition = ds::wstring_to_double(posStr);
			mDuration = ds::wstring_to_double(durStr);
			mVolume = ds::wstring_to_float(volStr) / 100.0f; // youtube uses 0.0 - 100.0, we use 0.0 - 1.0

			/* states:
				-1 – unstarted
				0 – ended
				1 – playing
				2 – paused
				3 – buffering
				5 – video cued
				*/
			mPlaying = theTitle.substr(statePos + 6, titPos - statePos - 6) == L"1";
			auto newTitle = ds::utf8_from_wstr(theTitle.substr(titPos + 6));
			if(newTitle != mVideoTitle){
				mVideoTitle = newTitle;
				if (mTitleChangedCallback) mTitleChangedCallback(mVideoTitle);
			}
		}
	});
}

void YouTubeWeb::setResource(const ds::Resource& resource) {
	setVideoId(resource.getFileName());
}

void YouTubeWeb::setMedia(const std::string& mediaPath) {
	setResource(ds::Resource(mediaPath));
}

void YouTubeWeb::setVideoId(const std::string& videoId) {
	mPlaying = false;
	//std::string url = "https://www.youtube.com/embed/" + resource.getFileName();
	std::string url = ds::Environment::expand("%APP%/data/html/youtube.html?v=" + videoId + "&a=" + std::to_string(mAutoStart));

	loadUrl(url);
}

void YouTubeWeb::play() {
	executeJavascript("play();");
}

void YouTubeWeb::pause() {
	executeJavascript("pause();");
}

void YouTubeWeb::stop() {
	executeJavascript("stop();");
}

void YouTubeWeb::togglePlayPause() {
	if(mPlaying){
		pause();
	} else {
		play();
	}
}

void YouTubeWeb::toggleMute() {
	executeJavascript("toggleMute();");
}

void YouTubeWeb::seekPercent(const float percenty) {
	executeJavascript("seekPercent(\"" + std::to_string(percenty) + "\");");
}


float YouTubeWeb::getCurrentPercent() {
	if (mDuration < 0.00001) return 0.0f;
	return (float)mCurrentPosition / mDuration;
}

float YouTubeWeb::getVolume() {
	return mVolume;
}

void YouTubeWeb::setVolume(const float theVolume) {
	float newVolume = theVolume;
	if (theVolume < 0.0f) newVolume = 0.0f;
	if (theVolume > 1.0f) newVolume = 1.0f;
	executeJavascript("setVolume(\"" + std::to_string(newVolume * 100.0f) + "\");");
	mVolume = newVolume; // we just assume everything went to plan
}

YouTubePlayer::YouTubePlayer(ds::ui::SpriteEngine& eng, const bool embedInterface)
	: ds::ui::Sprite(eng)
	, mYouTubeWeb(nullptr)
	, mYoutubeInterface(nullptr)
	, mEmbedInterface(embedInterface)
	, mShowInterfaceAtStart(true)
	, mKeyboardKeyScale(1.0f)
	, mAllowTouchToggle(true)
	, mStartInteractable(false)
	, mLetterbox(true)
	, mInterfaceBelowMedia(false)
	, mVolume(1.0f)
{
	mLayoutFixedAspect = true;
	enable(false);
}

void YouTubePlayer::setMediaViewerSettings(const MediaViewerSettings& settings) {
	setYouTubeSize(settings.mYouTubeSize);
	setLetterbox(settings.mLetterBox);
	setAllowTouchToggle(settings.mWebAllowTouchToggle);
	setShowInterfaceAtStart(settings.mShowInterfaceAtStart);
	setStartInteractable(settings.mWebStartTouchable);
	setNativeTouches(settings.mWebNativeTouches);
	mInterfaceBelowMedia = settings.mInterfaceBelowMedia;
	mInterfaceBottomPad = settings.mInterfaceBottomPad;
	mAutoStart = !settings.mVideoAutoPlayFirstFrame;
	mVolume = settings.mVideoVolume;
}


void YouTubePlayer::setYouTubeSize(const ci::vec2 youTubeSize) {
	mYouTubeSize = youTubeSize;
	if (mYouTubeWeb) {
		mYouTubeWeb->setSize(mYouTubeSize.x, mYouTubeSize.y);
	}

}
void YouTubePlayer::setAllowTouchToggle(const bool allowTouchToggle) {
	mAllowTouchToggle = allowTouchToggle;
	if(mYoutubeInterface){
		mYoutubeInterface->setAllowTouchToggle(allowTouchToggle);
	}
}

void YouTubePlayer::setMedia(const std::string mediaPath) {
	setResource(ds::Resource(mediaPath, ds::Resource::WEB_TYPE));
}

void YouTubePlayer::setResource(const ds::Resource& resource) {
	static const float fractionalWidthForContent = 0.6f;


	if (mYouTubeWeb) {
		if (mYoutubeInterface) {
			mYoutubeInterface->linkYouTubeWeb(nullptr);
		}

		mYouTubeWeb->release();
		mYouTubeWeb = nullptr;
	}

	mYouTubeWeb = new ds::ui::YouTubeWeb(mEngine);
	mYouTubeWeb->setDrawWhileLoading(true);

	mYouTubeWeb->setNativeTouchInput(mNativeTouches);

	mYouTubeWeb->setAddressChangedFn([this](const std::string& addy) {
		if (mYoutubeInterface) {
			mYoutubeInterface->updateWidgets();
		}
	});
	mYouTubeWeb->setAutoStart(mAutoStart);
	mYouTubeWeb->setResource(resource);

	float targetW = mYouTubeSize.x;
	float targetH = mYouTubeSize.y;

	if ((targetW == 0.0f) || (targetH == 0.0f)) {
		targetW = mEngine.getWorldWidth() * fractionalWidthForContent;
		targetH = mEngine.getWorldHeight();
	}

	setYouTubeSize(ci::vec2(targetW, targetH));
	setVolume(mVolume);

	addChildPtr(mYouTubeWeb);

	if (mStartInteractable) {
		mYouTubeWeb->enable(true);
	} else {
		mYouTubeWeb->enable(false);
	}

	if (mYoutubeInterface) {
		mYoutubeInterface->release();
		mYoutubeInterface = nullptr;
	}

	if (mEmbedInterface) {
		mYoutubeInterface = dynamic_cast<YoutubeInterface*>(MediaInterfaceBuilder::buildMediaInterface(mEngine, this, this));
		
		if (mYoutubeInterface) {
			setAllowTouchToggle(mAllowTouchToggle);
			mYoutubeInterface->sendToFront();
		}
	}

	if (mYoutubeInterface) {
		if (mShowInterfaceAtStart) {
			mYoutubeInterface->show();
		} else {
			mYoutubeInterface->setOpacity(0.0f);
			mYoutubeInterface->hide();
		}
	}

	setSize(mYouTubeWeb->getWidth(), mYouTubeWeb->getHeight());
}

void YouTubePlayer::onSizeChanged() {
	layout();
}

void YouTubePlayer::layout() {
	if (mYouTubeWeb) {
		fitInside(mYouTubeWeb, ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()), mLetterbox);
	}

	if (mYoutubeInterface && mEmbedInterface) {
		mYoutubeInterface->setSize(getWidth() * 2.0f / 3.0f, mYoutubeInterface->getHeight());

		float yPos = getHeight() - mYoutubeInterface->getScaleHeight() - mInterfaceBottomPad;
		if (yPos < getHeight() / 2.0f) yPos = getHeight() / 2.0f;
		if (yPos + mYoutubeInterface->getScaleHeight() > getHeight()) yPos = getHeight() - mYoutubeInterface->getScaleHeight();
		if (mInterfaceBelowMedia) yPos = getHeight();
		mYoutubeInterface->setPosition(getWidth() / 2.0f - mYoutubeInterface->getScaleWidth() / 2.0f, yPos);
	}
}

void YouTubePlayer::play() {
	if(mYouTubeWeb){
		mYouTubeWeb->play();
	}
}

void YouTubePlayer::pause() {
	if (mYouTubeWeb) {
		mYouTubeWeb->pause();
	}
}

void YouTubePlayer::stop() {
	if (mYouTubeWeb) {
		mYouTubeWeb->stop();
	}
}

void YouTubePlayer::togglePlayPause() {
	if (mYouTubeWeb) {
		mYouTubeWeb->togglePlayPause();
	}
}

void YouTubePlayer::toggleMute() {
	if (mYouTubeWeb) {
		mYouTubeWeb->toggleMute();
	}
}

void YouTubePlayer::userInputReceived() {
	ds::ui::Sprite::userInputReceived();
	showInterface();
}

void YouTubePlayer::showInterface() {
	if (mYoutubeInterface) {
		mYoutubeInterface->animateOn();
	}
}

void YouTubePlayer::hideInterface() {
	if (mYoutubeInterface) {
		mYoutubeInterface->startIdling();
	}
}

void YouTubePlayer::setShowInterfaceAtStart(const bool showInterfaceAtStart) {
	mShowInterfaceAtStart = showInterfaceAtStart;
}

void YouTubePlayer::setStartInteractable(const bool startInteractable) {
	mStartInteractable = startInteractable;
}

void YouTubePlayer::setLetterbox(const bool doLetterbox) {
	mLetterbox = doLetterbox;
	layout();
}

void YouTubePlayer::setNativeTouches(const bool isNative) {
	mNativeTouches = isNative;
	if (mYouTubeWeb) {
		mYouTubeWeb->setNativeTouchInput(mNativeTouches);
	}
}

void YouTubePlayer::setVolume(const float volume) {
	mVolume = volume;
	if(mYouTubeWeb){
		mYouTubeWeb->setVolume(volume);
	}
}

void YouTubePlayer::sendClick(const ci::vec3& globalClickPos) {
	if (mYouTubeWeb) {
		mYouTubeWeb->sendMouseClick(globalClickPos);
	}
}

ds::ui::YouTubeWeb* YouTubePlayer::getYouTubeWeb() {
	return mYouTubeWeb;
}

YoutubeInterface * YouTubePlayer::getYoutubeInterface() {
	return mYoutubeInterface;
}
}  // namespace ui
}  // namespace ds
