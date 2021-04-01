#include "stdafx.h"

#include "panoramic_video_player.h"


#include <ds/app/environment.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/debug/logger.h>
#include <ds/util/string_util.h>

#include <ds/ui/sprite/video.h>
#include <ds/ui/sprite/panoramic_video.h>
#include <ds/ui/button/image_button.h>

#include "ds/ui/media/interface/video_interface.h"
#include "ds/ui/media/media_interface_builder.h"
#include "ds/ui/media/media_viewer_settings.h"

namespace ds {
namespace ui {

PanoramicVideoPlayer::PanoramicVideoPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface)
	: ds::ui::Sprite(eng)
	, mVideo(nullptr)
	, mPanoramicVideo(nullptr)
	, mVideoInterface(nullptr)
	, mEmbedInterface(embedInterface)
	, mInterfaceBelowMedia(false)
	, mShowInterfaceAtStart(true)
	, mAutoSyncronize(true)
	, mAutoPlayFirstFrame(true)
	, mAllowOutOfBoundsMuted(true)
	, mPanning(0.0f)
	, mVolume(1.0f)
	, mLooping(true)
{
	mLayoutFixedAspect = true;
}

void PanoramicVideoPlayer::setMedia(const std::string mediaPath) {
	setResource(ds::Resource(mediaPath));
}

void PanoramicVideoPlayer::setResource(const ds::Resource& resource) {

	clear();

	mPanoramicVideo = new ds::ui::PanoramicVideo(mEngine);
	addChildPtr(mPanoramicVideo);
	mPanoramicVideo->setSize(1920.0f, 1080.0f);
	mPanoramicVideo->loadVideo(resource.getAbsoluteFilePath());
	mVideo = mPanoramicVideo->getVideo();

	if(!mVideo) {
		DS_LOG_WARNING("Video not loaded in panoramic video player!");
		mPanoramicVideo->release();
		mPanoramicVideo = nullptr;
		return;
	}
	mVideo->generateAudioBuffer(true);
	mVideo->setLooping(true);

	mVideo->setVideoCompleteCallback([this] {


		mVideo->seekPosition(0);

		if(!mLooping) {
			mVideo->pause();
		}

		// show the interface if we have one
		if(mVideoInterface) {
			mVideoInterface->userInputReceived();
		}

		if(mVideoCompleteCallback) {
			mVideoCompleteCallback();
		}
	});

	mPanoramicVideo->setPanoTappedCallback([this]{
		showInterface();
	});

	setPan(mPanning);
	setVolume(mVolume);
	setAutoSynchronize(mAutoSyncronize);
	setPlayableInstances(mPlayableInstances);
	allowOutOfBoundsMuted(mAllowOutOfBoundsMuted);
	setVideoLoop(mLooping);
	setAudioDevices(mAudioDevices);

	mVideo->setErrorCallback([this](const std::string& msg) {
		if(mErrorMsgCallback) mErrorMsgCallback(msg);
	});

	mVideo->setStatusCallback([this](const ds::ui::GstVideo::Status& status) {
		bool isGood = status == ds::ui::GstVideo::Status::STATUS_PLAYING;
		if(mGoodStatusCallback) {
			mGoodStatusCallback();
		}
	});

	if(mAutoPlayFirstFrame) {
		mVideo->setMute(true);
		mVideo->setAutoStart(false);
	} else {
		mVideo->setAutoStart(true);
	}

	if(mAutoPlayFirstFrame) {
		mVideo->playAFrame(-1.0, [this]() {
			mVideo->setMute(false);
		});
	}

	if(mVideoInterface) {
		mVideoInterface->release();
		mVideoInterface = nullptr;
	}

	if(mEmbedInterface) {
		mVideoInterface = dynamic_cast<VideoInterface*>(MediaInterfaceBuilder::buildMediaInterface(mEngine, this, this));
		if(mVideoInterface) {
			mVideoInterface->sendToFront();
		}
	}

	if(mVideoInterface) {
		if(mShowInterfaceAtStart) {
			mVideoInterface->show();
		} else {
			mVideoInterface->setOpacity(0.0f);
			mVideoInterface->hide();
		}
	}

	if(mVideo->getWidth() < 1.0f || mVideo->getHeight() < 1.0f) {
		// make this a setting? This is mostly for when the "video" is just an audio track
		// Probably should detect this properly from GstVideo and expose it to the outside world. Users may want to know that this only has audio
		setSize(600.0f, 100.0f);
	} else {
		setSize(mPanoramicVideo->getWidth(), mPanoramicVideo->getHeight());
	}
}

void PanoramicVideoPlayer::clear() {
	if(mPanoramicVideo) {
		mPanoramicVideo->release();
		mPanoramicVideo = nullptr;
		mVideo = nullptr;
		if(mVideoInterface) {
			mVideoInterface->linkVideo(nullptr);
		}
	}
}

void PanoramicVideoPlayer::onSizeChanged() {
	layout();
}

void PanoramicVideoPlayer::layout() {
	if(mPanoramicVideo) {
		mPanoramicVideo->setSize(getWidth(), getHeight());
	}

	if(mVideoInterface && mEmbedInterface) {
		mVideoInterface->setSize(getWidth() / 2.0f, mVideoInterface->getHeight());
		float yPos = getHeight() - mVideoInterface->getScaleHeight() - mInterfaceBottomPad;
		if(yPos < getHeight() / 2.0f) yPos = getHeight() / 2.0f;
		if(yPos + mVideoInterface->getScaleHeight() > getHeight()) yPos = getHeight() - mVideoInterface->getScaleHeight();
		if(mInterfaceBelowMedia) yPos = getHeight();
		mVideoInterface->setPosition(getWidth() / 2.0f - mVideoInterface->getScaleWidth() / 2.0f, yPos);
	}
}

void PanoramicVideoPlayer::showInterface() {
	if(mVideoInterface) {
		mVideoInterface->animateOn();
	}
}

void PanoramicVideoPlayer::hideInterface() {
	if(mVideoInterface) {
		mVideoInterface->startIdling();
	}
}

void PanoramicVideoPlayer::setShowInterfaceAtStart(bool showInterfaceAtStart) {
	mShowInterfaceAtStart = showInterfaceAtStart;
}

ds::ui::PanoramicVideo* PanoramicVideoPlayer::getPanoramicVideo() {
	return mPanoramicVideo;
}

void PanoramicVideoPlayer::play() {
	if(mVideo) {
		if(mVideo->isPlayingAFrame()) {
			mVideo->enablePlayingAFrame(false);
			mVideo->setMute(false);
		}
		mVideo->play();
	}
}

void PanoramicVideoPlayer::pause() {
	if(mVideo) {
		mVideo->pause();
	}
}

void PanoramicVideoPlayer::stop() {
	if(mVideo) {
		mVideo->stop();
	}
}

ds::ui::GstVideo* PanoramicVideoPlayer::getVideo() {
	return mVideo;
}

void PanoramicVideoPlayer::togglePlayPause() {
	if(mVideo) {
		if(mVideo->getIsPlaying()) {
			mVideo->pause();
		} else {
			mVideo->play();
		}
	}
}

void PanoramicVideoPlayer::toggleMute() {
	if(mVideo) {
		if(mVideo->getIsMuted()) {
			mVideo->setMute(false);
		} else {
			mVideo->setMute(true);
		}
	}
}

void PanoramicVideoPlayer::setMediaViewerSettings(MediaViewerSettings& settings){
	setPan(settings.mVideoPanning);
	setVolume(settings.mVideoVolume);
	setAutoSynchronize(settings.mVideoAutoSync);
	setPlayableInstances(settings.mVideoPlayableInstances);
	setAutoPlayFirstFrame(settings.mVideoAutoPlayFirstFrame);
	setVideoLoop(settings.mVideoLoop);
	setShowInterfaceAtStart(settings.mShowInterfaceAtStart);
	setAudioDevices(settings.mVideoAudioDevices);
	mInterfaceBelowMedia = settings.mInterfaceBelowMedia;
	mInterfaceBottomPad = settings.mInterfaceBottomPad;
}

void PanoramicVideoPlayer::setPan(const float newPan) {
	if(mVideo) {
		mVideo->setPan(newPan);
	}

	mPanning = newPan;
}

void PanoramicVideoPlayer::setVolume(const float volume) {
	if(mVideo) {
		mVideo->setVolume(volume);
	}

	mVolume = volume;
}

void PanoramicVideoPlayer::setAutoSynchronize(const bool doSync) {
	if(mVideo) {
		mVideo->setAutoSynchronize(doSync);
	}

	mAutoSyncronize = doSync;
}

void PanoramicVideoPlayer::setAutoPlayFirstFrame(const bool playFirstFrame) {
	mAutoPlayFirstFrame = playFirstFrame;
}

void PanoramicVideoPlayer::setPlayableInstances(const std::vector<std::string> instanceNames) {
	if(mVideo) {
		mVideo->setPlayableInstances(instanceNames);
	}

	mPlayableInstances = instanceNames;
}

void PanoramicVideoPlayer::allowOutOfBoundsMuted(const bool allowMuting) {
	mAllowOutOfBoundsMuted = allowMuting;
	if(mVideo) {
		mVideo->setAllowOutOfBoundsMuted(mAllowOutOfBoundsMuted);
	}
}

void PanoramicVideoPlayer::setVideoLoop(const bool doLoop) {
	mLooping = doLoop;
	if(mVideo) {
		mVideo->setLooping(mLooping);
	}
}

void PanoramicVideoPlayer::setAudioDevices(std::vector<GstAudioDevice>& audioDevices) {
	mAudioDevices = audioDevices;
	if(mVideo) {
		mVideo->setAudioDevices(audioDevices);
	}
}

} // namespace ui
} // namespace ds
