// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef DS_WEB_PRIVATE_CEF_WEB_APP
#define DS_WEB_PRIVATE_CEF_WEB_APP

/// This class has been modified from the CEF example SimpleApp

#include "include/cef_app.h"
#include "web_handler.h"
#include <functional>

namespace ds {
namespace web{


// Implement application-level callbacks for the browser process.
// This is responsible for the initial command-line setup, context initialization, and browser creation.
class WebApp : public CefApp,
	public CefBrowserProcessHandler
{
public:
	WebApp();

	// CefApp methods:
	virtual void OnBeforeCommandLineProcessing(
		const CefString& process_type,
		CefRefPtr<CefCommandLine> command_line) OVERRIDE;

	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
		OVERRIDE{ return this; }


	// CefBrowserProcessHandler methods:
	virtual void OnContextInitialized() OVERRIDE;

	void		createBrowser(const std::string& url, void * instancePtr, std::function<void(int)> createdCallback, const bool isTransparent, const bool useGPU = true);

private:
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(WebApp);

	CefRefPtr<WebHandler>	mHandler;
};
}
}

#endif  // DS_WEB_PRIVATE_CEF_WEB_APP
