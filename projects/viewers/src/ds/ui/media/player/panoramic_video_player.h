#pragma once

#include <ds/ui/sprite/sprite.h>
#include <gstreamer/gstreamer_audio_device.h>

namespace ds {

class Resource;

namespace ui {

class GstVideo;
class PanoramicVideo;
class VideoInterface;
class MediaInterface;
struct MediaViewerSettings;

/**
* \class PanoramicVideoPlayer
*			Creates a panoramicvideo and puts an interface on top of it.
*/
class PanoramicVideoPlayer : public ds::ui::Sprite {
public:
	PanoramicVideoPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface = true);

	virtual void						setResource(const ds::Resource& resource) override;
	void								setMedia(const std::string mediaPath);
	void								clear();

	void								layout();

	void								play();
	void								pause();
	void								stop();
	void								togglePlayPause();
	void								toggleMute();

	void								showInterface();
	void								hideInterface();
	VideoInterface*						getVideoInterface() { return mVideoInterface; }
	void								setShowInterfaceAtStart(bool showInterfaceAtStart);

	ds::ui::PanoramicVideo*				getPanoramicVideo();
	ds::ui::GstVideo*					getVideo();

	void								setGoodStatusCallback(std::function<void()> func) { mGoodStatusCallback = func; }
	void								setErrorCallback(std::function<void(const std::string&)> func) { mErrorMsgCallback = func; }
	void								setVideoCompleteCallback(std::function<void()> func) { mVideoCompleteCallback = func; }

	/// Sets all applicable settings from a MediaViewerSettings
	void setMediaViewerSettings(MediaViewerSettings& settings);

	/// See the function of the same name on GstVideo, Set any time, will remember between loading videos
	void								setPan(const float newPan);

	/// See the function of the same name on GstVideo, Set any time, will remember between loading videos
	void								setVolume(const float volume);

	/// See the function of the same name on GstVideo, Whether to synchronize across client/servers, default = true
	void								setAutoSynchronize(const bool doSync);

	/// See the function of the same name on GstVideo
	void								setPlayableInstances(const std::vector<std::string> instanceNames);

	/// Automatically plays the first frame of the video so you can see what the video is.
	/// Must be set before playing the video
	/// Default = true
	void								setAutoPlayFirstFrame(const bool playFirstFrame);

	/// If the video goes out-of-bounds, whether to auto-mute or not
	void								allowOutOfBoundsMuted(const bool allowMuting);

	/// Sets looping on the video
	void								setVideoLoop(const bool doLooping);

	/// Sets the audio devices for playback (see gstreamer_audio_device header for more info)
	void								setAudioDevices(std::vector<GstAudioDevice>& audioDevices);

protected:

	virtual void								onSizeChanged();
	VideoInterface*								mVideoInterface;
	ds::ui::PanoramicVideo*						mPanoramicVideo;
	ds::ui::GstVideo*							mVideo;
	bool										mEmbedInterface;
	bool										mShowInterfaceAtStart;
	std::function<void(void)>					mGoodStatusCallback;
	std::function<void(void)>					mVideoCompleteCallback;
	std::function<void(const std::string&)>		mErrorMsgCallback;

	/// Settings - these are kept locally here so the settings can be applied at any time
	float										mPanning;
	float										mVolume;
	bool										mAutoSyncronize;
	bool										mAutoPlayFirstFrame;
	bool										mAllowOutOfBoundsMuted;
	std::vector<std::string>					mPlayableInstances;
	std::vector<GstAudioDevice>					mAudioDevices;
	bool										mLooping;
	bool										mInterfaceBelowMedia;
	float										mInterfaceBottomPad = 50.0f;
};

} // namespace ui
} // namespace ds

