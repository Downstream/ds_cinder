#include "stdafx.h"

#include "web.h"

#include <algorithm>
#include <regex>
#include <cinder/ImageIo.h>
#include <boost/filesystem.hpp>
#include <ds/app/app.h>
#include <ds/app/engine/engine.h>
#include <ds/app/blob_reader.h>
#include <ds/app/environment.h>
#include <ds/data/data_buffer.h>
#include <ds/debug/logger.h>
#include <ds/math/math_func.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/tween/tweenline.h>
#include <ds/util/string_util.h>
#include "private/web_service.h"
#include "private/web_callbacks.h"
#include "ppl.h"

#include "include/cef_app.h"
#include "osr_d3d11_gl_win.h"


namespace {
// Statically initialize the world class. Done here because the Body is
// guaranteed to be referenced by the final application.
class Init {
public:
	Init() {
		ds::App::AddStartup([](ds::Engine& e) {
			ds::web::WebCefService*		w = new ds::web::WebCefService(e);
			if(!w) {
				DS_LOG_WARNING("Couldn't create the CEF web service!");
				return;
			}
			e.addService("cef_web", *w);

			e.installSprite([](ds::BlobRegistry& r){ds::ui::Web::installAsServer(r);},
							[](ds::BlobRegistry& r){ds::ui::Web::installAsClient(r);});
		});
	}
	void					doNothing() { }
};
Init						INIT;

char						BLOB_TYPE			= 0;
const ds::ui::DirtyState&	URL_DIRTY			= ds::ui::INTERNAL_A_DIRTY;
const ds::ui::DirtyState&	TOUCHES_DIRTY		= ds::ui::INTERNAL_B_DIRTY;
const ds::ui::DirtyState&	KEYBOARD_DIRTY		= ds::ui::INTERNAL_C_DIRTY;
const ds::ui::DirtyState&	HISTORY_DIRTY		= ds::ui::INTERNAL_D_DIRTY;
const char					URL_ATT				= 80;
const char					TOUCH_ATT			= 81;
const char					KEYBOARD_ATT		= 82;
const char					HISTORY_ATT			= 83;
}

namespace ds {
namespace ui {

/**
 * Web static
 */
void Web::installAsServer(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r);});
}

void Web::installAsClient(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<Web>(r);});
}

/**
 * Web
 */
Web::Web( ds::ui::SpriteEngine &engine, float width, float height )
	: IEntryField(engine)
	, mService(engine.getService<ds::web::WebCefService>("cef_web"))
	, mDragScrolling(false)
	, mDragScrollMinFingers(2)
	, mDragScrollingDirection(true)
	, mIsDragging(false)
	, mClickDown(false)
	, mPageScrollCount(0)
	, mDocumentReadyFn(nullptr)
	, mHasError(false)
	, mAllowClicks(true)
	, mSecondClickOnUp(true)
	, mBrowserId(-1)
	, mBuffer(nullptr)
	, mHasBuffer(false)
	, mBrowserSize(0, 0)
	, mPopupBuffer(nullptr)
	, mPopupShowing(false)
	, mPopupReady(false)
	, mHasPopupBuffer(false)
	, mUrl("")
	, mIsLoading(false)
	, mCanBack(false)
	, mCanForward(false)
	, mZoom(1.0)
	, mTransparentBackground(false)
	, mNeedsZoomCheck(false)
	, mHasDocCallback(false)
	, mHasErrorCallback(false)
	, mHasAddressCallback(false)
	, mHasTitleCallback(false)
	, mHasFullCallback(false)
	, mHasLoadingCallback(false)
	, mHasCallbacks(false)
	, mHasAuthCallback(false)
	, mIsFullscreen(false)
	, mNeedsInitialized(false)
	, mCallbacksCue(nullptr)
	, mNativeTouchInput(true)
	, mBrowserLayer(nullptr)
	, mPopupLayer(nullptr)
	, mAccelerated(true)
{
	// Should be unnecessary, but really want to make sure that static gets initialized
	INIT.doNothing();

	mBlobType = BLOB_TYPE;
	mLayoutFixedAspect = true;

	setTransparent(false);
	setUseShaderTexture(true);
	setSize(width, height);

	enable(true);
	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);

	setProcessTouchCallback([this](ds::ui::Sprite *, const ds::ui::TouchInfo &info) {
		handleTouch(info);
	});

	DS_LOG_VERBOSE(4, "Web: creating CEF web sprite");

	createBrowser();
}

void Web::createBrowser() {
	DS_LOG_VERBOSE(3, "Web: create browser");

	clearBrowser();

	mService.createBrowser("", this, [this](int browserId){
		{
			// This callback comes back from the CEF UI thread
			std::lock_guard<std::mutex> lock(mMutex);
			mBrowserId = browserId;
		}

		mNeedsInitialized = true; 

		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			
			mHasCallbacks = true;
		}

	}, mTransparentBackground, mAccelerated);
}

void Web::clearBrowser() {
	DS_LOG_VERBOSE(3, "Web: clear browser");

	if(mBrowserId < 0){
		mService.cancelCreation(this);
	} else {
		// This clears the callbacks too
		mService.closeBrowser(mBrowserId);
	}

	{
		std::lock_guard<std::mutex> lock(mMutex);
		mBrowserId = -1;
	}
}

Web::~Web() {

	clearBrowser();

	if(mCallbacksCue){
		mCallbacksCue->removeSelf();
	}

	if(mBuffer) {
		delete mBuffer;
		mBuffer = nullptr;
	}

	if(mPopupBuffer) {
		delete mPopupBuffer;
		mPopupBuffer = nullptr;
	}

	mHasCallbacks = false;
	mHasDocCallback = false;
	mDocumentReadyFn = nullptr;
	mHasErrorCallback = false;
	mErrorCallback = nullptr;
	mHasAddressCallback = false;
	mAddressChangedCallback = nullptr;
	mHasTitleCallback = false;
	mTitleChangedCallback = nullptr;
	mHasFullCallback = false;
	mFullscreenCallback = nullptr;
	mHasLoadingCallback = false;
	mLoadingUpdatedCallback = nullptr;
	mHasAuthCallback = false;
	mAuthRequestCallback = nullptr;
	
}

void Web::setWebTransparent(const bool isTransparent){
	if(isTransparent == mTransparentBackground) return;
	mTransparentBackground = isTransparent;

	createBrowser();
}

void Web::initializeBrowser(){
	if(mBrowserId < 0){
		return;
	}
	if(!mService.getD3D11Device() && mAccelerated)
	{
		return;
	}

	DS_LOG_INFO("Initialize browser: " << mUrl <<" " << mBrowserId);

	mNeedsInitialized = false;

	// Now that we know about the browser, set it to the correct size
	if (mAccelerated) {
		if (!mBrowserLayer) {
			onSizeChanged();
		}
		else {
			mService.requestBrowserResize(mBrowserId, mBrowserSize);
		}
	}
	else {
		if (!mBuffer) {
			onSizeChanged();
		}
		else {
			mService.requestBrowserResize(mBrowserId, mBrowserSize);
		}
	}

	loadUrl(mUrl);
	if(mZoom != 1.0){
		setZoom(mZoom);
	}

	ds::web::WebCefCallbacks wcc;
	wcc.mTitleChangeCallback = [this](const std::wstring& newTitle){
		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);

		mTitle = newTitle;

		mHasTitleCallback = true;
		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			mHasCallbacks = true;
		}
	};

	wcc.mLoadChangeCallback = [this](const bool isLoading, const bool canBack, const bool canForwards, const std::string& newUrl){
		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);

		mIsLoading = isLoading;
		mCanBack = canBack;
		mCanForward = canForwards;
		mCurrentUrl = newUrl;

		// zoom seems to need to be set for every page
		// This callback is locked in CEF, so zoom checking needs to happen later
		if(mZoom != 1.0){
			mNeedsZoomCheck = true;
		}

		mHasAddressCallback = true;
		mHasLoadingCallback = true;
		mHasDocCallback = true;

		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			mHasCallbacks = true;
		}
	};

	wcc.mPaintCallback = [this](const void * buffer, const int bufferWidth, const int bufferHeight){

		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);

		// verify the buffer exists and is the correct size
		// TODO: Add ability to redraw only the changed rectangles (which is what comes from CEF)
		// Would be much more performant, especially for large browsers with small ui changes (like blinking cursors)
		
		if(mAccelerated && bufferHeight == -1 && bufferWidth == -1)
		{
			if (mBrowserLayer) {
				mBrowserLayer->on_paint(const_cast<void*>(buffer));
			}
		}
		else {
			if (mBuffer && bufferWidth == mBrowserSize.x && bufferHeight == mBrowserSize.y) {
				mHasBuffer = true;
				//mBuffer = (unsigned char *)buffer;

				/*
				auto range = { 0,1,2,3 };
				int chunk = bufferWidth * bufferHeight;
				const char* tmp_buffer = static_cast<const char*>(buffer);
				concurrency::parallel_for(0, 4, [&](int idx)
				{
					memcpy(mBuffer + chunk*idx, tmp_buffer + chunk*idx, chunk);
				});
				*/
				memcpy(mBuffer, buffer, bufferWidth * bufferHeight * 4);
				
			}
		}
		
	};

	wcc.mPopupPaintCallback = [this](const void * buffer, const int bufferWidth, const int bufferHeight) {
		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);

		// resize buffer if needed
		if(mPopupBuffer && (bufferWidth != mPopupSize.x || bufferHeight != mPopupSize.y)) {
			if (mAccelerated)
			{
				mPopupLayer = std::shared_ptr<PopupLayer>(nullptr);
			} else {
				delete mPopupBuffer;
				mPopupBuffer = nullptr;
			} 
		}

		// create the buffer if needed
		if (mAccelerated) {
			if(!mPopupLayer)
			{
				if (mService.getD3D11Device()) {
					mPopupLayer = std::make_shared<PopupLayer>(mService.getD3D11Device());
					mPopupLayer->setTexture(bufferWidth, bufferHeight);
				}
			}
		} else {
			if (!mPopupBuffer) {
				mPopupBuffer = new unsigned char[bufferWidth * bufferHeight * 4];
			}
		}

		// if everything went ok
		if(bufferWidth == mPopupSize.x && bufferHeight == mPopupSize.y) {
			if(mAccelerated)
			{
				if(mPopupLayer)
				{
					mPopupLayer->on_paint(const_cast<void*>(buffer));
				}
			}
			else {
				if (mPopupBuffer) {
					mHasPopupBuffer = true;
					memcpy(mPopupBuffer, buffer, bufferWidth * bufferHeight * 4);
				}
			}
		}
	};

	wcc.mPopupRectCallback = [this](const int xp, const int yp, const int widthy, const int heighty) {

		std::lock_guard<std::mutex> lock(mMutex);
		mHasPopupBuffer = false;
		
		if(mPopupSize.x != widthy || mPopupSize.y != heighty) {
			if(mAccelerated)
			{
				mPopupLayer = std::shared_ptr<PopupLayer>(nullptr);
			}
			else {
				delete mPopupBuffer;
				mPopupBuffer = nullptr;
			}
			mPopupReady = false;
				
		}

		mPopupPos = ci::vec2(xp, yp);
		mPopupSize = ci::vec2(widthy, heighty);

	};

	wcc.mPopupShowCallback = [this](const bool showing) {
		std::lock_guard<std::mutex> lock(mMutex);
		mPopupShowing = showing;
	};

	wcc.mErrorCallback = [this](const std::string& theError){
		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);
		mHasError = true;
		mErrorMessage = theError;

		mHasErrorCallback = true;

		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			mHasCallbacks = true;
		}
	};

	wcc.mFullscreenCallback = [this](const bool isFullscreen){
		// This callback comes back from the CEF UI thread
		std::lock_guard<std::mutex> lock(mMutex);
		mIsFullscreen = isFullscreen;

		mHasFullCallback = true;

		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			mHasCallbacks = true;
		}
	};

	wcc.mAuthCallback = [this](const bool isProxy, const std::string& host, const int port, const std::string& realm, const std::string& scheme){
		// This callback comes back from the CEF IO thread
		std::lock_guard<std::mutex> lock(mMutex);

		// If the client ui has a callback for authorization, do that
		// Otherwise, just cancel the request
		if(mAuthRequestCallback){
			mAuthCallback.mIsProxy = isProxy;
			mAuthCallback.mHost = host;
			mAuthCallback.mPort = port;
			mAuthCallback.mRealm = realm;
			mAuthCallback.mScheme = scheme;

		} // if there's no authRequestCallback, we handle this in a callback to avoid recursive lock

	
		mHasAuthCallback = true;

		if(!mHasCallbacks){
			auto& t = mEngine.getTweenline().getTimeline();
			mCallbacksCue = t.add([this]{ dispatchCallbacks(); }, t.getCurrentTime() + 0.001f);
			mHasCallbacks = true;
		}
	};

	mService.addWebCallbacks(mBrowserId, wcc);
}


void Web::dispatchCallbacks(){
	if(mNeedsInitialized){
		initializeBrowser();
	}

	if(mHasDocCallback){
		if(mDocumentReadyFn) mDocumentReadyFn();
		mHasDocCallback = false;
	}

	if(mHasErrorCallback){
		if(mErrorCallback) mErrorCallback(mErrorMessage);
		mHasErrorCallback = false;
	}

	if(mHasAddressCallback){
		if(mAddressChangedCallback) mAddressChangedCallback(mUrl);
		mHasAddressCallback = false;
	}

	if(mHasTitleCallback){
		if(mTitleChangedCallback) mTitleChangedCallback(mTitle);
		mHasTitleCallback = false;
	}

	if(mHasFullCallback){
		if(mFullscreenCallback) mFullscreenCallback(mIsFullscreen);
		mHasFullCallback = false;
	}

	if(mHasLoadingCallback){
		if(mLoadingUpdatedCallback) mLoadingUpdatedCallback(mIsLoading);
		mHasLoadingCallback = false;
	}

	if(mHasAuthCallback){
		if(mAuthRequestCallback){
			mAuthRequestCallback(mAuthCallback);
		} else {
			mService.authCallbackCancel(mBrowserId);
		}
		mHasAuthCallback = false;
	}

	mHasCallbacks = false;
	mCallbacksCue = nullptr;
}

void Web::onUpdateClient(const ds::UpdateParams &p) {
	update(p);
}

void Web::onUpdateServer(const ds::UpdateParams &p) {
	mPageScrollCount = 0;

	update(p);
}

void Web::update(const ds::UpdateParams &p) {

	// Get zoom locks CEF, so 
	if(mNeedsZoomCheck && getZoom() != mZoom){
		mNeedsZoomCheck = false;
		setZoom(mZoom);
	}

	// Anything that modifies mBuffer needs to be locked
	std::lock_guard<std::mutex> lock(mMutex);

	if(mBuffer && mHasBuffer && !mAccelerated) {
		DS_LOG_VERBOSE(5, "Web: creating draw texture " << mUrl);

		ci::gl::Texture::Format fmt;
		fmt.enableMipmapping(true);
		//fmt.setMinFilter(GL_LINEAR);
		//fmt.setMagFilter(GL_LINEAR);
		if (!mWebTexture || mWebTexture->getWidth()!=mBrowserSize.x || mWebTexture->getHeight() != mBrowserSize.y) {
			mWebTexture = ci::gl::Texture::create(mBuffer, GL_BGRA, mBrowserSize.x, mBrowserSize.y, fmt);
		}
		else {
			mWebTexture->update(mBuffer, GL_BGRA, GL_UNSIGNED_BYTE, 0, mBrowserSize.x, mBrowserSize.y);
		}
		mHasBuffer = false;
	}


	if(mPopupBuffer && mHasPopupBuffer) {
		DS_LOG_VERBOSE(5, "Web: creating popup draw texture " << mUrl);

		ci::gl::Texture::Format fmt;
		fmt.enableMipmapping(true);
		//fmt.setMinFilter(GL_LINEAR);
		//fmt.setMagFilter(GL_LINEAR);
		mPopupTexture = ci::gl::Texture::create(mPopupBuffer, GL_BGRA, (int)mPopupSize.x, (int)mPopupSize.y, fmt);
		mHasPopupBuffer = false;
		mPopupReady = true;
	}

}

void Web::onSizeChanged() {
	{
		// Anything that modifies mBuffer needs to be locked
		std::lock_guard<std::mutex> lock(mMutex);


		const int theWid = static_cast<int>(getWidth());
		const int theHid = static_cast<int>(getHeight());
		const ci::ivec2 newBrowserSize(theWid, theHid);
		if (newBrowserSize == mBrowserSize && mBuffer) {
			return;
		}

		mBrowserSize = newBrowserSize;

		//accelerated renderers
		if (mAccelerated)
		{
			if (mService.getD3D11Device()) {
				mBrowserLayer = std::make_shared<BrowserLayer>(mService.getD3D11Device());
				mBrowserLayer->setTexture(theWid, theHid);
			}
		}

		if (mBuffer) {
			delete mBuffer;
			mBuffer = nullptr;
		}

		if (!mAccelerated) {
			const int bufferSize = theWid * theHid * 4;
			mBuffer = new unsigned char[bufferSize];
		}
		mHasBuffer = false;




	}

	DS_LOG_VERBOSE(4, "Web: changed size " << getSize() << " url=" << mUrl);

	if(mBrowserId > -1){
		mService.requestBrowserResize(mBrowserId, mBrowserSize);
	}
}

void Web::drawLocalClient() {
	auto localWebTexture = mWebTexture;
	auto localPopupTexture = mPopupTexture;
	if(mAccelerated)
	{
		if (mBrowserLayer) {
			localWebTexture = mBrowserLayer->ofSharedTexture;
		} else
		{
			localWebTexture = nullptr;
		}
		if (mPopupLayer) {
			localPopupTexture = mPopupLayer->ofSharedTexture;
		} else
		{
			localPopupTexture = nullptr;
		}
	}
	
	if(localWebTexture) {

		DS_LOG_VERBOSE(8, "Web: drawing web " << mUrl);

		if(mRenderBatch){
			// web texture is top down, and render batches work bottom up
			// so flippy flip flip

			if(!mTransparentBackground) {
			//	ci::gl::color(ci::Color::white());
			//	ci::gl::drawSolidRect(ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()));
			}

			ci::gl::scale(1.0f, -1.0f);
			ci::gl::translate(0.0f, -getHeight());
			mWebTexture->bind();
			mRenderBatch->draw();
			mWebTexture->unbind();

			if(localPopupTexture && mPopupShowing && mPopupReady) {
				ci::gl::draw(localPopupTexture, ci::Rectf(static_cast<float>(mPopupPos.x), static_cast<float>(getHeight() - localPopupTexture->getHeight()) - mPopupPos.y, static_cast<float>( localPopupTexture->getWidth()) + mPopupPos.x, static_cast<float>(getHeight()) - mPopupPos.y));
			}
		} else {
			ci::gl::draw(localWebTexture, ci::Rectf(0.0f, static_cast<float>(localWebTexture->getHeight()), static_cast<float>(localWebTexture->getWidth()), 0.0f));
		}
	}
}

std::string Web::getUrl() {
	return mUrl;
}

std::string Web::getCurrentUrl(){
	std::lock_guard<std::mutex> lock(mMutex);
	return mCurrentUrl;
}

void Web::loadUrl(const std::wstring &url) {
	loadUrl(ds::utf8_from_wstr(url));
}

void Web::loadUrl(const std::string &url) {

	DS_LOG_VERBOSE(1, "Web: loading url " << url);

	mCurrentUrl = url;
	mUrl = url;
	markAsDirty(URL_DIRTY);
	if(mBrowserId > -1 && !mUrl.empty()){
		mService.loadUrl(mBrowserId, mUrl);
	}
}

void Web::setResource(const ds::Resource& resource) {
	loadUrl(resource.getAbsoluteFilePath());
}

void Web::setUrl(const std::string& url) {
	loadUrl(url);
}

void Web::setUrlOrThrow(const std::string& url) {
	loadUrl(url);
}


void Web::keyPressed(ci::app::KeyEvent& keyEvent) {
	sendKeyDownEvent(keyEvent);
	sendKeyUpEvent(keyEvent);
}

void Web::keyPressed(const std::wstring& character, const ds::ui::SoftKeyboardDefs::KeyType keyType) {

	// spoof a keyevent to send to the web
	int code = 0;

	if(keyType == ds::ui::SoftKeyboardDefs::kShift) {
	} else if(keyType == ds::ui::SoftKeyboardDefs::kArrow) {
		if(character == L"<") code = ci::app::KeyEvent::KEY_LEFT;
		if(character == L"^") code = ci::app::KeyEvent::KEY_UP;
		if(character == L"v") code = ci::app::KeyEvent::KEY_DOWN;
		if(character == L">") code = ci::app::KeyEvent::KEY_RIGHT;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);
	} else if(keyType == ds::ui::SoftKeyboardDefs::kFunction) {
		if(character == L"F1") code = ci::app::KeyEvent::KEY_F1;
		if(character == L"F2") code = ci::app::KeyEvent::KEY_F2;
		if(character == L"F3") code = ci::app::KeyEvent::KEY_F3;
		if(character == L"F4") code = ci::app::KeyEvent::KEY_F4;
		if(character == L"F5") code = ci::app::KeyEvent::KEY_F5;
		if(character == L"F6") code = ci::app::KeyEvent::KEY_F6;
		if(character == L"F7") code = ci::app::KeyEvent::KEY_F7;
		if(character == L"F8") code = ci::app::KeyEvent::KEY_F8;
		if(character == L"F9") code = ci::app::KeyEvent::KEY_F9;
		if(character == L"F10") code = ci::app::KeyEvent::KEY_F10;
		if(character == L"F11") code = ci::app::KeyEvent::KEY_F11;
		if(character == L"F12") code = ci::app::KeyEvent::KEY_F12;
		ci::app::KeyEvent event( mEngine.getWindow(), code,	code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);

	} else if(keyType == ds::ui::SoftKeyboardDefs::kEscape) {
		code = ci::app::KeyEvent::KEY_ESCAPE;
		ci::app::KeyEvent event( mEngine.getWindow(), code,	code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);

	} else if(keyType == ds::ui::SoftKeyboardDefs::kFwdDelete) {
		code = ci::app::KeyEvent::KEY_DELETE;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);

	} else if(keyType == ds::ui::SoftKeyboardDefs::kSpecial) {
		if(character == L"Home") code = ci::app::KeyEvent::KEY_HOME;
		if(character == L"End") code = ci::app::KeyEvent::KEY_END;
		if(character == L"PgUp") code = ci::app::KeyEvent::KEY_PAGEUP;
		if(character == L"PgDn") code = ci::app::KeyEvent::KEY_PAGEDOWN;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);


	} else if(keyType == ds::ui::SoftKeyboardDefs::kDelete) {
		code = ci::app::KeyEvent::KEY_BACKSPACE;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);
	} else if(keyType == ds::ui::SoftKeyboardDefs::kEnter) {
		code = ci::app::KeyEvent::KEY_RETURN;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '\r', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);
	} else if(keyType == ds::ui::SoftKeyboardDefs::kTab) {
		code = ci::app::KeyEvent::KEY_TAB;
		ci::app::KeyEvent event(mEngine.getWindow(), code, code, '	', 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);
	} else {
		ci::app::KeyEvent event(mEngine.getWindow(), code, 0, (char)character.c_str()[0], 0, code);
		sendKeyDownEvent(event);
		sendKeyUpEvent(event);
	}
}

void Web::sendKeyDownEvent(const ci::app::KeyEvent &event, const bool isCharacter) {
	DS_LOG_VERBOSE(3, "Web: send key down " << event.getChar() << " code: " << event.getCode());

	mService.sendKeyEvent(mBrowserId, 0, event.getNativeKeyCode(), event.getChar(), event.isShiftDown(), event.isControlDown(), event.isAltDown(), isCharacter);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		mKeyPresses.push_back(WebKeyboardInput(0, event.getNativeKeyCode(), event.getChar(), event.isShiftDown(), event.isControlDown(), event.isAltDown()));
		markAsDirty(KEYBOARD_DIRTY);
	}
}

void Web::sendKeyUpEvent(const ci::app::KeyEvent &event) {
	DS_LOG_VERBOSE(3, "Web: send key up " << event.getChar() << " code: " << event.getCode());

	mService.sendKeyEvent(mBrowserId, 2, event.getNativeKeyCode(), event.getChar(), event.isShiftDown(), event.isControlDown(), event.isAltDown(), true);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		mKeyPresses.push_back(WebKeyboardInput(2, event.getNativeKeyCode(), event.getChar(), event.isShiftDown(), event.isControlDown(), event.isAltDown()));
		markAsDirty(KEYBOARD_DIRTY);
	}
}

void Web::sendMouseDownEvent(const ci::app::MouseEvent& e) {
	if(!mAllowClicks) return;

	sendTouchToService(e.getX(), e.getY(), 0, 0, 1);
}

void Web::sendMouseDragEvent(const ci::app::MouseEvent& e) {
	if(!mAllowClicks) return;

	sendTouchToService(e.getX(), e.getY(), 0, 1, 1);
}

void Web::sendMouseUpEvent(const ci::app::MouseEvent& e) {
	if(!mAllowClicks) return;

	sendTouchToService(e.getX(), e.getY(), 0, 2, 1);
}

void Web::sendMouseClick(const ci::vec3& globalClickPoint) {
	DS_LOG_VERBOSE(3, "Web: send mouse click " << globalClickPoint);

	if(!mAllowClicks) return;

	ci::vec2 pos = ci::vec2(globalToLocal(globalClickPoint));
	int xPos = (int)roundf(pos.x);
	int yPos = (int)roundf(pos.y);

	if (xPos < -10000000 || yPos < -10000000) {
		xPos = (int)(globalClickPoint.x / getScale().x);
		yPos = (int)(globalClickPoint.y / getScale().y);
	}

	sendTouchToService(xPos, yPos, 0, 0, 1);
//	sendTouchToService(xPos, yPos, 0, 1, 1);
	sendTouchToService(xPos, yPos, 0, 2, 1);
}

void Web::sendTouchToService(const int xp, const int yp, const int btn, const int state, const int clickCnt, 
							 const bool isWheel, const int xDelta, const int yDelta) {
	//std::cout << "Sending touch, state: " << state << " click: " << clickCnt << " " << xp << " " << yp << std::endl;
	if(mBrowserId < 0) return; 

	if(isWheel){
		mService.sendMouseWheelEvent(mBrowserId, xp, yp, xDelta, yDelta);
	} else {
		mService.sendMouseClick(mBrowserId, xp, yp, btn, state, clickCnt);
	}

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		WebTouch wt = WebTouch(xp, yp, btn, state, clickCnt);
		if(isWheel){
			wt.mIsWheel = true;
			wt.mXDelta = xDelta;
			wt.mYDelta = yDelta;
		}
		mTouches.push_back(wt);

		markAsDirty(TOUCHES_DIRTY);
	}
}

void Web::handleTouch(const ds::ui::TouchInfo& touchInfo) {
	if (mNativeTouchInput) {
		ci::vec2 pos = ci::vec2(globalToLocal(touchInfo.mCurrentGlobalPoint));
		int xPos = (int)roundf(pos.x);
		int yPos = (int)roundf(pos.y);

		// in case the global to local failed, use the global point
		if (xPos < -10000000 || yPos < -10000000) {
			xPos = (int)(touchInfo.mCurrentGlobalPoint.x / getScale().x);
			yPos = (int)(touchInfo.mCurrentGlobalPoint.y / getScale().y);
		}

		int phasey = 0;
		if (touchInfo.mPhase == ds::ui::TouchInfo::Moved) phasey = 1;
		if (touchInfo.mPhase == ds::ui::TouchInfo::Removed) phasey = 2;
		mService.sendTouchEvent(mBrowserId, touchInfo.mFingerId, xPos, yPos, phasey);
		return;
	}

	if(touchInfo.mFingerIndex != 0)
		return;

	ci::vec2 pos = ci::vec2(globalToLocal(touchInfo.mCurrentGlobalPoint));
	int xPos = (int)roundf(pos.x);
	int yPos = (int)roundf(pos.y);

	// in case the global to local failed, use the global point
	if(xPos < -10000000 || yPos < -10000000) {
		xPos = (int)(touchInfo.mCurrentGlobalPoint.x / getScale().x);
		yPos = (int)(touchInfo.mCurrentGlobalPoint.y / getScale().y);
	}

	if(ds::ui::TouchInfo::Added == touchInfo.mPhase) {
		if(mAllowClicks){
			// send a move at first, since a lot of sites have stuff with mouse overs
			sendTouchToService(xPos, yPos, 0, 1, 0);
			sendTouchToService(xPos, yPos, 0, 0, 1);
		}
		if(mDragScrolling){
			mClickDown = true;
		}

		mIsDragging = false;
		
	} else if(ds::ui::TouchInfo::Moved == touchInfo.mPhase) {

		if(!mIsDragging && glm::distance(touchInfo.mStartPoint, touchInfo.mCurrentGlobalPoint) > mEngine.getMinTapDistance()) {
			mIsDragging = true;
		}

		if(mDragScrolling && touchInfo.mNumberFingers >= mDragScrollMinFingers){
			if(mIsDragging) {
				if(mClickDown) {
					if(mAllowClicks) {
						sendTouchToService(xPos, yPos, 0, 1, 0);
						sendTouchToService(xPos, yPos, 0, 2, 0);
					}
					mClickDown = false;
				}

				float yDelta = touchInfo.mCurrentGlobalPoint.y - mPreviousTouchPos.y;
				if (!mDragScrollingDirection) yDelta = -yDelta;
				sendTouchToService(xPos, yPos, 0, 0, 0, true, 0, static_cast<int>(roundf(yDelta)));
			}

			
		} else {
			if(mAllowClicks){
				sendTouchToService(xPos, yPos, 0, 1, 1);
			}
		}
	} else if(ds::ui::TouchInfo::Removed == touchInfo.mPhase) {
		if(mAllowClicks) {
			//sendTouchToService(xPos, yPos, 0, 2, 1);

			if(!mIsDragging){// && touchInfo.mStartPoint != touchInfo.mCurrentGlobalPoint) {
				// send another click
				if(mSecondClickOnUp) {
					sendTouchToService(xPos, yPos, 0, 0, 1);
				}
				//sendTouchToService(xPos, yPos, 0, 1, 1);
				sendTouchToService(xPos, yPos, 0, 2, 1);
			} else {
				sendTouchToService(xPos, yPos, 0, 2, 0);
			}
		}
	}

	mPreviousTouchPos = touchInfo.mCurrentGlobalPoint;
}

void Web::setZoom(const double percent) {
	mZoom = percent;
	mService.setZoomLevel(mBrowserId, (percent - 1.0) / .25);
}

double Web::getZoom() const {
	if(mBrowserId < 0) return mZoom;
	return (mService.getZoomLevel(mBrowserId)*.25 + 1.0);
}

void Web::goBack() {
	DS_LOG_VERBOSE(2, "Web: going back on " << mUrl);

	mService.goBackwards(mBrowserId);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		mHistoryRequests.push_back(WebControl(WebControl::GO_FORW));
		markAsDirty(HISTORY_DIRTY);
	}
}

void Web::goForward() {
	DS_LOG_VERBOSE(2, "Web: going forwards on " << mUrl);

	mService.goForwards(mBrowserId);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		mHistoryRequests.push_back(WebControl(WebControl::GO_BACK));
		markAsDirty(HISTORY_DIRTY);
	}
}

void Web::reload(const bool ignoreCache) {
	DS_LOG_VERBOSE(2, "Web: reloading on " << mUrl);

	mService.reload(mBrowserId, ignoreCache);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		if(ignoreCache){
			mHistoryRequests.push_back(WebControl(WebControl::RELOAD_HARD));
		} else {
			mHistoryRequests.push_back(WebControl(WebControl::RELOAD_SOFT));
		}
		markAsDirty(HISTORY_DIRTY);
	}
}

void Web::stop() {
	DS_LOG_VERBOSE(2, "Web: stop loading on " << mUrl);

	mService.stopLoading(mBrowserId);

	if(mEngine.getMode() == ds::ui::SpriteEngine::SERVER_MODE || mEngine.getMode() == ds::ui::SpriteEngine::CLIENTSERVER_MODE){
		mHistoryRequests.push_back(WebControl(WebControl::STOP_LOAD));
		markAsDirty(HISTORY_DIRTY);
	}
}

bool Web::canGoBack() {
	return mCanBack;
}

bool Web::canGoForward() {
	return mCanForward;
}

bool Web::isLoading() {
	return mIsLoading;
}

void Web::setTitleChangedFn(const std::function<void(const std::wstring& newTitle)>& func){
	mTitleChangedCallback = func;
}

void Web::setAddressChangedFn(const std::function<void(const std::string& new_address)>& fn) {
	mAddressChangedCallback = fn;
}

void Web::setLoadingUpdatedCallback(std::function<void(const bool isLoading)> fn) {
	mLoadingUpdatedCallback = fn;
}

void Web::setDocumentReadyFn(const std::function<void(void)>& fn) {
	mDocumentReadyFn = fn;
}

void Web::setErrorCallback(std::function<void(const std::string&)> func){
	mErrorCallback = func;
}

void Web::setFullscreenChangedCallback(std::function<void(const bool)> func){
	mFullscreenCallback = func;
}

void Web::setAuthCallback(std::function<void(AuthCallback)> func){
	mAuthRequestCallback = func;
}

void Web::authCallbackCancel(){
	mService.authCallbackCancel(mBrowserId);
}

void Web::authCallbackContinue(const std::string& username, const std::string& password){
	mService.authCallbackContinue(mBrowserId, username, password);
}

void Web::setErrorMessage(const std::string &message){
	mHasError = true;
	mErrorMessage = message;

	if(mErrorCallback){
		mErrorCallback(mErrorMessage);
	}
}

void Web::clearError(){
	mHasError = false;
}

ci::vec2 Web::getDocumentSize() {
	// TODO?
	return ci::vec2(getWidth(), getHeight());
}

ci::vec2 Web::getDocumentScroll() {
	/* TODO
	if (!mWebViewPtr) return ci::vec2(0.0f, 0.0f);
	return get_document_scroll(*mWebViewPtr);
	*/
	return ci::vec2(0.0f, 0.0f);
}

void Web::executeJavascript(const std::string& theScript, const std::string& debugUrl /*= ""*/) {
	mService.executeJavascript(mBrowserId, theScript, debugUrl);
}

void Web::writeAttributesTo(ds::DataBuffer &buf) {
	ds::ui::Sprite::writeAttributesTo(buf);

	if (mDirty.has(URL_DIRTY)) {
		buf.add(URL_ATT);
		buf.add(mUrl);
	}

	if(mDirty.has(TOUCHES_DIRTY) && !mTouches.empty()){
		buf.add(TOUCH_ATT);
		buf.add(static_cast<int>(mTouches.size()));
		for (auto it : mTouches){
			buf.add(it.mX);
			buf.add(it.mY);
			buf.add(it.mBttn);
			buf.add(it.mState);
			buf.add(it.mClickCount);
			buf.add(it.mIsWheel);
			buf.add(it.mXDelta);
			buf.add(it.mYDelta);
		}

		mTouches.clear();
	}

	if(mDirty.has(KEYBOARD_DIRTY) && !mKeyPresses.empty()){
		buf.add(KEYBOARD_ATT);
		buf.add(static_cast<int>(mKeyPresses.size()));
		for(auto it : mKeyPresses){
			buf.add(it.mState);
			buf.add(it.mNativeKeyCode);
			buf.add(it.mCharacter);
			buf.add(it.mShiftDown);
			buf.add(it.mCntrlDown);
			buf.add(it.mAltDown);
		}

		mKeyPresses.clear();
	}

	if(mDirty.has(HISTORY_DIRTY) && !mHistoryRequests.empty()){
		buf.add(HISTORY_ATT);
		buf.add(static_cast<int>(mHistoryRequests.size()));
		for (auto it : mHistoryRequests){
			buf.add(it.mCommand);
		}

		mHistoryRequests.clear();
	}
}

void Web::readAttributeFrom(const char attributeId, ds::DataBuffer &buf) {
	if(attributeId == URL_ATT) {
		setUrl(buf.read<std::string>());
	} else if(attributeId == TOUCH_ATT){
		auto sizey = buf.read<int>();
		for(int i = 0; i < sizey; i++){
			int xxx = buf.read<int>();
			int yyy = buf.read<int>();
			int btn = buf.read<int>();
			int sta = buf.read<int>();
			int clk = buf.read<int>();
			bool iw = buf.read<bool>();
			int xd = buf.read<int>();
			int yd = buf.read<int>();
			sendTouchToService(xxx, yyy, btn, sta, clk, iw, xd, yd);
		}

	} else if(attributeId == KEYBOARD_ATT){
		auto sizey = buf.read<int>();
		for(int i = 0; i < sizey; i++){
			int state = buf.read<int>();
			int nativ = buf.read<int>();
			char chary = buf.read<char>();
			bool isShif = buf.read<bool>();
			bool isCntrl = buf.read<bool>();
			bool isAlt = buf.read<bool>();

			if(mBrowserId > -1){
				mService.sendKeyEvent(mBrowserId, state, nativ, chary, isShif, isCntrl, isAlt, true);
			}
		}

	} else if(attributeId == HISTORY_ATT){
		auto sizey = buf.read<int>();
		for(auto i = 0; i < sizey; i++){
			int commandy = buf.read<int>();
			if(commandy == WebControl::GO_BACK){
				goBack();
			} else if(commandy == WebControl::GO_FORW){
				goForward();
			} else if(commandy == WebControl::RELOAD_SOFT){
				reload();
			} else if(commandy == WebControl::RELOAD_HARD){
				reload(true);
			} else if(commandy == WebControl::STOP_LOAD){
				stop();
			} 
		}
	} else {
		ds::ui::Sprite::readAttributeFrom(attributeId, buf);
	}
}

void Web::setAllowClicks(const bool doAllowClicks){
	mAllowClicks = doAllowClicks;
}

void Web::deleteCookies(const std::string& url, const std::string& cookieName) {
	mService.deleteCookies(url, cookieName);
}

Web::BrowserLayer::BrowserLayer(const std::shared_ptr<d3d11::Device>& device)
	: d3d11::Layer(device) {
}

void Web::BrowserLayer::on_paint(void* share_handle) {
	frame_buffer_->on_paint(share_handle);
}

std::pair<uint32_t, uint32_t> Web::BrowserLayer::texture_size() const {
	const auto texture = frame_buffer_->texture();
	return std::make_pair(texture->width(), texture->height());
}

Web::PopupLayer::PopupLayer(const std::shared_ptr<d3d11::Device>& device)
	: BrowserLayer(device) {
}

void Web::PopupLayer::set_bounds(const CefRect& bounds) {
	const auto comp = composition();
	if (!comp)
		return;

	const auto outer_width = comp->width();
	const auto outer_height = comp->height();
	if (outer_width == 0 || outer_height == 0)
		return;

	original_bounds_ = bounds;
	bounds_ = bounds;

	// If x or y are negative, move them to 0.
	if (bounds_.x < 0)
		bounds_.x = 0;
	if (bounds_.y < 0)
		bounds_.y = 0;
	// If popup goes outside the view, try to reposition origin
	if (bounds_.x + bounds_.width > outer_width)
		bounds_.x = outer_width - bounds_.width;
	if (bounds_.y + bounds_.height > outer_height)
		bounds_.y = outer_height - bounds_.height;
	// If x or y became negative, move them to 0 again.
	if (bounds_.x < 0)
		bounds_.x = 0;
	if (bounds_.y < 0)
		bounds_.y = 0;

	const auto x = bounds_.x / float(outer_width);
	const auto y = bounds_.y / float(outer_height);
	const auto w = bounds_.width / float(outer_width);
	const auto h = bounds_.height / float(outer_height);
	move(x, y, w, h);
}

} // namespace ui
} // namespace ds
