#include "stdafx.h"

#include "stream_player.h"


#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/util/ui_utils.h>
#include <ds/util/string_util.h>

#include <ds/ui/button/image_button.h>
#include <ds/ui/sprite/video.h>

#include "ds/ui/media/interface/video_interface.h"
#include "ds/ui/media/media_interface_builder.h"
#include "ds/ui/media/media_viewer_settings.h"

namespace ds {
namespace ui {

StreamPlayer::StreamPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface)
  : ds::ui::Sprite(eng)
  , mVideo(nullptr)
  , mVideoInterface(nullptr)
  , mEmbedInterface(embedInterface)
  , mShowInterfaceAtStart(true)
  , mIsPlaying(false)
  , mLetterbox(true)
  , mInterfaceBelowMedia(false)
  , mVolume(1.0f) {
	mLayoutFixedAspect = true;
}

void StreamPlayer::setResource(const ds::Resource& resource) {

	if (mVideo) {
		mVideo->release();
		mVideo = nullptr;
		if (mVideoInterface) {
			mVideoInterface->linkVideo(nullptr);
		}
	}

	mVideo = new ds::ui::GstVideo(mEngine);
	mVideo->generateAudioBuffer(true);
	mVideo->setLooping(true);

	mVideo->setErrorCallback([this](const std::string& msg) {
		if (mErrorMsgCallback) mErrorMsgCallback(msg);
	});

	mVideo->setStatusCallback([this](const ds::ui::GstVideo::Status& status) {
		// bool isGood = status == ds::ui::GstVideo::Status::STATUS_PLAYING;
		// if(mGoodStatusCallback){
		//	mGoodStatusCallback();
		//}
	});

	setVolume(mVolume);

	mVideo->setStreamingLatency(static_cast<uint64_t>(mStreamLatency * 1000000000.0));

	mVideo->setResource(resource);

	addChildPtr(mVideo);

	if (mVideoInterface) {
		mVideoInterface->release();
		mVideoInterface = nullptr;
	}

	if (mEmbedInterface) {
		mVideoInterface = dynamic_cast<VideoInterface*>(MediaInterfaceBuilder::buildMediaInterface(mEngine, this, this));
		if (mVideoInterface) {
			mVideoInterface->sendToFront();
		}
	}

	if (mVideoInterface) {
		if (mShowInterfaceAtStart) {
			mVideoInterface->show();
		} else {
			mVideoInterface->setOpacity(0.0f);
			mVideoInterface->hide();
		}

		if (resource.getType() == ds::Resource::VIDEO_STREAM_TYPE) {
			mVideoInterface->hide();
		}
	}

	setSize(mVideo->getWidth(), mVideo->getHeight());
}

void StreamPlayer::onUpdateServer(const ds::UpdateParams& updateParams) {
	if (mGoodStatusCallback && mVideo) {
		bool isPlaying = mVideo->getVideoPlayingFramerate() > 1.0f;
		if (isPlaying != mIsPlaying) {
			if (isPlaying) {
				mGoodStatusCallback();
			}
			mIsPlaying = isPlaying;
		}
	}
}

void StreamPlayer::onSizeChanged() { layout(); }

void StreamPlayer::layout() {
	if (mVideo) {
		if (mVideo->getWidth() > 0.0f) {
			fitInside(mVideo, ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()), mLetterbox);
		}
	}

	if (mVideoInterface && mEmbedInterface) {
		mVideoInterface->setSize(getWidth() / 2.0f, mVideoInterface->getHeight());

		float yPos = getHeight() - mVideoInterface->getScaleHeight() - mInterfaceBottomPad;
		if (yPos < getHeight() / 2.0f) yPos = getHeight() / 2.0f;
		if(yPos + mVideoInterface->getScaleHeight() > getHeight()) yPos = getHeight() - mVideoInterface->getScaleHeight();
		if(mInterfaceBelowMedia) yPos = getHeight();
		mVideoInterface->setPosition(getWidth() / 2.0f - mVideoInterface->getScaleWidth() / 2.0f, yPos);
	}
}

void StreamPlayer::showInterface() {
	if (mVideoInterface) {
		mVideoInterface->animateOn();
	}
}

void StreamPlayer::hideInterface() {
	if (mVideoInterface) {
		mVideoInterface->startIdling();
	}
}

void StreamPlayer::setMediaViewerSettings(const MediaViewerSettings& settings) {
	setVolume(settings.mVideoVolume);
	setShowInterfaceAtStart(settings.mShowInterfaceAtStart);
	setLetterbox(settings.mLetterBox);
	setStreamLatency(settings.mVideoStreamingLatency);
	mInterfaceBelowMedia = settings.mInterfaceBelowMedia;
	mInterfaceBottomPad = settings.mInterfaceBottomPad;
}

void StreamPlayer::setShowInterfaceAtStart(bool showInterfaceAtStart) { mShowInterfaceAtStart = showInterfaceAtStart; }

void StreamPlayer::setAutoRestartStream(bool autoRestart) {
	if (mVideo) {
		mVideo->setAutoRestartStream(autoRestart);
	}
}

void StreamPlayer::setLetterbox(const bool doLetterbox) {
	mLetterbox = doLetterbox;
	layout();
}

void StreamPlayer::setStreamLatency(const double latencyInSeconds) {
	mStreamLatency = latencyInSeconds;
	if (mVideo) {
		mVideo->setStreamingLatency(static_cast<uint64_t>(latencyInSeconds * 1000000000.0));
	}
}

void StreamPlayer::setVolume(const float volume) {
	if (mVideo) {
		mVideo->setVolume(volume);
	}

	mVolume = volume;
}

void StreamPlayer::play() {
	if (mVideo) {
		if (mVideo->isPlayingAFrame()) {
			mVideo->enablePlayingAFrame(false);
			mVideo->setMute(false);
		}
		mVideo->play();
	}
}

void StreamPlayer::pause() {
	if (mVideo) {
		mVideo->pause();
	}
}

void StreamPlayer::togglePlayPause() {
	if (mVideo) {
		if (mVideo->getIsPlaying()) {
			mVideo->pause();
		} else {
			mVideo->play();
		}
	}
}

void StreamPlayer::stop() {
	if (mVideo) {
		mVideo->stop();
	}
}

void StreamPlayer::toggleMute() {
	if (mVideo) {
		if (mVideo->getIsMuted()) {
			mVideo->setMute(false);
		} else {
			mVideo->setMute(true);
		}
	}
}

ds::ui::GstVideo* StreamPlayer::getVideo() { return mVideo; }

}  // namespace ui
}  // namespace ds
