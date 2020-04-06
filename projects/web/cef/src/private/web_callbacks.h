#pragma once
#ifndef PRIVATE_CEF_WEB_CALLBACKS_H
#define PRIVATE_CEF_WEB_CALLBACKS_H

#include <functional>

namespace ds {
class Engine;

namespace web {

/**
* \class ds::web::WebCefCallbacks
* \brief A wrapper object for all the callbacks from CEF to Web sprites
*/
struct WebCefCallbacks  {
public:
	WebCefCallbacks(){};

	// Gets called when the browser sends new paint info, aka new buffers
	std::function<void(const void *, const int, const int)> mPaintCallback;
	

	// Popups here are most commonly HTML select elements (drop down menus)
	std::function<void(const void *, const int, const int)> mPopupPaintCallback;
	std::function<void(const int xp, const int yp, const int wid, const int height)> mPopupRectCallback;
	std::function<void(const bool showing)>					mPopupShowCallback;

	// The loading state has changed (started or finished loading.
	std::function<void(const bool isLoading, const bool canGoBack, const bool canGoForward, const std::string& newUrl)> mLoadChangeCallback;

	// The title of the page has changed
	std::function<void(const std::wstring&)>				mTitleChangeCallback;

	// Something bad happened!
	std::function<void(const std::string&)>					mErrorCallback;

	std::function<void(const bool)>							mFullscreenCallback;

	std::function<void(const bool isProxy, const std::string& host, const int port, const std::string& realm, const std::string& scheme)>	mAuthCallback;



};

} // namespace web
} // namespace ds

#endif // PRIVATE_CEF_WEB_CALLBACKS_H