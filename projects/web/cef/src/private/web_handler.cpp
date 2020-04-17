#include "stdafx.h"

// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "web_handler.h"

#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include <iostream>
#include <thread>

#include <ds/debug/logger.h>

#include <cinder/app/KeyEvent.h>

namespace {

	ds::web::WebHandler* g_instance = NULL;

}  // namespace

namespace ds {
namespace web {


WebHandler::WebHandler()
{
	DCHECK(!g_instance);
	g_instance = this;
}

WebHandler::~WebHandler() {
	g_instance = NULL;
}

// static
WebHandler* WebHandler::GetInstance() {
	return g_instance;
}

void WebHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
							   const CefString& title) {

	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(findy->second.mTitleChangeCallback){
			findy->second.mTitleChangeCallback(title.ToWString());
		}
	}
}

void WebHandler::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(findy->second.mFullscreenCallback){
			findy->second.mFullscreenCallback(fullscreen);
		}
	}
}

void WebHandler::useOrphan(std::function<void(int)> callback, const std::string startUrl){

	if(mOrphanedBrowsers.empty()){
		return;
	}
	int browserIdentifier = 0;
	{
		// be sure this is locked with other requests to the browser lists
		// NOTE: don't lock loadURL as locks cannot be recursive
		base::AutoLock lock_scope(mLock);
		auto browser = mOrphanedBrowsers.back();
		mOrphanedBrowsers.pop_back();

		browserIdentifier = browser->GetIdentifier();
		mBrowserList[browserIdentifier] = browser;
	}

	loadUrl(browserIdentifier, startUrl);

	if(callback){
		callback(browserIdentifier);
	}	
}

void WebHandler::deleteCookies(const std::string& url, const std::string& cookieName) {
	auto cookieManager = CefCookieManager::GetGlobalManager(nullptr);
	if(cookieManager) {
		cookieManager->DeleteCookies(CefString(url), CefString(cookieName), nullptr);
	}
}

void WebHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();


	int browserIdentifier = browser->GetIdentifier();

	{
		// be sure this is locked with other requests to the browser lists
		base::AutoLock lock_scope(mLock);

		auto findy = mBrowserList.find(browserIdentifier);
		if(findy != mBrowserList.end()){
			DS_LOG_WARNING("Multiple browsers tracked with the same identifier! This could lead to issues. Identifier: " << browserIdentifier);

			// This can be called on any thread
			findy->second->GetHost()->CloseBrowser(true);
			mBrowserList.erase(findy);
		}
	}

	{
		// be sure this is locked with other requests to the browser lists
		base::AutoLock lock_scope(mLock);
		// Ensure that the size is correct (in case identifiers are reused)
		auto findySize = mBrowserSizes.find(browserIdentifier);
		if(findySize != mBrowserSizes.end()){
			mBrowserSizes.erase(findySize);
		}
	}

	if(mCreatedCallbacks.empty()){
		// be sure this is locked with other requests to the browser lists
		base::AutoLock lock_scope(mLock);
		mOrphanedBrowsers.push_back(browser);
	} else {
		{
			// be sure this is locked with other requests to the browser lists
			// Also be sure that the created callback doesn't include the lock to prevent recursive locks
			base::AutoLock lock_scope(mLock);
			mBrowserList[browserIdentifier] = browser;
		}

		// Callbacks will be locked back on the main thread, and not here
		mCreatedCallbacks.begin()->second(browserIdentifier);

		{
			base::AutoLock lock_scope(mLock);
			mCreatedCallbacks.erase(mCreatedCallbacks.begin());
		}
	}
}

bool WebHandler::DoClose(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();

	// Allow the close.
	return false;
}


void WebHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();

	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	// clear any tracked sizes
	auto findySize = mBrowserSizes.find(browser->GetIdentifier());
	if(findySize != mBrowserSizes.end()){
		mBrowserSizes.erase(findySize);
	}

	// Remove from the list of existing browsers.
	for(auto bit = mBrowserList.begin(); bit != mBrowserList.end(); ++bit) {
		if(bit->second->IsSame(browser)) {
			mBrowserList.erase(bit);
			break;
		}
	}
}

// This callback happens when there's a request for a new window, tab, page, etc. 
// Currently, we're rerouting this all to load the new page in the same browser
// Could possibly change this behaviour to send a callback to the UI / client code and open a new instance
bool WebHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, 
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
								  bool* no_javascript_access){
	// This can be called on any thread
	if(!target_url.empty()){
		browser->GetMainFrame()->LoadURL(target_url);
	}
	return true; // true prevents the popup
}

std::string getErrorStringForError(const int errorCode){
	switch(errorCode){
		case(ERR_NONE) : return "None";
		case(ERR_FAILED) : return "Failed";
		case(ERR_ABORTED) : return "Aborted";
		case(ERR_INVALID_ARGUMENT): return "Invalid Argument";
		case(ERR_INVALID_HANDLE): return "Invalid Handle";
		case(ERR_FILE_NOT_FOUND): return "File Not Found";
		case(ERR_TIMED_OUT): return "Timed Out";
		case(ERR_FILE_TOO_BIG): return "File Too Big";
		case(ERR_UNEXPECTED): return "Unexpected";
		case(ERR_ACCESS_DENIED): return "Access Denied";
		case(ERR_NOT_IMPLEMENTED): return "Not Implemented";
		case(ERR_CONNECTION_CLOSED): return "Connection Closed";
		case(ERR_CONNECTION_RESET): return "Connection Reset";
		case(ERR_CONNECTION_REFUSED): return "Connection Refused";
		case(ERR_CONNECTION_ABORTED): return "Connection Aborted";
		case(ERR_CONNECTION_FAILED): return "Connection Failed";
		case(-118) : return "Connection Timed Out";
		case(ERR_NAME_NOT_RESOLVED): return "Name Not Resolved";
		case(ERR_INTERNET_DISCONNECTED): return "Interned Disconnected";
		case(ERR_SSL_PROTOCOL_ERROR): return "SSL Protocol Error";
		case(ERR_ADDRESS_INVALID): return "Address Invalid";
		case(ERR_ADDRESS_UNREACHABLE): return "Address Unreachable";
		case(ERR_SSL_CLIENT_AUTH_CERT_NEEDED): return "SSL Client Auth Cert Needed";
		case(ERR_TUNNEL_CONNECTION_FAILED): return "Tunnel Connection Failed";
		case(ERR_NO_SSL_VERSIONS_ENABLED): return "No SSL Versions Enabled";
		case(ERR_SSL_VERSION_OR_CIPHER_MISMATCH): return "SSL Version Or Cipher Mismatch";
		case(ERR_SSL_RENEGOTIATION_REQUESTED): return "SSL Renegotiation Requested";
		case(ERR_CERT_COMMON_NAME_INVALID): return "CERT Common Name Invalid";
		case(ERR_CERT_DATE_INVALID): return "CERT Date Invalid";
		case(ERR_CERT_AUTHORITY_INVALID): return "CERT Authority Invalid";
		case(ERR_CERT_CONTAINS_ERRORS): return "CERT Contains Errors";
		case(ERR_CERT_NO_REVOCATION_MECHANISM): return "CERT No Revocation Mechanism";
		case(ERR_CERT_UNABLE_TO_CHECK_REVOCATION): return "CERT Unable to Check Revocation";
		case(ERR_CERT_REVOKED): return "CERT Revoked";
		case(ERR_CERT_INVALID): return "CERT Invalid";
		case(ERR_CERT_WEAK_SIGNATURE_ALGORITHM): return "CERT Weak Signature Algorithm";
		case(ERR_CERT_NON_UNIQUE_NAME): return "CERT Non-unique Name";
		case(ERR_CERT_WEAK_KEY): return "CERT Weak Key";
		case(ERR_CERT_NAME_CONSTRAINT_VIOLATION): return "CERT Name Constraint Violation";
		case(ERR_CERT_VALIDITY_TOO_LONG): return "CERT Validity Too Long";
		case(ERR_INVALID_URL): return "Invalid URL";
		case(ERR_DISALLOWED_URL_SCHEME): return "Disallowed URL Scheme";
		case(ERR_UNKNOWN_URL_SCHEME): return "Unknown URL Scheme";
		case(ERR_TOO_MANY_REDIRECTS): return "Too Many Redirects";
		case(ERR_UNSAFE_REDIRECT): return "Unsafe Redirect";
		case(ERR_UNSAFE_PORT): return "Unsafe Port";
		case(ERR_INVALID_RESPONSE): return "Invalid Response";
		case(ERR_INVALID_CHUNKED_ENCODING): return "Invalid Chunked Encoding";
		case(ERR_METHOD_NOT_SUPPORTED): return "Method Not Supported";
		case(ERR_UNEXPECTED_PROXY_AUTH): return "Unexpected Proxy Auth";
		case(ERR_EMPTY_RESPONSE): return "Empty Response";
		case(ERR_RESPONSE_HEADERS_TOO_BIG): return "Response Headers Too Big";
		case(ERR_CACHE_MISS): return "Cache Miss";
		case(ERR_INSECURE_RESPONSE): return "Insecure Response";
	}

	return "Unknown Error";
}

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
	return "data:" + mime_type + ";base64," +
		CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
		.ToString();
}

void WebHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefFrame> frame,
								ErrorCode errorCode,
								const CefString& errorText,
								const CefString& failedUrl) {

	// This callback comes back on the UI thread, so will need to be synchronized to the main app thread
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	// Don't display an error for downloaded files.
	// This is super common and not technically an error or whatever
	if(errorCode == ERR_ABORTED)
		return;

	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(findy->second.mErrorCallback){
			std::stringstream ss;
			ss << "Failed to load URL with error " << getErrorStringForError(errorCode) << " number= " << errorCode;
			findy->second.mErrorCallback(ss.str());
		}
	}

	// make a small page to load with some super fancy css
	std::stringstream ss;
	ss << "<html><head><title>Page failed to load</title></head><style>"
		".container {position: absolute;top: 50%;left: 50%;-moz-transform: translateX(-50%) translateY(-50%);"
		"-webkit-transform: translateX(-50%) translateY(-50%);"
		"transform: translateX(-50%) translateY(-50%);"
		"font-family: Arial, Helvetica, sans-serif;"
		"color:#eeeeee;font-size:xx-large}.small{font-size:medium}"
		"</style><body bgcolor=\"#121212\">"
		"<div class=container><span>"
		"<h1>Sorry!</h1>"
		"<h3>Page failed to load</h3>"
		"<b>URL: </b>";
	ss << failedUrl
	   << "<br/><b>Error:</b> " << getErrorStringForError(errorCode) 
		<< "<br/><br/><span class=small>" << errorText << " (" << errorCode << ")</span>";


	ss << "</span></div></body></html>";
	frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}


void WebHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);
	
	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(findy->second.mLoadChangeCallback){
			findy->second.mLoadChangeCallback(isLoading, canGoBack, canGoForward, browser->GetMainFrame()->GetURL().ToString());
		}
	}
}

void WebHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	rect.x = rect.y = 0;
	// This can be called on any thread
	auto findy = mBrowserSizes.find(browser->GetIdentifier());
	if(findy == mBrowserSizes.end()){
		// initial size before the main app knows about this browser.
		rect.width = 640;
		rect.height = 480;
	} else {
		// The main app knows about this browser and set the size
		rect.width = findy->second.x;
		rect.height = findy->second.y;
	}
}

bool WebHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY){
	screenX = viewX;
	screenY = viewY;
	return true;
}

void WebHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) {
	// This callback comes back on the UI thread (so will need to be synchronized to the main app thread)
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()) {
		if(findy->second.mPopupShowCallback) {
			findy->second.mPopupShowCallback(show);
		}
	}
}

void WebHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) {
	// This callback comes back on the UI thread (so will need to be synchronized to the main app thread)
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()) {
		if(findy->second.mPopupRectCallback) {
			findy->second.mPopupRectCallback(rect.x, rect.y, rect.width, rect.height);
		}
	}
}

void WebHandler::OnPaint(CefRefPtr<CefBrowser> browser,
							PaintElementType type, 
							const RectList& dirtyRects, 
							const void* buffer, int width, int height){
	// This callback comes back on the UI thread (so will need to be synchronized to the main app thread)
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	//TODO: Handle dirty rects

	int browserId = browser->GetIdentifier();

	//std::cout << "OnPaint, " << browserId << " type: " << type << " " << width << " " << height << std::endl;

	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(type == PaintElementType::PET_VIEW) {
			if(findy->second.mPaintCallback) {
				//void * newBuffer = new unsigned char[width * height * 4];
				//memcpy(newBuffer, buffer, width * height * 4);
				//findy->second.mPaintCallback(newBuffer, width, height);
				findy->second.mPaintCallback(buffer, width, height);
			}
		} else if(type == PaintElementType::PET_POPUP) {
			if(findy->second.mPopupPaintCallback) {
				findy->second.mPopupPaintCallback(buffer, width, height);
			}
		}
	}
}

void WebHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, CursorType type, const CefCursorInfo& custom_cursor_info){
	// This is when it changes from a hand to a pointer, to a directional arrow, etc.
//	std::cout << "On Cursor change : " << type << std::endl;
}


void WebHandler::OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser, TextInputMode input_mode) {
	// throw up a keyboard based on teh input mode
	// or really, send a callback to the other part of the app to pop it up
	//std::cout << "OnVirtualKeyboardRequested " << input_mode << std::endl;
}

void WebHandler::OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value){
	//std::cout << "Status message: " << value.ToString() << std::endl;
}

bool WebHandler::OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source){
	//std::cout << "On set focus" << std::endl;
	return false;
}

void WebHandler::OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next){
//	std::cout << "On Take Focus " << next << std::endl;
}

bool WebHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event){
//	std::cout << "On Key Event: " << event.focus_on_editable_field << std::endl;
	return false;
}

// This comes from the IO thread, so it needs to be locked with the main thread 
bool WebHandler::GetAuthCredentials(CefRefPtr<CefBrowser> browser, const CefString& origin_url, bool isProxy, const CefString& host, int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);
	bool handled = false;
	int browserId = browser->GetIdentifier();
	auto findy = mWebCallbacks.find(browserId);
	if(findy != mWebCallbacks.end()){
		if(findy->second.mAuthCallback){
			mAuthCallbacks[browserId] = callback;
			findy->second.mAuthCallback(isProxy, host.ToString(), port, realm.ToString(), scheme.ToString());
			handled = true;
		}
	}

	return handled;
}

void WebHandler::authRequestCancel(const int browserId){

	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);
	auto findy = mAuthCallbacks.find(browserId);
	if(findy != mAuthCallbacks.end()){
		findy->second->Cancel();
		mAuthCallbacks.erase(findy);
	}
}

void WebHandler::authRequestContinue(const int browserId, const std::string& username, const std::string& password){

	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);
	auto findy = mAuthCallbacks.find(browserId);
	if(findy != mAuthCallbacks.end()){
		findy->second->Continue(CefString(username), CefString(password));
		mAuthCallbacks.erase(findy);
	}
}

void WebHandler::closeBrowser(const int browserId){

	CefRefPtr<CefBrowserHost> browserHost = nullptr;

	{
		// be sure this is locked with other requests to the browser list
		base::AutoLock lock_scope(mLock);

		auto findyS = mBrowserSizes.find(browserId);
		if(findyS != mBrowserSizes.end()){
			mBrowserSizes.erase(findyS);
		}

		auto findyP = mWebCallbacks.find(browserId);
		if(findyP != mWebCallbacks.end()){
			mWebCallbacks.erase(findyP);
		}

		auto findy = mBrowserList.find(browserId);
		if(findy != mBrowserList.end()){
			browserHost = findy->second->GetHost();
			mBrowserList.erase(findy);
		}
	}

	if(browserHost){
		// This can be called on any thread
		browserHost->CloseBrowser(true);
	}
}

void WebHandler::addCreatedCallback(void * instancePtr, std::function<void(int)> callback){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	mCreatedCallbacks[instancePtr] = callback;
}

void WebHandler::cancelCreation(void * instancePtr){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	// When the browser is finally created (asynchronously), then it will find the creation requests empty and be closed
	auto findy = mCreatedCallbacks.find(instancePtr);
	if(findy != mCreatedCallbacks.end()){
		mCreatedCallbacks.erase(findy);
	}
}

void WebHandler::addWebCallbacks(int browserId, ds::web::WebCefCallbacks& callbacks){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	mWebCallbacks[browserId] = callbacks;
}

void WebHandler::sendMouseClick(const int browserId, const int x, const int y, const int bttn, const int state, const int clickCount){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	if(mBrowserList.empty()) return;

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		CefMouseEvent mouseEvent;
		mouseEvent.x = x;
		mouseEvent.y = y;

		auto browserHost = findy->second->GetHost();

		CefBrowserHost::MouseButtonType btnType = (bttn == 0 ? MBT_LEFT : (bttn == 2 ? MBT_RIGHT : MBT_MIDDLE));
		if(state == 0){
			// This can be called on any thread
			browserHost->SendMouseClickEvent(mouseEvent, btnType, false, clickCount);
		} else if(state == 1){
			// This can be called on any thread
			browserHost->SendMouseMoveEvent(mouseEvent, false);
		} else {
			// This can be called on any thread
			browserHost->SendMouseClickEvent(mouseEvent, btnType, true, clickCount);
		}
	} else if(browserId >= 0) {
		DS_LOG_WARNING("Browser not found in list to sendMouseClick to! BrowserId=" << browserId);
	}	
}

void WebHandler::sendMouseWheelEvent(const int browserId, const int x, const int y, const int xDelta, const int yDelta){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	if(mBrowserList.empty()) return;

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		auto browserHost = findy->second->GetHost();
		CefMouseEvent mouseEvent;
		mouseEvent.x = x;
		mouseEvent.y = y;

		// This can be called on any thread
		browserHost->SendMouseWheelEvent(mouseEvent, xDelta, yDelta);
	} else if(browserId >= 0) {
		DS_LOG_WARNING("Browser not found in list to sendMouseWheel to! BrowserId=" << browserId);
	}
}

void WebHandler::sendTouchEvent(const int browserId, const int touchId, const int x, const int y, const int phase) {
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	if (mBrowserList.empty()) return;

	auto findy = mBrowserList.find(browserId);
	if (findy != mBrowserList.end()) {
		auto browserHost = findy->second->GetHost();
		CefTouchEvent touchEvent;
		touchEvent.id = touchId;
		touchEvent.x = x;
		touchEvent.y = y;

		if (phase == 0) {
			touchEvent.type = CEF_TET_PRESSED;
		} else if (phase == 1) {
			touchEvent.type = CEF_TET_MOVED;
		} else {
			touchEvent.type = CEF_TET_RELEASED;
		}


		// This can be called on any thread
		browserHost->SendTouchEvent(touchEvent);
	} else if (browserId >= 0) {
		DS_LOG_WARNING("Browser not found in list to sendMouseWheel to! BrowserId=" << browserId);
	}

}


void WebHandler::sendKeyEvent(const int browserId, const int state, int windows_key_code, char character, const bool shiftDown, const bool cntrlDown, const bool altDown, const bool isCharacter){
	// be sure this is locked with other requests to the browser list
	base::AutoLock lock_scope(mLock);

	CefKeyEvent keyEvent;

	bool isChar = false;
	keyEvent.type = KEYEVENT_RAWKEYDOWN;
#ifdef WIN32
    keyEvent.native_key_code = MapVirtualKey(windows_key_code, MAPVK_VK_TO_VSC);
#else
    keyEvent.native_key_code = windows_key_code;
#endif

	switch(windows_key_code) {
		// These keys are non-character keys and have different windows_key_codes from the character
		// May need to add more codes to this list, or modify as time goes on
        case ci::app::KeyEvent::KEY_BACKSPACE:
        case ci::app::KeyEvent::KEY_TAB:
		case ci::app::KeyEvent::KEY_ESCAPE:

		{
			keyEvent.windows_key_code = windows_key_code;
			break;
		}
		case ci::app::KeyEvent::KEY_F1:			keyEvent.windows_key_code = 112; break;
		case ci::app::KeyEvent::KEY_F2:			keyEvent.windows_key_code = 113; break;
		case ci::app::KeyEvent::KEY_F3:			keyEvent.windows_key_code = 114; break;
		case ci::app::KeyEvent::KEY_F4:			keyEvent.windows_key_code = 115; break;
		case ci::app::KeyEvent::KEY_F5:			keyEvent.windows_key_code = 116; break;
		case ci::app::KeyEvent::KEY_F6:			keyEvent.windows_key_code = 117; break;
		case ci::app::KeyEvent::KEY_F7:			keyEvent.windows_key_code = 118; break;
		case ci::app::KeyEvent::KEY_F8:			keyEvent.windows_key_code = 119; break;
		case ci::app::KeyEvent::KEY_F9:			keyEvent.windows_key_code = 120; break;
		case ci::app::KeyEvent::KEY_F10:		keyEvent.windows_key_code = 121; break;
		case ci::app::KeyEvent::KEY_F11:		keyEvent.windows_key_code = 122; break;
		case ci::app::KeyEvent::KEY_F12:		keyEvent.windows_key_code = 123; break;
		case ci::app::KeyEvent::KEY_DELETE:		keyEvent.windows_key_code = 46; break;
		case ci::app::KeyEvent::KEY_HOME:		keyEvent.windows_key_code = 36; break;
		case ci::app::KeyEvent::KEY_END:		keyEvent.windows_key_code = 35; break;
		case ci::app::KeyEvent::KEY_PAGEDOWN:	keyEvent.windows_key_code = 34; break;
		case ci::app::KeyEvent::KEY_PAGEUP:		keyEvent.windows_key_code = 33; break;
		case ci::app::KeyEvent::KEY_LEFT:		keyEvent.windows_key_code = 37; break;
		case ci::app::KeyEvent::KEY_RIGHT:		keyEvent.windows_key_code = 39; break;
		case ci::app::KeyEvent::KEY_UP:			keyEvent.windows_key_code = 38; break;
		case ci::app::KeyEvent::KEY_DOWN:		keyEvent.windows_key_code = 40; break;

		default:
			isChar = true;
			keyEvent.windows_key_code = character;
	}

	keyEvent.modifiers = 0;

	if(shiftDown){
		keyEvent.modifiers |= EVENTFLAG_SHIFT_DOWN;
	}

	if(cntrlDown){
		keyEvent.modifiers |= EVENTFLAG_CONTROL_DOWN;
	}

	if(altDown){
		keyEvent.modifiers |= EVENTFLAG_ALT_DOWN;
	}

	// Assume any keys targetted for the number pad (NUMPAD0-9 and Plus/minus/divide, etc) can be characters
	keyEvent.modifiers |= EVENTFLAG_NUM_LOCK_ON;
	keyEvent.modifiers |= EVENTFLAG_IS_KEY_PAD;


	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){

		auto browserHost = findy->second->GetHost();


		if(state == 0){
			if(isCharacter && isChar){
				keyEvent.type = KEYEVENT_CHAR;
			} else {
				keyEvent.type = KEYEVENT_KEYDOWN;
			}

			//std::cout << "Key down: " << isChar << " " << character << " " << keyEvent.windows_key_code << " " << keyEvent.native_key_code << std::endl;

			// This can be called on any thread
			browserHost->SendKeyEvent(keyEvent);

		} else {
			keyEvent.type = KEYEVENT_KEYUP;

			//std::cout << "Key up: " << isChar << " " << character << " " << keyEvent.windows_key_code << " " << keyEvent.native_key_code << std::endl;
			// This can be called on any thread
			browserHost->SendKeyEvent(keyEvent);
		}
	} else if(browserId >= 0) {
		DS_LOG_WARNING("Couldn't find the correct browser to send key event to! BrowserId = " << browserId);
	}
}

void WebHandler::loadUrl(const int browserId, const std::string& newUrl){
	if(newUrl.empty()) return;

	CefRefPtr<CefFrame> mainFrame = nullptr;

	{
		// be sure this is locked with other requests to the browser list
		// Grab the main frame in the lock, but call LoadURL outside the lock to avoid recursive locks
		base::AutoLock lock_scope(mLock);

		auto findy = mBrowserList.find(browserId);
		if(findy != mBrowserList.end()){
			// This can be called on any thread
			mainFrame = findy->second->GetMainFrame();
		}
	}

	if(mainFrame){
		// This can be called on any thread
		mainFrame->LoadURL(CefString(newUrl));
	}
}

void WebHandler::executeJavascript(const int browserId, const std::string& theJS, const std::string& sourceUrl) {
	if(theJS.empty()) return;

	CefRefPtr<CefFrame> mainFrame = nullptr;

	{
		// be sure this is locked with other requests to the browser list
		// Grab the main frame in the lock, but call LoadURL outside the lock to avoid recursive locks
		base::AutoLock lock_scope(mLock);

		auto findy = mBrowserList.find(browserId);
		if(findy != mBrowserList.end()) {
			// This can be called on any thread
			mainFrame = findy->second->GetMainFrame();
		}
	}

	if(mainFrame) {
		// This can be called on any thread
		mainFrame->ExecuteJavaScript(CefString(theJS), CefString(sourceUrl), 0);
	}
}

void WebHandler::requestBrowserResize(const int browserId, const ci::ivec2 newSize){
	// be sure this is locked with other requests to the browser lists
	{
		base::AutoLock lock_scope(mLock);
		mBrowserSizes[browserId] = newSize;
	}

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		// This tells the browser to get the view size, in which case it will look up the browser size in the mBrowserSizes map and resize accordingly, then repaint
		// This can be called on any thread
		findy->second->GetHost()->WasResized();
	}

}

void WebHandler::goForwards(const int browserId){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		// This can be called on any thread
		findy->second->GoForward();
	}
}

void WebHandler::goBackwards(const int browserId){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		// This can be called on any thread
		findy->second->GoBack();
	}
}

void WebHandler::reload(const int browserId, const bool ignoreCache){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		if(ignoreCache){
			// This can be called on any thread
			findy->second->ReloadIgnoreCache();
		} else {
			// This can be called on any thread
			findy->second->Reload();
		}
	}
}

void WebHandler::stopLoading(const int browserId){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		// StopLoad() can be called on any thread
		findy->second->StopLoad();
	}
}

double WebHandler::getZoomLevel(const int browserId){
	// Ensure this thread is locked with the rest of CEF, as GetZoomLevel requires the UI thread
	base::AutoLock lock_scope(mLock);

//	CEF_REQUIRE_UI_THREAD();
	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		return findy->second->GetHost()->GetZoomLevel();
	}
	
	return 0.0;
}

void WebHandler::setZoomLevel(const int browserId, const double newZoom){
	// be sure this is locked with other requests to the browser lists
	base::AutoLock lock_scope(mLock);

	//CEF_REQUIRE_UI_THREAD();
	auto findy = mBrowserList.find(browserId);
	if(findy != mBrowserList.end()){
		// SetZoomLevel can be called on any thread
		findy->second->GetHost()->SetZoomLevel(newZoom);
	}
}

}
}
