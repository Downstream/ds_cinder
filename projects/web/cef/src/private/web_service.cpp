#include "stdafx.h"

#include "private/web_service.h"

#include <ds/app/engine/engine.h>
#include <ds/ui/sprite/web.h>
#include <ds/app/environment.h>

#include <chrono>
#include <thread>

#include "include/cef_app.h"
#include "web_handler.h"

// On linux, we need to shutdown CEF by posting a Quit task to the RunMessageLoop thread
#ifndef _WIN32
#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
namespace {
void shutdownCefCallbackFunc() {
	CefQuitMessageLoop();
}
} // anonymous namespace
#endif

namespace ds {
namespace web {

/**
 * \class ds::web::Service
 */
WebCefService::WebCefService(ds::Engine& e)
	: ds::AutoUpdate(e, AutoUpdateType::SERVER | AutoUpdateType::CLIENT)
{
	mEngine.registerSpriteImporter("web", [this](ds::ui::SpriteEngine& engine)->ds::ui::Sprite*{
		return new ds::ui::Web(mEngine);
	});

	mEngine.registerSpritePropertySetter("web_url", [this](ds::ui::Sprite& bs, const std::string& theValue, const std::string& fileReferrer){
		ds::ui::Web* webby = dynamic_cast<ds::ui::Web*>(&bs);
		if(!webby){
			DS_LOG_WARNING("Tried to set the web_url of a non-ds::ui::Web sprite. Ignoring!");
			return;
		}

		webby->setUrl(ds::Environment::expand(theValue));
	});


}

WebCefService::~WebCefService() {
	try{
#ifdef _WIN32
		CefShutdown();
#else
		bool quitMessageLoopSuccess = false;
		const int MAX_TRIES=10;
		for (int i=0; i<MAX_TRIES; i++) {
			if (CefPostTask(TID_UI, base::Bind(&shutdownCefCallbackFunc))) {
				quitMessageLoopSuccess = true;
				break;
			}
			else {
				DS_LOG_WARNING( "WebCefService: CefQuitMessageLoop message was not successfully sent.  Trying " << (MAX_TRIES-i-1) << " more times..." );
				std::this_thread::sleep_for( std::chrono::milliseconds(100) );
			}
		}

		if (mCefMessageLoopThread) {
			if (quitMessageLoopSuccess)
				mCefMessageLoopThread->join();
			else {
				DS_LOG_WARNING( "WebCefService did not shut down cleanly.  Abandoning CEF message loop thread." );
				mCefMessageLoopThread->detach();
			}
		}
#endif
	} catch(...){
		DS_LOG_WARNING("WebCefService destructor exception");
	}
}

void WebCefService::start() {
	CefEnableHighDPISupport();

	mCefSimpleApp = CefRefPtr<WebApp>(new WebApp);

	// Register CDM. This is content decryption (aka DRM)
	// I couldn't actually get this working. You're supposed to copy the manifest.json and widevine dll's from your chrome directory
	// But for whatever reason, Netflix and Hulu still complain about missing stuff.
	// Version mismatch? I dunno. Anyways, this is here.
	//CefString thePath = CefString(ds::Environment::expand("%APP%").c_str());
	//CefRegisterWidevineCdm(thePath, NULL);

#ifdef _WIN32
	CefMainArgs main_args(GetModuleHandle(NULL));

	/*
	int exit_code = CefExecuteProcess(main_args, NULL, sandbox_info);
	if(exit_code >= 0){
		DS_LOG_WARNING("CEF setup exit code is not the expected value! Code: " << exit_code);
	}
	*/

	CefSettings settings;
	// There's no sandboxing for IO requests
	settings.no_sandbox = true;

	// probably want to persist cookies
	//settings.persist_session_cookies = true;

	// Single process could be used for debugging, but it's much slower and not-production ready
	// No longer supported
	//settings.single_process = false;

	// CEF handles threading for the message loop (required for performance, otherwise slow apps can become deadlocked)
	settings.multi_threaded_message_loop = true;

	// We're using Offscreen Rendering.
	settings.windowless_rendering_enabled = true;

	settings.background_color = CefColorSetARGB(255, 255, 255, 255);

	//settings.command_line_args_disabled = true;

	// CEF's multi-process structure: it's required.
	// There's basically another process that needs to be spawned for different things (rendering, plugins, IO, etc)
	// So we create a small exe that's just there to run CEF. 
	// It's based on cefsimple, from the CEF binary distribution, in case you need to recompile:
	// Remove most of the stuff in cefsimple_win.cc and set it's subsystem in the linker settings to WINDOWS
	// Here's the entirety of the source for that app:
	/*
	#include <windows.h>
	#include <include/cef_app.h>
	int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
	){
		CefMainArgs main_args;
		return CefExecuteProcess(main_args, NULL, NULL);
	}
	*/

	// This requires cefsimple.exe to be in the current working directory
	const char* path = "cefsimple.exe";
	CefString(&settings.browser_subprocess_path).FromASCII(path);

	const std::string thePath = ds::Environment::expand("%LOCAL%/cache/browser_cache/");
	const char* cachePath = thePath.c_str();
	CefString(&settings.cache_path).FromASCII(cachePath);

	CefInitialize(main_args, settings, mCefSimpleApp.get(), NULL);

#else
	// Linux CEF startup on separate thread
	mCefMessageLoopThread.reset( new std::thread([this]{
		// Collect command-line parameters to pass to cefsimple
		auto params = ds::Environment::getCommandLineParams();
		char* argv[params.size()];
		int i=0;
		for (const auto &param : params ) {
			argv[i++] = strdup( param.c_str() );
		}
		CefMainArgs main_args(params.size(), argv);

		// Setup Cef settings
		CefSettings settings;
		settings.no_sandbox = true;
		settings.single_process = false;
		// No multi_threaded_message_loop support in Linux
		settings.multi_threaded_message_loop = false;
		settings.windowless_rendering_enabled = true;

		// Setup paths
		// This requires cefsimple executable to be in same directory as the app executable.
		auto cef_dir = ci::app::Platform::get()->getExecutablePath().generic_string();
		//DS_LOG_INFO( "CEF Directory: " << cef_dir );
		CefString(&settings.resources_dir_path)		.FromString(cef_dir);
		CefString(&settings.locales_dir_path)		.FromString(cef_dir + "/locales");
		CefString(&settings.browser_subprocess_path).FromString(cef_dir + "/cefsimple");

		CefInitialize(main_args, settings, mCefSimpleApp.get(), NULL);

		// Delete strings allocated for command-line params
		for (int i=0; i<params.size(); i++)
			delete argv[i];

		CefRunMessageLoop();
		DS_LOG_INFO("CEF Message Loop terminated.");
		CefShutdown();
	}));
#endif
}

void WebCefService::update(const ds::UpdateParams&) {
	//CefDoMessageLoopWork();
}

void WebCefService::createBrowser(const std::string& startUrl, void * instancePtr, std::function<void(int)> createdCallback, const bool isTransparent, const bool useGPU){
	if(mCefSimpleApp){
		try{
			mCefSimpleApp->createBrowser(startUrl, instancePtr, createdCallback, isTransparent, useGPU);
		} catch(std::exception& e){
			DS_LOG_WARNING("WebCefService: Exception creating browser: " << e.what());
		}
	}
}

void WebCefService::cancelCreation(void * instancePtr){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->cancelCreation(instancePtr);
	}
}

void WebCefService::closeBrowser(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->closeBrowser(browserId);
	}
}

void WebCefService::addWebCallbacks(int browserId, WebCefCallbacks& callbacks){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->addWebCallbacks(browserId, callbacks);
	}
}

void WebCefService::sendMouseClick(const int browserId, const int x, const int y, const int bttn, const int state, const int clickCount){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->sendMouseClick(browserId, x, y, bttn, state, clickCount);
	}
}

void WebCefService::sendMouseWheelEvent(const int browserId, const int x, const int y, const int xDelta, const int yDelta){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->sendMouseWheelEvent(browserId, x, y, xDelta, yDelta);
	}
}

void WebCefService::sendTouchEvent(const int browserId, const int touchId, const int x, const int y, const int phase) {
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if (handler) {
		handler->sendTouchEvent(browserId, touchId, x, y, phase);
	}
}

void WebCefService::sendKeyEvent(const int browserId, const int state, int windows_key_code, char character, const bool shiftDown, const bool cntrlDown, const bool altDown, const bool isCharacter){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->sendKeyEvent(browserId, state, windows_key_code, character, shiftDown, cntrlDown, cntrlDown, isCharacter);
	}
}

void WebCefService::loadUrl(const int browserId, const std::string& newUrl){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->loadUrl(browserId, newUrl);
	}

}

void WebCefService::executeJavascript(const int browserId, const std::string& theJS, const std::string& debugUrl) {
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler) {
		handler->executeJavascript(browserId, theJS, debugUrl);
	}
}

void WebCefService::requestBrowserResize(const int browserId, const ci::ivec2 newSize){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->requestBrowserResize(browserId, newSize);
	}
}

void WebCefService::goForwards(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->goForwards(browserId);
	}
}

void WebCefService::goBackwards(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->goBackwards(browserId);
	}
}

void WebCefService::reload(const int browserId, const bool ignoreCache){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->reload(browserId, ignoreCache);
	}
}

void WebCefService::stopLoading(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->stopLoading(browserId);
	}
}

void WebCefService::setZoomLevel(const int browserId, const double newZoomLevel){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->setZoomLevel(browserId, newZoomLevel);
	}
}

double WebCefService::getZoomLevel(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		return handler->getZoomLevel(browserId);
	}

	return 0.0;
}

void WebCefService::authCallbackCancel(const int browserId){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->authRequestCancel(browserId);
	}
}

void WebCefService::authCallbackContinue(const int browserId, const std::string& username, const std::string& password){
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler){
		handler->authRequestContinue(browserId, username, password);
	}
}

void WebCefService::deleteCookies(const std::string& url, const std::string& cookies) {
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if(handler) {
		handler->deleteCookies(url, cookies);
	}
}

std::shared_ptr<d3d11::Device> WebCefService::getD3D11Device()
{
	CefRefPtr<WebHandler> handler(WebHandler::GetInstance());
	if (handler) {
		return handler->getD3D11Device();
	} else
	{
		return nullptr;
	}
}
} // namespace web
} // namespace ds
