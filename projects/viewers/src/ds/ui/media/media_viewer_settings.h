#pragma once

#include <string>
#include <cinder/Vector.h>
#include <cinder/Color.h>
#include "ds/ui/sprite/pdf_link.h"
#include <gstreamer/gstreamer_audio_device.h>

namespace ds {
namespace ui { 

/// MediaViewerSettings
/// A set of things you can change about MediaViewers
/// Most of these need to be set before loading the media
/// This is a struct cause everything is public
struct MediaViewerSettings {

	MediaViewerSettings()
		: mDefaultBounds(0.0f, 0.0f)
		, mShowInterfaceAtStart(true)
		, mInterfaceBelowMedia(false)
		, mInterfaceBottomPad(50.0f)
		, mLetterBox(true)
		, mWebDefaultSize(0.0f, 0.0f)
		, mWebKeyboardKeyScale(1.0f)
		, mWebKeyboardAbove(true)
		, mWebAllowKeyboard(true)
		, mWebAllowTouchToggle(true)
		, mWebStartTouchable(false)
		, mWebNativeTouches(true)
		, mCacheImages(false)
		, mMipMapImages(true)
		, mPdfCanShowLinks(true)
		, mVideoPanning(0.0f)
		, mVideoVolume(1.0f)
		, mVideoAutoSync(true)
		, mVideoAutoPlayFirstFrame(true)
		, mVideoAllowOutOfBoundsMuting(true)
		, mVideoLoop(true)
		, mVideoResetOnComplete(true)
		, mVideoStreamingLatency(0.2)
		, mVideoGlMode(false)
		, mVideoNVDecode(false)
		, mPanoramicVideoInteractive(true)
		, mYouTubeSize(1920.0f, 1080.0f)
	{}

	//--------------------Overall Settings -----------------------------------------//
	/// The size to be calculated to fit inside when initially loading content
	ci::vec2					mDefaultBounds;

	/// Whether the interface should be visible immediately or not
	bool						mShowInterfaceAtStart;

	/// If true, puts the interface below (vertically) the media, otherwise floats on top on the lower third (the default)
	bool						mInterfaceBelowMedia;

	/// If the interface is above the media, how far off the bottom should it be? Default = 50 pixels
	float						mInterfaceBottomPad;

	/// If true, centers and fits the content. If false, will expand the content so it fills the player (doesn't clip by default though)
	bool						mLetterBox;

	//--------------------Web Settings ---------------------------------------------//

	/// The size to load the web page, which will then get scaled up or down when the viewer changes size
	ci::vec2					mWebDefaultSize;		
	// the scale of the keyboard
	float						mWebKeyboardKeyScale;
	// If we should show the keyboard button at all. Default = true
	bool						mWebAllowKeyboard;
	// If we should show the button to allowing touching web sites. Default=true
	bool						mWebAllowTouchToggle;
	// If true, the keyboard appears above the interface. False will show below
	bool						mWebKeyboardAbove;
	// If true, the website will load interactable. Default = false
	bool						mWebStartTouchable;
	// If true, web sprites will send native touch events to the browser. Default = true
	bool						mWebNativeTouches;


	//--------------------YouTube Settings ------------------------------------------//
	// Note that some video and web settings are used for youtube as well (autoplay, volume, start touchable, native touches)
	ci::vec2					mYouTubeSize;


	//--------------------Image Settings -------------------------------------------//

	/// whether to cache the primary and thumb images. Default = false
	bool						mCacheImages;

	/// whether to mipmap primary images. Default = true;
	bool						mMipMapImages;

	//--------------------PDF Settings ---------------------------------------------//

	/// When the PDF gets touch-toggled, show any internal PDF links. default = true
	bool						mPdfCanShowLinks;

	/// Called back when a PDF link is tapped
	std::function<void(ds::pdf::PdfLinkInfo)> mPdfLinkTappedCallback;

	//--------------------Video Settings -------------------------------------------//

	/// From -1.0f (left) to 1.0f (right). Default = 0.0f (center)
	float						mVideoPanning;

	/// The volume of the video's audio when it starts
	float						mVideoVolume;

	/// A list of clients to not load the video on
	/// Default is empty, which loads everywhere
	/// See GstVideo header for more info
	std::vector<std::string>	mVideoPlayableInstances;

	/// Whether to auto-synchronize video across clients (no effect in standalone mode)
	/// Default = true
	bool						mVideoAutoSync;

	/// Set this to false in a some-instances-only environment, otherwise clients might never get audio
	/// Automatically displays the first frame of the video
	bool						mVideoAutoPlayFirstFrame;

	/// If the video goes out of the current bounds of the window, mute the video automatically (or don't have that functionality)
	bool						mVideoAllowOutOfBoundsMuting;

	/// Whether the video should loop or not by default
	bool						mVideoLoop;

	/// Whether to seek to 0.0 when the video finishes in non-loop mode; default=true
	bool						mVideoResetOnComplete;

	/// Enables OpenGL mode in GStreamer
	bool						mVideoGlMode;

	/// Enables NVidia Cuda decoding, requires nvdec plugin and GlMode to be on
	bool						mVideoNVDecode;

	/// Ability to pan panoramic videos at start
	bool						mPanoramicVideoInteractive;

	/// The latency when opening a stream in seconds
	double						mVideoStreamingLatency;

	/// Which audio devices to play audio out of (see the gstreamer_audio_devices header for more details)
	std::vector<GstAudioDevice> mVideoAudioDevices;


};

} // namespace ui
} // namespace ds

