#pragma once
#ifndef PRIVATE_CEF_WEBSERVICE_H_
#define PRIVATE_CEF_WEBSERVICE_H_

#include <ds/app/engine/engine_service.h>
#include <ds/app/auto_update.h>
#include <memory>
#include <thread>

#include "web_callbacks.h"
#include "web_app.h"

namespace ds {
class Engine;

namespace web {

/**
 * \class ds::web::Service
 * \brief The engine service object that provides access to the Chromium Embedded Framework objects.
 *			This class initializes CEF and provides some high-level functions.
 *			Then it acts as a pass-through to the underlying functionality, which is mostly in WebHandler.

 *	Overall Structure: Service (this) -> create WebApp (initializes context and creates browsers -> creates WebHandler (a single class that manages all the browser instances)
 *						Each Browser (a CEF concept / construct) is a single unit that loads a url into a main frame (and potentially nested frames). 
 *						WebSprites get one browser each. Since browsers are created asynchronously, there's a callback for the WebSprite to grab the browser id after creation.
 *						Once the browser is created (generally within a frame or two), then subsequent functions can be called (forward/back/reload/etc) 
 *						and setup callbacks (for functions such as title changes, errors, new browsing locations, updated view buffers)
 *						CEF runs multiple processes for each browser instance, which is handled by cefsimple.exe, which must be in the working directory.
 */
class WebCefService : public ds::EngineService,
				public ds::AutoUpdate {
public:
	WebCefService(ds::Engine&);
	~WebCefService();

	virtual void			start();

	// Browser crated callback will be called after a browser is created (asynchronously) and returns an ID for the browser.
	// Use that id to interact with the browser after that.
	// The instance ptr is used to clear the callback internally if the instance goes away before the browser has been created
	void					createBrowser(const std::string& startUrl, void * instancePtr, std::function<void(int)> browserCreatedCallback, const bool isTransparent = true,const bool useGPU = true);
	void					cancelCreation(void * instancePtr);

	// Asynchronously close the browser. This also clears any callbacks and no other commands will function. Assume the browser is dead after this call
	void					closeBrowser(const int browserId);

	// Register the object to get callbacks for lots of events (Load complete, title change, etc). See WebCefCallbacks header for details
	void					addWebCallbacks(const int browserId, WebCefCallbacks& callback);

	// browser id was returned from the createBrowser callback, 
	// x / y in pixels from top/left in browser space
	// bttn: left=0, middle = 1; right = 2
	// state: 0 = down, 1 = move, 2 = release. 
	// ClickCount: 1 for 1 click, 2 for 2, etc.
	void					sendMouseClick(const int browserId, const int x, const int y, const int bttn, const int state, const int clickCount);

	// browser id was returned from the createBrowser callback, 
	// x / y in pixels from top/left in browser space
	// xDelta: pixels scrolled in horizontal direction
	// yDelta: pixels scrolled in vertical direction
	void					sendMouseWheelEvent(const int browserId, const int x, const int y, const int xDelta, const int yDelta);


	// browser id was returned from the createBrowser callback, 
	// x / y in pixels from top/left in browser space
	// id is the unique id of the touch point, can be anything other than -1. Up to 16 touch points are tracked
	// phase is the same as "state" from send mouse: 0 = down, 1 = move, 2 = release
	void					sendTouchEvent(const int browserId, const int touchId, const int x, const int y, const int phase);

	// browser id was returned from the createBrowser callback, 
	// state: 0 = down, 1 = move, 2 = release
	// windows_key_code: the VK_### Key code
	// Character: If there's a character associated with this key press. If this is a special key (enter, shift, delete, whatevs) there won't be a char
	// ShiftDown/CntrlDown/AltDown: If the modifier key is depressed (cheer up, modifier key!)
	// isCharacter will force the key event to be keyDown instead of a character (if this is a charcter)
	void					sendKeyEvent(const int browserId, const int state, int windows_key_code, char character, const bool shiftDown, const bool cntrlDown, const bool altDown, const bool isCharacter);

	// Asynchronously load the requested url. Notifications from WebCefCallbacks will happen when appropriate after this
	void					loadUrl(const int browserId, const std::string& newUrl);

	// Asynchronously execute Javascript. The url is for debugging callbacks
	void					executeJavascript(const int browserId, const std::string& theJS, const std::string& debugUrl);

	// Asynchronously resizes the browser. After the resize is complete, a new paint callback will come through at the correct size
	void					requestBrowserResize(const int browserId, const ci::ivec2 newSize);

	// Request the browser to go forwards in history (if it can)
	void					goForwards(const int browserId);

	// Request the browser to go backwards in history (if it can)
	void					goBackwards(const int browserId);

	// Request the browser to reload the current page. If ignoreCache is true, will ignore any saved data and re-load any resources from the net
	void 					reload(const int browserId, const bool ignoreCache);

	// Cancels current load
	void 					stopLoading(const int browserId);

	// This expects a CEF zoom-level, where 0.0 == 100%
	void					setZoomLevel(const int browserId, const double newZoomLevel);
	double					getZoomLevel(const int browserId);

	void					authCallbackCancel(const int browserId);
	void					authCallbackContinue(const int browserId, const std::string& username, const std::string& password);

	void					deleteCookies(const std::string& url, const std::string& cookies);
	std::shared_ptr<d3d11::Device> getD3D11Device();

protected:
	virtual void			update(const ds::UpdateParams&);
	
#ifndef _WIN32
	std::shared_ptr<std::thread>
							mCefMessageLoopThread;
#endif

private:
	CefRefPtr<WebApp>		mCefSimpleApp;
};

} // namespace web
} // namespace ds

#endif // PRIVATE_WEBSERVICE_H_
