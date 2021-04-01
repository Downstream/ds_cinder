#pragma once
#ifndef DS_UI_MEDIA_STREAM_PLAYER_STREAM_PLAYER
#define DS_UI_MEDIA_STREAM_PLAYER_STREAM_PLAYER


#include <ds/ui/sprite/sprite.h>

namespace ds {

class Resource;

namespace ui {

class GstVideo;
class VideoInterface;
class MediaInterface;
struct MediaViewerSettings;

/**
 * \class VideoPlayer
 *			Creates a video and puts an interface on top of it.
 */
class StreamPlayer : public ds::ui::Sprite {
  public:
	StreamPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface = true);

	virtual void setResource(const ds::Resource& resource) override;

	void layout();

	void play();
	void pause();
	void togglePlayPause();
	void stop();
	void toggleMute();

	void			showInterface();
	void			hideInterface();
	VideoInterface* getVideoInterface() { return mVideoInterface; }

	/// Sets all applicable settings from a MediaViewerSettings
	void setMediaViewerSettings(const MediaViewerSettings& settings);

	void setShowInterfaceAtStart(bool showInterfaceAtStart);
	void setAutoRestartStream(bool autoRestart);
	void setLetterbox(const bool doLetterbox);

	void setStreamLatency(const double latencyInSeconds);

	/// See the function of the same name on GstVideo, Set any time, will remember between loading videos
	void setVolume(const float volume);

	ds::ui::GstVideo* getVideo();

	void setGoodStatusCallback(std::function<void()> func) { mGoodStatusCallback = func; }
	void setErrorCallback(std::function<void(const std::string&)> func) { mErrorMsgCallback = func; }

  protected:
	virtual void							onUpdateServer(const ds::UpdateParams& updateParams) override;
	virtual void							onSizeChanged() override;
	VideoInterface*							mVideoInterface;
	ds::ui::GstVideo*						mVideo;
	float									mVolume;
	bool									mIsPlaying;
	bool									mEmbedInterface;
	bool									mShowInterfaceAtStart;
	bool									mInterfaceBelowMedia;
	float									mInterfaceBottomPad = 50.0f;
	bool									mLetterbox;
	double									mStreamLatency;
	std::function<void(void)>				mGoodStatusCallback;
	std::function<void(const std::string&)> mErrorMsgCallback;
};

}  // namespace ui
}  // namespace ds

#endif
