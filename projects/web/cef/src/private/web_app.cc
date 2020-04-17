#include "stdafx.h"

// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "web_app.h"

#include <string>

#include "web_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include <ds/debug/debug_defines.h>
#include <ds/debug/logger.h>
#include <cinder/app/App.h>

// For getting X11 window handle from app window
#ifndef _WIN32
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#endif

namespace ds{
namespace web{

WebApp::WebApp() {
}

#include <iostream>
void WebApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line){

	// This could prevent a crash in debug when closing (but sometimes doesn't):
	// Seems like with proper multi-processing, this doesn't happen anymore.
	// But it's here for posterity, in case you ever wanna disable stuff randomly
	//command_line->AppendSwitch(CefString("disable-extensions"));

	command_line->AppendSwitchWithValue(CefString("enable-system-flash"), CefString("1"));
	//command_line->AppendSwitch("enable-gpu");
	//command_line->AppendSwitch("enable-gpu-compositing");
	//command_line->AppendSwitch("disable-gpu");
	//command_line->AppendSwitch("disable-gpu-compositing");

	command_line->AppendSwitch("enable-media-stream");
	command_line->AppendSwitch("enable-speech-input");
	command_line->AppendSwitch("enable-usermedia-screen-capture");

	// Some docs online say to use these two, but I had bad results with these
	//command_line->AppendSwitch("disable-surfaces");
	//command_line->AppendSwitch("enable-begin-frame-scheduling");

	//command_line->AppendSwitch("off-screen-rendering-enabled");
	command_line->AppendSwitchWithValue("off-screen-frame-rate", "60");
	command_line->AppendSwitchWithValue(CefString("touch-optimized-ui"),CefString("enabled"));
}

void WebApp::OnContextInitialized() {
	CEF_REQUIRE_UI_THREAD();

	/* in case you need to check what's been enabled or disabled
	CefRefPtr<CefCommandLine> command_line =
		CefCommandLine::GetGlobalCommandLine();

	const bool disable_extensions = command_line->HasSwitch("disable-extensions");
	*/

	// SimpleHandler implements browser-level callbacks.
	mHandler = CefRefPtr<WebHandler>(new WebHandler());
}

void WebApp::createBrowser(const std::string& url, void * instancePtr, std::function<void(int)> createdCallback, const bool isTransparent){

	// Handler has an unused browser instance, so use that instead of creating a new one
	if(mHandler && mHandler->hasOrphans()){
		mHandler->useOrphan(createdCallback, url);
		return;
	}

	// Specify CEF browser settings here.
	CefBrowserSettings browser_settings;
	browser_settings.windowless_frame_rate = 60;

	// TODO: make a setting
	//browser_settings.web_security = STATE_DISABLED;


	// On Windows, ci::Window::getNative() returns a HWND, cast as void*.
	// On Linux, ci::Window::getNative() returns a GLFWwindow*, cast as void*.
	// On Windows, CefWindowHandle is typedef'ed as HWND
	// On Linux CefWindowHandle is typedef'ed as unsigned long 
	// In either case, this value will be 64-bit or 32-bit depending on target.
	// On Windows, we can use the HWND directly, but on Linux, we need to get
	// the X11 Window ID from the GLFWwindow.
#ifdef _WIN32
	CefWindowHandle window = (CefWindowHandle)ci::app::getWindow()->getNative();
#else
	CefWindowHandle window = (CefWindowHandle)::glfwGetX11Window( (GLFWwindow*)ci::app::getWindow()->getNative() );
#endif

	// Information used when creating the native window.
	CefWindowInfo window_info;
	window_info.SetAsWindowless(window);// , isTransparent);

	if(mHandler){
		mHandler->addCreatedCallback(instancePtr, createdCallback);
		// Create the first browser window.
		CefBrowserHost::CreateBrowser(window_info, mHandler, url, browser_settings, NULL, NULL);
	} else {
		DS_LOG_WARNING("No handler exists when trying to create a browser!");
	}

}


}
}
