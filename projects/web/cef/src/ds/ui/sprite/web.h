#pragma once
#ifndef DS_UI_SPRITE_WEB_H_
#define DS_UI_SPRITE_WEB_H_

#include <cinder/app/KeyEvent.h>
#include <cinder/app/MouseEvent.h>
#include <ds/ui/soft_keyboard/entry_field.h>
#include "ds/ui/sprite/text.h"

#include <mutex>
#include <thread>

namespace ds {
namespace web {
	class WebCefService;
}

namespace ui {
/**
 * \class Web
 * \brief Display a web page using Chromium Embedded Framework: https://bitbucket.org/chromiumembedded/cef
 *		  The process and threading model here is complex.
 *		  In short, on creation, this sprite will asynchronously request an underlying browser from CefWebService.
 *        When the browser is created, this sprite will get a unique Browser Id that uses to make requests of the browser (back, load url, etc)
 *		  The browser also sends callbacks for certain events (loading state, fullscreen, etc)
 *		  The browser runs in it's own thread(s), so callbacks need to be locked before sending to the rest of ds_cinder-land
 *		  Callbacks are synchronized with the main thread using the mutex. They also need to happen outside the update loop, so they are cached and called via a 1-frame delay
 *		  Requests into the browser can (generally) happen on any thread, and CEF handles thread synchronization
 *		  CEF also uses multiple processes for rendering, IO, etc. but that is opaque to this class
 *		  When implementing new functionality, be sure to read the documentation of CEF carefully

 *		  We're extending IEntryField so websites can get physical keyboard entry the same way EntryFields can
 */
class Web : public ds::ui::IEntryField {
public:
	struct AuthCallback {
		AuthCallback() : mIsProxy(false), mPort(0){}
		bool			mIsProxy;
		std::string		mHost;
		int				mPort;
		std::string		mRealm;
		std::string		mScheme;
	};


	Web(ds::ui::SpriteEngine &engine, float width = 0.0f, float height = 0.0f);
	~Web();

	/// IEntryField API input
	virtual void								keyPressed(ci::app::KeyEvent& keyEvent);
	virtual void								keyPressed(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType);


	// Loads the new url in the main frame (what you'd expect to happen)
	void										loadUrl(const std::wstring &url);
	void										loadUrl(const std::string &url);

	/// Loads a url based on the resource absolute file name
	virtual void								setResource(const ds::Resource& resource) override;

	// setURL is identical to loadUrl. No exceptions will be thrown from any set or load url function.
	// load/set url are all kept around for compatibility with the old APIs
	void										setUrl(const std::string&);
	void										setUrlOrThrow(const std::string&);

	// returns the last URL set from loadUrl or setUrl (those two are identical). The page may have updated in the meantime
	std::string									getUrl();
	// returns the current url of the site as dispatched from CEF
	std::string									getCurrentUrl();

	// -------- Input Controls -------------------------------- //
	// If this is on (the default), will send the touch events directly to Chromium, which will handle them natively
	// This affects the internal touch handling if this sprite is enabled for touch input and doesn't affect the sendMouseEvents below
	void										setNativeTouchInput(const bool doNativeTouch) { mNativeTouchInput = doNativeTouch; }

	// These are only considered if this sprite is enabled and setNativeTouchInput(false) has been called
	// If the sprite is being touched by mDragScrollMinFingers or more, will send mouse scroll events to the web view.
	void										setDragScrolling(const bool doScrolling){ mDragScrolling = doScrolling; }
	void										setDragScrollingMinimumFingers(const int numFingers){ mDragScrollMinFingers = numFingers; }
	void										setDragScrollingDirection(bool scrollsUp) { mDragScrollingDirection = scrollsUp; }

	/// If the isCharacter flag is false, will send a key down event instead
	void										sendKeyDownEvent(const ci::app::KeyEvent &event, const bool isCharacter = true);
	void										sendKeyUpEvent(const ci::app::KeyEvent &event);

	// This web sprite handles touch-to-mouse events by default.
	// Though you can use these to roll your own touch stuff
	void										sendMouseDownEvent(const ci::app::MouseEvent &event);
	void										sendMouseDragEvent(const ci::app::MouseEvent &event);
	void										sendMouseUpEvent(const ci::app::MouseEvent &event);

	void										sendMouseClick(const ci::vec3& globalClickPoint);

	// DEPRECATED: This is for API-compatibility with the old Awesomium. Always draws while loading now.
	void										setDrawWhileLoading(const bool){};


	// Set the zoom level, where 1 = 100%, 0.25 = 25% etc.
	// Note: this is not the same value as CEF zoom levels (where 0.0 == 100%). This is percentage, like Chrome
	void										setZoom(const double percent);
	// Get the zoom level, where 1 = 100%, 0.25 = 25% etc.
	// Note: this is not the same value as CEF zoom levels (where 0.0 == 100%). This is percentage, like Chrome
	double										getZoom() const;

	// Actions
	void										goBack();
	void										goForward();
	void										reload(const bool ignoreCache = false);
	bool										isLoading();
	void										stop();
	bool										canGoBack();
	bool										canGoForward();

	const std::wstring&							getPageTitle(){ return mTitle; }

	//--- Page Callbacks ----------------------------------- //
	// The page's title has been updated (this may happen multiple times per page load)
	void										setTitleChangedFn(const std::function<void(const std::wstring& newTitle)>&);

	// The page has been navigated to a new address
	void										setAddressChangedFn(const std::function<void(const std::string& new_address)>&);

	// The state of loading in the web browser has been updated (started or finished loading)
	// This is a great time to check the canGoBack(), canGoForwards(), and getURL() as they'll be updated immediately before this
	// If loading just completed, the DocumentReadyFn will be called immediately after this 
	void										setLoadingUpdatedCallback(std::function<void(const bool isLoading)> func);

	// This API is for compatibility with older code that only got updated when the load was complete. Use loadingUpdatedCallback for more granular updates
	// The page has finished loading. This also updates the canNext / canBack properties
	void										setDocumentReadyFn(const std::function<void(void)>&);

	// Something went wrong and the page couldn't load. 
	// Passes back a string with some info (should probably pass back a more complete package of info at some point)
	void										setErrorCallback(std::function<void(const std::string&)> func);

	// The page entered or exited fullscreen. The bool will be true if in fullscreen. 
	// The content that's fullscreen'ed will take up the entire web instance. 
	void										setFullscreenChangedCallback(std::function<void(const bool)> func);

	// The page has requested authorization. If you set this callback, you need to respond using authCallbackCancel() or authCallbackContinue() or the browser will hang indefinitely
	void										setAuthCallback(std::function<void(AuthCallback)> func);
	// After an authorization request, this cancels the request
	void										authCallbackCancel();
	// After an authorization request, this responds with the user / pass to try to continue to the page
	void										authCallbackContinue(const std::string& username, const std::string& password);

	// An error has occurred. No longer displays a text sprite for errors, simply calls back the error callback.
	// You're responsible for displaying the error message yourself
	void										setErrorMessage(const std::string &message);
	void										clearError();

	// Convenience to access various document properties. Note that
	// the document probably needs to have passed onLoaded() for this
	// to be reliable.
	ci::vec2									getDocumentSize();
	ci::vec2									getDocumentScroll();

	/// Executes the script on the main frame of the loaded browser. Browser must exist already for this to work
	void										executeJavascript(const std::string& theScript, const std::string& debugUrl = "");

	/// Lets you disable clicking, but still scroll via "mouse wheel"
	void										setAllowClicks(const bool doAllowClicks);

	/// Sends a second mouse down when releasing, which fixes some websites, but breaks others. The default is on
	void										setSecondClickOnUp(const bool secondClick) { mSecondClickOnUp = secondClick; }

	/// Deletes any cookies matching url (leave blank for all urls) and matching the cookieName (leave blank for all cookies from that url)
	void										deleteCookies(const std::string& url, const std::string& cookieName);

	/// DEPRECATED, everything is transparent now - If true, any transparent web pages will be blank, false will have a white background for pages
	void										setWebTransparent(const bool isTransparent);
	bool										getWebTransparent(){ return mTransparentBackground; }


	virtual void								onUpdateClient(const ds::UpdateParams&) override;
	virtual void								onUpdateServer(const ds::UpdateParams&) override;
	virtual void								drawLocalClient() override;

protected:
	virtual void								onSizeChanged();
	virtual void								writeAttributesTo(ds::DataBuffer&);
	virtual void								readAttributeFrom(const char attributeId, ds::DataBuffer&);

private:

	// For syncing touch input across client/servers
	struct WebTouch {
		WebTouch(const int x, const int y, const int btn = 0, const int state = 0, const int clickCnt = 0)
		: mX(x), mY(y), mBttn(btn), mState(state), mClickCount(clickCnt), mIsWheel(false), mXDelta(0), mYDelta(0){}
		int mX;
		int mY;
		int mBttn;
		int mClickCount;
		int mState;
		bool mIsWheel;
		int mXDelta;
		int mYDelta;
	};

	// For syncing keyboard input across server/clients
	struct WebKeyboardInput {
		WebKeyboardInput(const int state, const int nativeKeyCode, const char character, const bool shiftDown, const bool controlDown, const bool altDown)
		: mState(state), mNativeKeyCode(nativeKeyCode), mCharacter(character), mShiftDown(shiftDown), mCntrlDown(controlDown), mAltDown(altDown){}
		const int mState;
		const int mNativeKeyCode;
		const char mCharacter;
		const bool mShiftDown;
		const bool mCntrlDown;
		const bool mAltDown;
	};

	// For syncing back/forward/stop/reload across server/clients
	struct WebControl {
		static const int GO_BACK = 0;
		static const int GO_FORW = 1;
		static const int RELOAD_SOFT = 2;
		static const int RELOAD_HARD = 3;
		static const int STOP_LOAD = 4;
		WebControl(const int command) : mCommand(command){}
		const int mCommand;
	};

	// Sends to the local web service as well as syncing to any clients
	void										sendTouchToService(const int xp, const int yp, const int btn, const int state, const int clickCnt, 
																   const bool isWheel = false, const int xDelta = 0, const int yDelta = 0);
	void										update(const ds::UpdateParams&);
	void										handleTouch(const ds::ui::TouchInfo&);

	void										clearBrowser();
	void										createBrowser();
	void										initializeBrowser();
	bool										mNeedsInitialized;

	ds::web::WebCefService&						mService;

	int											mBrowserId;
	unsigned char *								mBuffer;
	bool										mHasBuffer;
	ci::ivec2									mBrowserSize; // basically the w/h of this sprite, but tracked so we only recreate the buffer when needed
	ci::gl::TextureRef							mWebTexture;
	bool										mTransparentBackground;

	unsigned char *								mPopupBuffer;
	bool										mHasPopupBuffer;
	ci::gl::TextureRef							mPopupTexture;
	ci::vec2									mPopupPos;
	ci::vec2									mPopupSize;
	bool										mPopupShowing;
	bool										mPopupReady;


	double										mZoom;
	bool										mNeedsZoomCheck;

	bool										mNativeTouchInput;
	bool										mSecondClickOnUp;
	ci::vec3									mPreviousTouchPos;
	bool										mAllowClicks;
	bool										mClickDown;
	bool										mDragScrolling;
	int											mDragScrollMinFingers;
	bool										mDragScrollingDirection;
	bool										mIsDragging;
	// Cache the page size and scroll during touch events
	ci::vec2									mPageSizeCache,
												mPageScrollCache;
	// Prevent the scroll from being cached more than once in an update.
	int32_t										mPageScrollCount;

	// Callbacks need to be called outside of the update loop (since there could be sprites added or removed as a result of the callbacks)
	// So store the callback state and call it using a cinder tween delay
	void										dispatchCallbacks();
	bool										mHasCallbacks;
	bool										mHasDocCallback;
	bool										mHasErrorCallback;
	bool										mHasAddressCallback;
	bool										mHasTitleCallback;
	bool										mHasFullCallback;
	bool										mHasLoadingCallback;
	bool										mHasAuthCallback;

	std::function<void(void)>					mDocumentReadyFn;
	std::function<void(const std::string&)>		mErrorCallback;
	std::function<void(const std::string&)>		mAddressChangedCallback;
	std::function<void(const std::wstring&)>	mTitleChangedCallback;
	std::function<void(const bool)>				mFullscreenCallback;
	std::function<void(const bool)>				mLoadingUpdatedCallback;
	std::function<void(AuthCallback)>			mAuthRequestCallback;
	AuthCallback								mAuthCallback;

	// Replicated state
	std::string									mUrl;
	std::string									mCurrentUrl;
	std::vector<WebTouch>						mTouches;
	std::vector<WebKeyboardInput>				mKeyPresses;
	std::vector<WebControl>						mHistoryRequests;

	bool										mHasError;
	std::string									mErrorMessage;

	// CEF state, cached here so we don't need to (or can't) query the other threads/processes
	std::wstring								mTitle;
	bool										mIsLoading;
	bool										mCanBack;
	bool										mCanForward;
	bool										mIsFullscreen;

	// Ensure threads are locked when getting callbacks, copying buffers, etc
	std::mutex									mMutex;

	ci::CueRef									mCallbacksCue;




	// Initialization
public:
	static void									installAsServer(ds::BlobRegistry&);
	static void									installAsClient(ds::BlobRegistry&);
};

} // namespace ui
} // namespace ds

#endif // DS_UI_SPRITE_WEB_H_
