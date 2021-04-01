#pragma once

#include <ds/ui/sprite/web.h>
#include <ds/ui/sprite/sprite.h>

namespace ds {
namespace ui {
class YoutubeInterface;
class MediaInterface;
struct MediaViewerSettings;

/**
* \class YouTubeWeb
*			A web sprite with some state tracking and API for video playback controls.
*			This is broken out into another sprite so the interface and the player can both link to it
*/
class YouTubeWeb : public ds::ui::Web {
public:
	YouTubeWeb(ds::ui::SpriteEngine& eng);

	/// Takes the file name of the resource and sets the video id
	virtual void setResource(const ds::Resource& resource) override;
	/// Convenience that sets the resource
	void setMedia(const std::string& mediaPath);
	/// sets the video id
	void setVideoId(const std::string& videoId);
	/// If true, will start playing the youtube immediately on load
	void setAutoStart(const bool autoStart) { mAutoStart = autoStart; }

	/// Once the video has started playing, this will be available
	std::string getVideoTitle() { return mVideoTitle; };
	void setTitleChangedCallback(std::function<void(std::string newTitle)> func) {
		mTitleChangedCallback = func;
	}

	void play();
	void pause();
	void stop();
	void togglePlayPause();
	void toggleMute();

	/// seeks to this percent in the video
	void seekPercent(const float percenty);
	float getCurrentPercent();

	double getDuration() { return mDuration; }
	double getCurrentTime() { return mCurrentPosition; }

	bool getIsPlaying() { return mPlaying; }

	// 0.0f = no sound, 1.0f = full volume
	void setVolume(const float theVolume);
	// 0.0f - 1.0f 
	float getVolume();

protected:
	std::string mVideoTitle;
	std::string mVideoId;
	double mCurrentPosition = 0.0;
	double mDuration = 0.0;
	float mVolume = 0.0f; // 0.0 - 100.0
	bool mPlaying = false;
	bool mAutoStart = true;
	std::function<void(std::string)> mTitleChangedCallback;
};

/**
* \class YouTubePlayer
*			Creates a YouTubeWeb sprite and puts an interface on top of it specifically designed to handle youtube videos
*/
class YouTubePlayer : public ds::ui::Sprite {
public:
	YouTubePlayer(ds::ui::SpriteEngine& eng, const bool embedInterface);

	virtual void setResource(const ds::Resource& resource) override;
	void setMedia(const std::string mediaPath);

	virtual void userInputReceived();
	void		 layout();

	void play();
	void pause();
	void stop();
	void togglePlayPause();
	void toggleMute();

	void showInterface();
	void hideInterface();

	void sendClick(const ci::vec3& globalClickPos);

	ds::ui::YouTubeWeb*  getYouTubeWeb();
	YoutubeInterface* getYoutubeInterface();  // may be nullptr if embedInterface is false

	/// Sets all applicable settings from a MediaViewerSettings
	void setMediaViewerSettings(const MediaViewerSettings& settings);

	void setYouTubeSize(const ci::vec2 youTubeSize);
	void setAllowTouchToggle(const bool allowTouchToggle);
	void setShowInterfaceAtStart(const bool showInterfaceAtStart);
	void setStartInteractable(const bool startInteractable);
	void setLetterbox(const bool doLetterbox);
	void setNativeTouches(const bool isNative);
	void setVolume(const float volume);

protected:
	virtual void onSizeChanged();

	ds::ui::YouTubeWeb*  mYouTubeWeb;
	YoutubeInterface* mYoutubeInterface;
	ci::vec2	  mYouTubeSize;
	float		  mKeyboardKeyScale;
	std::function<void(bool)>	mKeyboardStatusCallback = nullptr;

	bool mEmbedInterface;
	bool mShowInterfaceAtStart;
	bool mLetterbox;
	bool mAllowTouchToggle;
	bool mStartInteractable;
	bool mInterfaceBelowMedia;
	float mInterfaceBottomPad = 50.0f;
	bool mNativeTouches;
	bool mAutoStart;
	float mVolume;
};

}  // namespace ui
}  // namespace ds

