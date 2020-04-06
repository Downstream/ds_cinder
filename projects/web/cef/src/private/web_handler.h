// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef DS_WEB_PRIVATE_CEF_WEB_HANDLER
#define DS_WEB_PRIVATE_CEF_WEB_HANDLER

/// This class has been modified from the CEF example SimpleApp's SimpleHandler

#include "include/cef_client.h"
#include "include/base/cef_lock.h"

#include <functional>
#include <list>
#include <cinder/Vector.h>

#include "web_callbacks.h"
#include "osr_d3d11_gl_win.h"

namespace ds {
namespace web{

	class AcceleratedRenderHandler;

class WebHandler : public CefClient,
	public CefDisplayHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler,
	public CefRenderHandler,
	public CefJSDialogHandler,
	public CefFocusHandler,
	public CefKeyboardHandler,
	public CefRequestHandler
{
	friend AcceleratedRenderHandler;
public:
	explicit WebHandler();
	~WebHandler();

	// Provide access to the single global instance of this object.
	static WebHandler* GetInstance();

	// CefClient methods:
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefFocusHandler> GetFocusHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() OVERRIDE{
		return this;
	}
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE{
		return this;
	}

		// CefDisplayHandler methods:
	virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
		const CefString& title) OVERRIDE;

	virtual void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser,
										bool fullscreen) OVERRIDE;

	// CefLifeSpanHandler methods:
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

	virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
							   CefRefPtr<CefFrame> frame,
							   const CefString& target_url,
							   const CefString& target_frame_name,
							   CefLifeSpanHandler::WindowOpenDisposition target_disposition,
							   bool user_gesture,
							   const CefPopupFeatures& popupFeatures,
							   CefWindowInfo& windowInfo,
							   CefRefPtr<CefClient>& client,
							   CefBrowserSettings& settings,
							   CefRefPtr<CefDictionaryValue>& extra_info,
							   bool* no_javascript_access) OVERRIDE;

	// CefLoadHandler methods:
	virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
							 CefRefPtr<CefFrame> frame,
							 ErrorCode errorCode,
							 const CefString& errorText,
							 const CefString& failedUrl) OVERRIDE;

	virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
									  bool isLoading,
									  bool canGoBack,
									  bool canGoForward) OVERRIDE;

	// CefRenderHandler methods:
	virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) OVERRIDE;
	virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
								int viewX,
								int viewY,
								int& screenX,
								int& screenY) OVERRIDE;
	virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) OVERRIDE;
	virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) OVERRIDE;

	virtual void OnPaint(CefRefPtr<CefBrowser> browser,
						 PaintElementType type,
						 const RectList& dirtyRects,
						 const void* buffer,
						 int width, int height) OVERRIDE;
	virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects,
		void* share_handle) OVERRIDE;

	virtual void OnCursorChange(CefRefPtr<CefBrowser> browser,
	                            CefCursorHandle cursor,
	                            CursorType type,
	                            const CefCursorInfo& custom_cursor_info) OVERRIDE;

	virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
							   CefRefPtr<CefDragData> drag_data,
							   DragOperationsMask allowed_ops,
							   int x, int y) OVERRIDE{
		return true;
	}


	virtual void OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser,
		TextInputMode input_mode);

	virtual void OnStatusMessage(CefRefPtr<CefBrowser> browser,
		const CefString& value) OVERRIDE;
	virtual bool OnSetFocus(CefRefPtr<CefBrowser> browser,
		FocusSource source) OVERRIDE;
	virtual void OnTakeFocus(CefRefPtr<CefBrowser> browser,
							 bool next) OVERRIDE;

	virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
							const CefKeyEvent& event,
							CefEventHandle os_event) OVERRIDE;

	// CefJSDialogHandler
	// TODO: Callback to UI and optionally handle
	virtual bool OnJSDialog(CefRefPtr<CefBrowser> browser,
										   const CefString& origin_url,
										   JSDialogType dialog_type,
										   const CefString& message_text,
										   const CefString& default_prompt_text,
										   CefRefPtr<CefJSDialogCallback> callback,
										   bool& suppress_message) {
		suppress_message = true;
		DS_LOG_INFO("JS Dialog: " << message_text.ToString() << " from: " << origin_url.ToString());
		return false;
	}

	virtual bool GetAuthCredentials(CefRefPtr<CefBrowser> browser,
									const CefString& origin_url,
									bool isProxy,
									const CefString& host,
									int port,
									const CefString& realm,
									const CefString& scheme,
									CefRefPtr<CefAuthCallback> callback) OVERRIDE;

	void					authRequestCancel(const int browserId);
	void					authRequestContinue(const int browserId, const std::string& username, const std::string& password);

	// Requests the browser to be closed and also clears and related callbacks
	void					closeBrowser(const int browserId);

	// Adds a callback to a list of callbacks for after browsers are created
	void 					addCreatedCallback(void * instancePtr, std::function<void(int)> callback);
	void 					cancelCreation(void * instancePtr);

	// See WebCefCallbacks for info
	void 					addWebCallbacks(const int browserId, ds::web::WebCefCallbacks& callback);

	// Sends some mouse input to the browser
	void 					sendMouseClick(const int browserId, const int x, const int y, const int bttn, const int state, const int clickCount);

	// Sends some mouse wheel input to the browser
	void 					sendMouseWheelEvent(const int browserId, const int x, const int y, const int xDelta, const int yDelta);

	// Sends a key event to the browser
	void 					sendKeyEvent(const int browserId, const int state, int windows_key_code, char character, const bool shiftDown, const bool cntrlDown, const bool altDown, const bool isCharacter);

	// Sends a touch event
	void					sendTouchEvent(const int browserId, const int touchId, const int x, const int y, const int phase);

	// Loads a new URL in the specified browser's main frame
	void 					loadUrl(const int browserId, const std::string& newUrl);

	// Execute JavaScript on this browser
	void 					executeJavascript(const int browserId, const std::string& theJS, const std::string& sourceUrl);

	// Resize the browser. Happens asynchronously, meaning a paint callback will come back later with the actual info
	void 					requestBrowserResize(const int browserId, const ci::ivec2 newSize);

	// Regular browser controls
	void 					goForwards(const int browserId);
	void 					goBackwards(const int browserId);
	void 					reload(const int browserId, const bool ignoreCache);
	void 					stopLoading(const int browserId);

	// Zoom level here is the same as CEF expects: 0.0 = 100% zoom
	void 					setZoomLevel(const int browserId, const double newZoom);
	double 					getZoomLevel(const int browserId);

	// Orphans can be created if the visible instance is released before the browser is created. 
	// We keep those browsers around to be used the next time a browser is requested
	bool 					hasOrphans(){ return !mOrphanedBrowsers.empty(); }
	void 					useOrphan(std::function<void(int)> callback, const std::string startUrl);

	// No browser ID cause cookies are global
	// Url and cookie name are optional. Will delete all if not specified.
	void					deleteCookies(const std::string& url, const std::string& cookieName);
	std::shared_ptr<d3d11::Device>		getD3D11Device() { return mD3d11_device; }

private:

	// switch to a map for faster lookup
	// List of existing browser windows. Only accessed on the CEF UI thread.
	std::map<int, CefRefPtr<CefBrowser>>				mBrowserList;

	// Track the sizes that we want the browsers to be, since resizing is asynchronous
	std::map<int, ci::ivec2>							mBrowserSizes;

	std::map<void *,std::function<void(int)>>			mCreatedCallbacks;
	std::map<int, ds::web::WebCefCallbacks>				mWebCallbacks;

	std::map<int, CefRefPtr<CefAuthCallback>>			mAuthCallbacks;

	// Browsers that were created but their instances were removed before they were used
	std::vector<CefRefPtr<CefBrowser>>					mOrphanedBrowsers;

	// The lock is used to ensure stuff that is immediately returned to the main app thread
	// is synchronized with the rest of CEF
	base::Lock											mLock;
	CefRefPtr<CefRenderHandler>							mAltRenderHandler;
	std::shared_ptr<d3d11::Device>						mD3d11_device;
	

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(WebHandler);
	
};

}
}

#endif  // DS_WEB_PRIVATE_CEF_WEB_HANDLER
