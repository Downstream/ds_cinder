#include "stdafx.h"

#include "ds/app/engine/engine.h"
#include "ds/app/app.h"
#include "ds/app/auto_draw.h"
#include "ds/app/environment.h"
#include "ds/app/engine/engine_roots.h"
#include "ds/app/engine/engine_service.h"
#include "ds/app/engine/engine_stats_view.h"
#include "ds/cfg/settings.h"
#include "ds/cfg/settings_editor.h"
#include "ds/ui/touch/tuio_input.h"

#ifdef _WIN32
#include <Winuser.h>
#include <VersionHelpers.h>
#include "ds/debug/console.h"
#endif

#include "ds/debug/debug_defines.h"
#include "ds/debug/logger.h"
#include "ds/math/math_defs.h"
#include "ds/metrics/metrics_service.h"
#include "ds/ui/touch/draw_touch_view.h"
#include "ds/ui/touch/touch_event.h"
#include "ds/util/file_meta_data.h"

#include <cinder/Display.h>
#include <boost/algorithm/string.hpp>
#include <cinder/tuio/Tuio.h>

#include "engine_events.h"

//! This entire header is included for one single
//! function Poco::Path::expand. This slowly needs
//! to get removed. Poco is not part of the Cinder.
#include <Poco/Path.h>

//#include <boost/algorithm/string/predicate.hpp>

#pragma warning (disable : 4355)    // disable 'this': used in base member initializer list

namespace {
void				root_setup(std::vector<std::unique_ptr<ds::EngineRoot>>&);


#ifdef _WIN32
ds::Console				GLOBAL_CONSOLE;
#endif

}

const ds::BitMask	ds::ENGINE_LOG = ds::Logger::newModule("engine");

namespace ds {

const int Engine::NumberOfNetworkThreads = 2;

Engine::Engine(ds::App& app, ds::EngineSettings &settings,
			   ds::EngineData& ed, const RootList& _roots, const int appMode)
	: ds::ui::SpriteEngine(ed, appMode)
	, mRequestedRootList(_roots)
	, mDsApp(app)
	, mTweenline(app.timeline())
	, mIdling(true)
	, mTouchMode(ds::ui::TouchMode::kTuioAndMouse)
	, mTouchManager(*this, mTouchMode)
	, mLoadImageService(*this)
	, mPangoFontService(*this)
	, mSettings(settings)
	, mSettingsEditor(nullptr)
	, mTouchBeginEvents(mTouchMutex,	mLastTouchTime,  [&app, this](const ds::ui::TouchEvent& e) {app.onTouchesBegan(e); this->mTouchManager.touchesBegin(e);}, "touchbegin")
	, mTouchMovedEvents(mTouchMutex,	mLastTouchTime,  [&app, this](const ds::ui::TouchEvent& e) {app.onTouchesMoved(e); this->mTouchManager.touchesMoved(e);}, "touchmoved")
	, mTouchEndedEvents(mTouchMutex,	mLastTouchTime,  [&app, this](const ds::ui::TouchEvent& e) {app.onTouchesEnded(e); this->mTouchManager.touchesEnded(e);}, "touchend")
	, mMouseBeginEvents(mTouchMutex,	mLastTouchTime,  [this](const MousePair& e)  {handleMouseTouchBegin(e.first, e.second);}, "mousebegin")
	, mMouseMovedEvents(mTouchMutex,	mLastTouchTime,  [this](const MousePair& e)  {handleMouseTouchMoved(e.first, e.second);}, "mousemoved")
	, mMouseEndedEvents(mTouchMutex,	mLastTouchTime,  [this](const MousePair& e)  {handleMouseTouchEnded(e.first, e.second);}, "mouseend")
	, mTuioObjectsBegin(mTouchMutex,	mLastTouchTime,  [&app](const TuioObject& e) {app.tuioObjectBegan(e);}, "tuiobegin")
	, mTuioObjectsMoved(mTouchMutex,	mLastTouchTime,  [&app](const TuioObject& e) {app.tuioObjectMoved(e);}, "tuiomoved")
	, mTuioObjectsEnded(mTouchMutex,	mLastTouchTime,  [&app](const TuioObject& e) {app.tuioObjectEnded(e);}, "tuioend")
	, mAutoHideMouse(true)
	, mHideMouse(false)
	, mShowConsole(false)
	, mUniqueColor(0, 0, 0)
	, mAutoDraw(new AutoDrawService())
	, mCachedWindowW(0)
	, mCachedWindowH(0)
	, mAverageFps(0.0f)
	, mTuioInput(std::make_shared<ds::ui::TuioInput>(*this, mTuioPort, ci::vec2(1), ci::vec2(0), 0.0f, 0, ci::Rectf(ci::vec2(0), ci::vec2(0))))
	, mFonts(*this)
	, mEventClient(ed.mNotifier, [this](const ds::Event *m){ if(m) onAppEvent(*m); })
	, mAutoRefresh(*this)
{

	ds::event::Registry::get().addEventCreator(ds::app::RequestAppExitEvent::NAME(), [this]()->ds::Event* {return new ds::app::RequestAppExitEvent(); });
	ds::event::Registry::get().addEventCreator(ds::app::IdleEndedEvent::NAME(), [this]()->ds::Event* {return new ds::app::IdleEndedEvent(); });
	ds::event::Registry::get().addEventCreator(ds::app::IdleStartedEvent::NAME(), [this]()->ds::Event* {return new ds::app::IdleStartedEvent(); });
	ds::event::Registry::get().addEventCreator(ds::EngineStatsView::ToggleStatsRequest::NAME(), [this]()->ds::Event* {return new ds::EngineStatsView::ToggleStatsRequest(); });
	ds::event::Registry::get().addEventCreator(ds::EngineStatsView::ToggleHelpRequest::NAME(), [this]()->ds::Event* {return new ds::EngineStatsView::ToggleHelpRequest(); });

	setupEngine();

	if (mAutoDraw) addService("AUTODRAW", *mAutoDraw);

}

Engine::~Engine() {
	// Important to do this here before the auto update list is destructed.
	// so any autoupdate services get removed.
	mData.clearServices();

	hideConsole();
}

void Engine::setupEngine() {
	setupConsole();
	setupLogger();
	setupFrameRate();
	setupVerticalSync();
	setupWindowMode();
	setupMouseHide();
	setupWorldSize();
	setupSrcDstRects();
	setupAutoSpan();
	setupRoots();
	setupMute();
	setupResourceLocation();
	setupIdleTimeout();
	setupMetrics();
	setupAutoRefresh();
}

void Engine::setupLogger() {

	ds::Logger::setup(mSettings);

	mData.mAppInstanceName = mSettings.getString("platform:guid");

	ds::Environment::setConfigDirFileExpandOverride(mSettings.getBool("configuration_folder:allow_expand_override"));
}

void Engine::setupWorldSize(){
	mData.mWorldSize = mSettings.getVec2("world_dimensions");
}

void Engine::setupSrcDstRects(){
	// Src rect and dst rect are new, and should obsolete local_rect. For now, default to illegal values,
	// which makes them get ignored and default to the main display

	auto screenMode = mSettings.getString("screen:mode");
	ds::to_lowercase(screenMode);
	bool isFullscreen = screenMode.find("full") != std::string::npos;

	auto autoSizeMode = mSettings.getString("screen:auto_size");
	ds::to_lowercase(autoSizeMode);

	if(autoSizeMode == "all_span"){
		mSettings.getSetting("span_all_displays", 0).mRawValue = "true";
		return;
	} else if (autoSizeMode == "main_span" || autoSizeMode == "letterbox") {
		ci::DisplayRef mainDisplay = ci::Display::getMainDisplay();
		const ci::Rectf mainDisplayRect = ci::Rectf(0.0f, 0.0f, (float)mainDisplay->getWidth(), (float)mainDisplay->getHeight());
		ci::Rectf newSrcRect = mainDisplayRect;
		ci::Rectf newDstRect = mainDisplayRect;

		if (autoSizeMode == "letterbox"
			&& (mainDisplayRect.getWidth() != mData.mWorldSize.x|| mainDisplayRect.getHeight() != mData.mWorldSize.y)
			&& mData.mWorldSize.x > 0 && mData.mWorldSize.y > 0
			) {

			// a = w / h
			// w = ah
			// h = w / a
			float mainDispAsp = mainDisplayRect.getWidth() / mainDisplayRect.getHeight();
			float worldAsp = mData.mWorldSize.x / mData.mWorldSize.y;
			newSrcRect = ci::Rectf(0.0f, 0.0f, mData.mWorldSize.x, mData.mWorldSize.y);
			
			// same aspect ratio: scale to fill the screen
			if (mainDispAsp == worldAsp) {
				newDstRect = mainDisplayRect;

			// pillarbox
			} else if (mainDispAsp > worldAsp) {
				float newW = mainDisplayRect.getHeight() * worldAsp;
				newDstRect = ci::Rectf(mainDisplayRect.getWidth() / 2.0f - newW / 2.0f, 0.0f, mainDisplayRect.getWidth() / 2.0f + newW / 2.0f, mainDisplayRect.getHeight());

			// letterbox
			} else {
				float newH = mainDisplayRect.getWidth() / worldAsp;
				newDstRect = ci::Rectf(0.0f, mainDisplayRect.getHeight() / 2.0f - newH / 2.0f, mainDisplayRect.getWidth(), mainDisplayRect.getHeight() / 2.0f + newH / 2.0f);

			}
		} 

		/// avoid windows' dumb auto-fullscreen thing
		if (!isFullscreen && newDstRect.getWidth() == mainDisplayRect.getWidth() && newDstRect.getHeight() == mainDisplayRect.getHeight()) {
				
			newSrcRect.x2 += 1;
			newDstRect.x2 += 1;
		}

		mData.mSrcRect = newSrcRect;
		mData.mDstRect = newDstRect;
		if (mData.mWorldSize.x < 1 || mData.mWorldSize.y < 1) {
			mData.mWorldSize = ci::vec2(mainDisplayRect.getWidth(), mainDisplayRect.getHeight());
		}
	} else {
		mData.mSrcRect = mSettings.getRect("src_rect");
		mData.mDstRect = mSettings.getRect("dst_rect");

		if (mData.mDstRect.getWidth() < 1 || mData.mDstRect.getHeight() < 1) {
			DS_LOG_WARNING("Screen rect is 0 width or height. Overriding to full screen size");
			ci::DisplayRef mainDisplay = ci::Display::getMainDisplay();
			ci::Rectf mainDisplayRect = ci::Rectf(0.0f, 0.0f, (float)mainDisplay->getWidth(), (float)mainDisplay->getHeight());
			mData.mSrcRect = mainDisplayRect;
			mData.mDstRect = mainDisplayRect;
			if (mData.mWorldSize.x < 1 || mData.mWorldSize.y < 1) {
				mData.mWorldSize = ci::vec2(mainDisplayRect.getWidth(), mainDisplayRect.getHeight());
			}
		}
	}

	
	ci::app::getWindow()->setPos(mData.mDstRect.getUpperLeft());
	ci::app::getWindow()->setSize(mData.mDstRect.getSize());

	mData.mOriginalSrcRect = mData.mSrcRect;

	DS_LOG_INFO("Screen dst_rect is (" << mData.mDstRect.x1 << ", " << mData.mDstRect.y1 << ") - (" << mData.mDstRect.x2 << ", " << mData.mDstRect.y2 << ")");
}

void Engine::setupAutoSpan() {
	bool autoSpan = mSettings.getBool("span_all_displays");
	if(autoSpan && ci::app::getWindow()) {
		ci::app::getWindow()->spanAllDisplays();
		auto theX = static_cast<float>(ci::app::getWindow()->getPos().x);
		auto theY = static_cast<float>(ci::app::getWindow()->getPos().y);
		mData.mWorldSize.x = static_cast<float>(ci::app::getWindow()->getWidth());
		mData.mWorldSize.y = static_cast<float>(ci::app::getWindow()->getHeight());
		mData.mSrcRect = ci::Rectf(0.0f, 0.0f, mData.mWorldSize.x, mData.mWorldSize.y);
		mData.mDstRect = ci::Rectf(theX, theY, theX + mData.mWorldSize.x, theY + mData.mWorldSize.y);
		mData.mOriginalSrcRect = mData.mSrcRect;

		mSettings.getSetting("screen:mode", 0).mRawValue = "borderless";
		mSettings.getSetting("world_dimensions", 0).mRawValue = ds::unparseVector(mData.mWorldSize);
		mSettings.getSetting("src_rect", 0).mRawValue = ds::unparseRect(mData.mSrcRect);
		mSettings.getSetting("dst_rect", 0).mRawValue = ds::unparseRect(mData.mDstRect);

		DS_LOG_INFO("Auto-spanning window, world size:" << mSettings.getSetting("world_dimensions", 0).mRawValue << " src_rect:" << mSettings.getSetting("src_rect", 0).mRawValue << " dst_rect:" << mSettings.getSetting("dst_rect", 0).mRawValue);
	}
}

void Engine::setupConsole(){
	bool defaultShowConsole = false;
	DS_DBG_CODE(defaultShowConsole = true);
	if(mSettings.getBool("console:show", 0, defaultShowConsole)){
		showConsole();
	} else {
		hideConsole();
	}
}

void Engine::setupWindowMode(){
	if(!ci::app::getWindow()) return;

	auto newMode = mSettings.getString("screen:mode");
	if(newMode == "borderless"){
		ci::app::getWindow()->setFullScreen(false);
		ci::app::getWindow()->setBorderless(true);
	} else if(newMode.find("full") != std::string::npos) {
		/// setting fullscreen immediately causes some weirdness
		static bool firstSet = true;
		if(!firstSet) {
			ci::app::getWindow()->setFullScreen(true);
		}
		firstSet = false;
	} else {
		ci::app::getWindow()->setFullScreen(false);
		ci::app::getWindow()->setBorderless(false);
	}

	ci::app::getWindow()->setAlwaysOnTop(mSettings.getBool("screen:always_on_top"));
	ci::app::getWindow()->setTitle(mSettings.getString("screen:title"));
}

void Engine::setupMouseHide(){
	mHideMouse = mSettings.getBool("hide_mouse");
	mAutoHideMouse = mSettings.getBool("auto_hide_mouse");
	if(mAutoHideMouse) mHideMouse = true;
}

void Engine::setupFrameRate(){
	mData.mFrameRate = mSettings.getFloat("frame_rate");
	ci::app::setFrameRate(mData.mFrameRate);
}

void Engine::setupVerticalSync(){
	ci::gl::enableVerticalSync(mSettings.getBool("vertical_sync"));
}

void Engine::setupIdleTimeout(){
	setIdleTimeout(mSettings.getInt("idle_time"));

	auto numRoots = getRootCount();
	for(size_t i = 0; i < numRoots; i++) {
		getRootSprite(i).setSecondBeforeIdle(mData.mIdleTimeout);
		getRootSprite(i).startIdling();
	}
}

void Engine::setupMute(){
	setMute(mSettings.getBool("platform:mute"));
}

void Engine::setupResourceLocation() {


	mData.mCmsURL = mSettings.getString("cms:url");
	if(mData.mCmsURL.empty() || mData.mCmsURL == "DS_BASEURL") {
		auto _env_var_ptr = std::getenv("DS_BASEURL");
		mData.mCmsURL = std::string{ _env_var_ptr == nullptr ? "" : _env_var_ptr };
		if(mData.mCmsURL.empty()) {
			DS_LOG_VERBOSE(1, "cms:url and DS_BASEURL are both not defined. Only a problem if this app relies on Engine::getCmsURL()");
		} else {
			DS_LOG_INFO("Engine: Cms URL: " << mData.mCmsURL);
		}

	}
	
	std::string resourceLocation = ds::getNormalizedPath(mSettings.getString("resource_location"));
	if(resourceLocation.empty()) {
	} else {
		if(boost::contains(resourceLocation, "%USERPROFILE%")) {
#ifndef _WIN32
			boost::replace_all(resourceLocation, "%USERPROFILE%", Poco::Path::expand("~"));
			DS_LOG_WARNING("Linux workaround: Converting \"%USERPROFILE%\" to \"~\" in resources_location...");
#endif
		}

		resourceLocation = Poco::Path::expand(resourceLocation);
		resourceLocation = ds::Environment::expand(resourceLocation); // allow use of %APP%, etc
		Resource::Id::setupPaths(
			ds::getNormalizedPath(resourceLocation),
			ds::getNormalizedPath(mSettings.getString("resource_db")),
			ds::getNormalizedPath(mSettings.getString("project_path"))
		);
	}
}

void Engine::setupRoots() {
	clearRoots();

	// Don't construct roots on startup for clients, and instead create them when we connect to a server
	const std::string	arch(mSettings.getString("platform:architecture"));
	bool isClient = false;
	if(arch == "client") isClient = true;

	sprite_id_t							root_id = EMPTY_SPRITE_ID - 1;
	// Construct the root sprites
	if(!isClient) {
		// enable the bottom root to grab idle events
		bool firstRoot = true;
		RootList				roots(mRequestedRootList.runInitFn());
		if(roots.empty()) roots.ortho();
		for(auto it = roots.mRoots.begin(), end = roots.mRoots.end(); it != end; ++it) {
			RootList::Root&			r(*it);
			r.mRootId = root_id;
			std::unique_ptr<EngineRoot>		root;
			if(r.mType == r.kOrtho) root.reset(new OrthRoot(*this, r, r.mRootId));
			else if(r.mType == r.kPerspective) root.reset(new PerspRoot(*this, r, r.mRootId, r.mPersp));
			if(!root) {
				DS_LOG_WARNING("Couldn't create root in the engine!");
				continue;
			}

			if(firstRoot) {
				root->getSprite()->enable(true);
				firstRoot = false;
			}

			mRoots.push_back(std::move(root));
			--root_id;
		}
		if(mRoots.empty()) {
			DS_LOG_WARNING("Engine can't create single root");
		}
		root_setup(mRoots);
	}

	// If we're drawing the touches, create a separate top-level root to do that
	// For clients, the debug touch root is created by the server and synced

	// Add a view for displaying the stats.
	createStatsView(root_id);

	const bool drawTouches = mSettings.getBool("touch:debug");
	if(drawTouches && !isClient) {
		RootList::Root					root_cfg;
		root_cfg.mType = root_cfg.kOrtho;
		root_cfg.mDebugDraw = true;
		root_cfg.mDrawScaled = true;
		root_cfg.mRootId = root_id;
		std::unique_ptr<EngineRoot>		root;
		root.reset(new OrthRoot(*this, root_cfg, root_id));
		if(root) {
			ds::ui::Sprite*				parent = root->getSprite();
			if(parent) {
				parent->setDrawDebug(true);
				mRoots.push_back(std::move(root));
			}
		}
	}

	// Initialize the roots
	const EngineRoot::Settings	er_settings(mData.mWorldSize, mData.mSrcRect, mData.mDstRect);
	for(auto it = mRoots.begin(), end = mRoots.end(); it != end; ++it) {
		EngineRoot&				r(*(it->get()));
		r.setup(er_settings);
	}
}

void Engine::setupMetrics() {
	if(mMetricsService) {
		delete mMetricsService;
	}

	mMetricsService = new ds::MetricsService(*this);
}

void Engine::setupAutoRefresh() {
	mAutoRefresh.initialize();
}

void Engine::toggleConsole() {
	if(mShowConsole) hideConsole();
	else showConsole();
}

void Engine::showConsole(){
	// prevent calling create multiple times
	if(mShowConsole) return;

	mShowConsole = true;
	// TODO: Make this cleaner
#ifdef _WIN32
	GLOBAL_CONSOLE.create();
#endif
}


void Engine::hideConsole(){
	if(!mShowConsole) return;
	mShowConsole = false;
#ifdef _WIN32
	GLOBAL_CONSOLE.destroy();
#endif
}

void Engine::prepareSettings(ci::app::AppBase::Settings& settings) {
	settings.setWindowSize(static_cast<int>(getWidth()), static_cast<int>(getHeight()));

	/// Note: some of these are set in the engine constructor, but they don't get accurately applied on startup unless they're here too
	/// Sucks, but here it is

	auto screenMode = mSettings.getString("screen:mode");
	if(screenMode == "full" || screenMode == "fullscreen"){
		settings.setFullScreen(true);
	} else if(screenMode == "borderless"){
		settings.setBorderless(true);
	} else if(screenMode == "window"){
		settings.setBorderless(false);
		settings.setFullScreen(false);
	}

	settings.setResizable(false);
	bool aot = mSettings.getBool("screen:always_on_top");
	settings.setAlwaysOnTop(aot);
	settings.setFrameRate(mData.mFrameRate);
	
	DS_LOG_INFO("Engine::prepareSettings: screenMode is " << screenMode << " and always on top " << settings.isAlwaysOnTop());


	settings.setTitle(mSettings.getString("screen:title"));

}

void Engine::reloadSettings() {
	mSettings.loadInitialSettings();
	setupEngine();
	setupTouch(mDsApp);
	setup(mDsApp);
}

void Engine::onAppEvent(const ds::Event& in_e){
	if(in_e.mWhat == ds::cfg::Settings::SettingsEditedEvent::WHAT()){
		const ds::cfg::Settings::SettingsEditedEvent& e((const ds::cfg::Settings::SettingsEditedEvent&)in_e);
		if(e.mSettingsType == "engine"){
			if(e.mSettingName == "screen:mode" || e.mSettingName == "screen:always_on_top" || e.mSettingName == "screen:title"){
				setupWindowMode();
			} else if(e.mSettingName == "world_dimensions"){
				setupWorldSize();
			} else if(e.mSettingName == "src_rect" || e.mSettingName == "dst_rect"){
				setupSrcDstRects();
				markCameraDirty();
			} else if(e.mSettingName == "console:show"){
				setupConsole();
			} else if(e.mSettingName == "mouse:hide"){
				setupMouseHide();
			} else if(e.mSettingName == "frame_rate"){
				setupFrameRate();
			} else if(e.mSettingName == "vertical_sync"){
				setupVerticalSync();
			} else if(e.mSettingName == "idle_time"){
				setupIdleTimeout();
			} else if(e.mSettingName == "platform:mute"){
				setupMute();
			} else if(e.mSettingName.find("touch") != std::string::npos){
				setupTouch(mDsApp);
			} else if(e.mSettingName == "animation:duration") {
				setAnimDur(mSettings.getFloat("animation:duration"));
			}
		}
	} else if(in_e.mWhat == ds::app::RequestAppExitEvent::WHAT()) {
		mDsApp.quit();
	}
}

void Engine::showSettingsEditor(ds::cfg::Settings& theSettings){
	if(mSettingsEditor){
		mSettingsEditor->showSettings(theSettings.getName());
	}
}

void Engine::hideSettingsEditor(){
	if(mSettingsEditor){
		mSettingsEditor->hideSettings();
	}
}

bool Engine::isShowingSettingsEditor(){
	if(mSettingsEditor){
		return mSettingsEditor->visible();
	}

	return false;
}

void Engine::setup(ds::App& app) {

	mCinderWindow = app.getWindow();

	mTouchTranslator.setTranslation(mData.mSrcRect.x1, mData.mSrcRect.y1);
	mTouchTranslator.setScale(mData.mSrcRect.getWidth() / ci::app::getWindowWidth(), mData.mSrcRect.getHeight() / ci::app::getWindowHeight());


	const std::string	arch(mSettings.getString("platform:architecture"));
	bool isClient = false;
	if(arch == "client") isClient = true;
	const bool			drawTouches = mSettings.getBool("touch:debug", 0, false);
	for(auto it = mRoots.begin(), end = mRoots.end(); it != end; ++it) {
		(*it)->postAppSetup();
		(*it)->setCinderCamera();

		// Assume only one debug synchronized root? oh boy I hope so!
		if(!isClient && drawTouches && (*it)->getBuilder().mDebugDraw && (*it)->getBuilder().mSyncronize){

			ds::ui::DrawTouchView* v = new ds::ui::DrawTouchView(*this, mSettings, mTouchManager);
			(*it)->getSprite()->addChildPtr(v);
		}
	}

	const int		w = static_cast<int>(getWidth()),
		h = static_cast<int>(getHeight());
	if(w < 1 || h < 1) {
		// GN: recent updates should make this impossible to get to.
		//		but leaving this here in case some weird case sets the size to an invalid value
		DS_LOG_WARNING("Engine::setup() on 0 size width or height");
	}

	float curr = static_cast<float>(ci::app::getElapsedSeconds());
	mLastTime = curr;
	mLastTouchTime = 0;

	mUpdateParams.setDeltaTime(0.0f);
	mUpdateParams.setElapsedTime(curr);

	static bool firstRun = true;
	/// we only need to start services once, and not on any restarts
	if(firstRun) {
		// Start any library services
		if(!mData.mServices.empty()) {
			for(auto it = mData.mServices.begin(), end = mData.mServices.end(); it != end; ++it) {
				if(it->second) it->second->start();
			}
		}
		firstRun = false;
	}
}

void Engine::setupTouch(ds::App& app) {
	// touch settings
	mTouchManager.setOverrideTranslation(mSettings.getBool("touch:override_translation"));
	mTouchManager.setOverrideDimensions(mSettings.getVec2("touch:dimensions"));
	mTouchManager.setOverrideOffset(mSettings.getVec2("touch:offset"));
	mTouchManager.setTouchFilterRect(mSettings.getRect("touch:filter_rect"));

	mRotateTouchesDefault = mSettings.getBool("touch:rotate_touches_default");
	
	setTouchSmoothing(mSettings.getBool("touch:smoothing"));
	setTouchSmoothFrames(mSettings.getInt("touch:smooth_frames"));


	mData.mMinTapDistance = mSettings.getFloat("touch:tap_threshold");
	mData.mMinTouchDistance = mSettings.getFloat("touch:minimum_distance");
	mData.mSwipeQueueSize = mSettings.getInt("touch:swipe:queue_size");
	mData.mSwipeMinVelocity = mSettings.getFloat("touch:swipe:minimum_velocity");
	mData.mSwipeMaxTime = mSettings.getFloat("touch:swipe:maximum_time");

	setAnimDur(	mSettings.getFloat("animation:duration"));

	mTouchMode = ds::ui::TouchMode::fromSettings(mSettings);
	setTouchMode(mTouchMode);
	// don't lose idle just because we got a marker moved event
	mTuioObjectsMoved.setAutoIdleReset(false);
	if (ds::ui::TouchMode::hasTuio(mTouchMode)) {
		startTuio(app);
	} else {
		stopTuio();
	}

	mTuioInputs.clear();
	auto& theSettings = getSettings("tuio_inputs");
	const int numInputs = theSettings.getInt("tuio_input:number", 0, 0);
	for(int i = 0; i < numInputs; i++) {
		const int       tuioPort = theSettings.getInt("tuio_input:port", i, 0);
		const int       idOffset = theSettings.getInt("tuio_input:id_offset", i, 32);
		const ci::vec2  touchScale = theSettings.getVec2("tuio_input:scale", i, ci::vec2());
		const ci::vec2  touchOffset = theSettings.getVec2("tuio_input:offset", i, ci::vec2());
		const float     touchRotation = theSettings.getFloat("tuio_input:rotation", i, 0.0f);
		const ci::Rectf filterRect = theSettings.getRect("tuio_input:filter_rect", i, ci::Rectf());
		auto tuioInput = std::make_shared<ds::ui::TuioInput>(*this, tuioPort, touchScale, touchOffset, touchRotation,
			idOffset, filterRect);
		tuioInput->start(true);
		mTuioInputs.push_back(tuioInput);
	}

#ifdef _WIN32
	if(mDsApp.getWindow()) {
		auto hwnd = (HWND)mDsApp.getWindow()->getNative();
		if(ds::ui::TouchMode::hasSystem(mTouchMode)) {

			BOOL(WINAPI *RegisterTouchWindow)(HWND, ULONG);
			*(size_t *)&RegisterTouchWindow = (size_t)::GetProcAddress(::GetModuleHandle(TEXT("user32.dll")), "RegisterTouchWindow");
			if(RegisterTouchWindow) {
				(*RegisterTouchWindow)(hwnd, TWF_WANTPALM); // Immediately get the palm touch without waiting
			}
		}
#if 0
		/// Turn off all that dumb feedback shit always
		BOOL fEnabled = FALSE;
		SetWindowFeedbackSetting(hwnd,
									FEEDBACK_TOUCH_CONTACTVISUALIZATION,
									0, sizeof(fEnabled), &fEnabled);
		SetWindowFeedbackSetting(hwnd,
									FEEDBACK_TOUCH_TAP,
									0, sizeof(fEnabled), &fEnabled);
		SetWindowFeedbackSetting(hwnd,
									FEEDBACK_TOUCH_DOUBLETAP,
									0, sizeof(fEnabled), &fEnabled);
		SetWindowFeedbackSetting(hwnd,
									FEEDBACK_TOUCH_PRESSANDHOLD,
									0, sizeof(fEnabled), &fEnabled);
		SetWindowFeedbackSetting(hwnd,
									FEEDBACK_TOUCH_RIGHTTAP,
									0, sizeof(fEnabled), &fEnabled);
		
#endif
	}
#endif
}

void Engine::startTuio(ds::App& app) {
	mTuioObjectsMoved.setAutoIdleReset(false);

	mTuioPort = mSettings.getInt("touch:tuio:port");
	mTuioInput->start(false, mTuioPort);
	if (auto tuioReceiver = mTuioInput->getReceiver()) {
		registerForTuioObjects(tuioReceiver);
	}
}

void Engine::stopTuio() {
	mTuioInput->stop();
}

void Engine::clearRoots(){
	mRoots.clear();
}

void Engine::createClientRoots(std::vector<RootList::Root> roots){
	sprite_id_t	root_id = 0;

	for(auto it = roots.begin(), end = roots.end(); it != end; ++it) {
		const RootList::Root&			r(*it);
		std::unique_ptr<EngineRoot>		root;
		sprite_id_t thisRootId = (*it).mRootId;
		if(r.mType == r.kOrtho) root.reset(new OrthRoot(*this, r, thisRootId));
		else if(r.mType == r.kPerspective) root.reset(new PerspRoot(*this, r, thisRootId, r.mPersp));
		if(!root){
			DS_LOG_WARNING("Engine can't create root");
			continue;
		}
		mRoots.push_back(std::move(root));
		if(thisRootId < root_id){
			root_id = thisRootId;
		}
	}

	--root_id;
	createStatsView(root_id);
	root_setup(mRoots);

	const EngineRoot::Settings	er_settings(mData.mWorldSize, mData.mSrcRect, mData.mDstRect);
	for(auto it = mRoots.begin(), end = mRoots.end(); it != end; ++it) {
		EngineRoot&				r(*(it->get()));
		r.setup(er_settings);
	}

	for(auto it = mRoots.begin(), end = mRoots.end(); it != end; ++it) {
		(*it)->postAppSetup();
		(*it)->setCinderCamera();
	}
}

void Engine::createStatsView(sprite_id_t root_id){
	RootList::Root					root_cfg;
	root_cfg.mType = root_cfg.kOrtho;
	root_cfg.mDebugDraw = true;
	//root_cfg.mDrawScaled = false;
	root_cfg.mSyncronize = false;
	std::unique_ptr<EngineRoot>		root;
	root.reset(new OrthRoot(*this, root_cfg, root_id));
	if(root) {
		ds::ui::Sprite*				parent = root->getSprite();
		if(parent) {
			parent->setDrawDebug(true);
			mSettingsEditor = new ds::cfg::SettingsEditor(*this);
			if(mSettingsEditor){
				parent->addChildPtr(mSettingsEditor);
			}

			EngineStatsView*		v = new EngineStatsView(*this);
			if(v) {
				parent->addChild(*v);
			}

			mRoots.push_back(std::move(root));
		}
	}

}

ds::EventNotifier& Engine::getChannel(const std::string &name) {
	if(name.empty()){
		DS_LOG_WARNING("Engine::getChannel() on empty name");
		if(mChannels.empty()){
			mChannels["blank"] = Channel();
			return mChannels["blank"].mNotifier;
		} else {
			return mChannels.begin()->second.mNotifier;
		}
	}
	if (!mChannels.empty()) {
		auto f = mChannels.find(name);
		if (f != mChannels.end()) return f->second.mNotifier;
	}
	mChannels[name] = Channel();
	auto f = mChannels.find(name);
	if (f != mChannels.end()) return f->second.mNotifier;
	DS_LOG_WARNING("Engine::getChannel() no channel named " + name);
	if(mChannels.empty()){
		mChannels["blank"] = Channel();
		return mChannels["blank"].mNotifier;
	} else {
		return mChannels.begin()->second.mNotifier;
	}
}

void Engine::addChannel(const std::string &name, const std::string &description) {
	if(name.empty()){
		DS_LOG_WARNING("Engine::addChannel() on empty name");
		return;
	}
	mChannels[name] = Channel(description);
}

ds::AutoUpdateList& Engine::getAutoUpdateList(const int mask) {
	if ((mask&AutoUpdateType::SERVER) != 0) return mAutoUpdateServer;
	if ((mask&AutoUpdateType::CLIENT) != 0) return mAutoUpdateClient;
	DS_LOG_WARNING("Engine::getAutoUpdateList() on illegal param");
	return mAutoUpdateServer;
}

void Engine::addService(const std::string& str, ds::EngineService& service) {
	if (mData.mServices.empty()) {
		mData.mServices[str] = &service;
	} else {
		auto found = mData.mServices.find(str);
		if (found != mData.mServices.end()) {
			delete found->second;
			found->second = nullptr;
		}
		mData.mServices[str] = &service;
	}
}

void Engine::loadSettings(const std::string& name, const std::string& filename) {
	mData.mEngineCfg.loadSettings(name, filename);
}

void Engine::saveSettings(const std::string& name, const std::string& filename) {
	mData.mEngineCfg.saveSettings(name, filename);
}

void Engine::appendSettings(const std::string& name, const std::string& filename) {
	mData.mEngineCfg.appendSettings(name, filename);
}

void Engine::loadTextCfg(const std::string& filename) {
	mData.mEngineCfg.loadText(filename, *this);
}

size_t Engine::getRootCount() const {
	return mRoots.size();
}

ui::Sprite& Engine::getRootSprite(const size_t index) {
	if(index < 0 || index >= mRoots.size()){
		DS_LOG_WARNING("Engine::getRootSprite() on invalid index " << index);
		ui::Sprite* fs = mRoots.front()->getSprite();
		return *fs;
	}
	ui::Sprite*		s = mRoots[index]->getSprite();
	if(!s) DS_LOG_WARNING("Engine::getRootSprite() on null sprite");
	return *s;
}

ds::ui::Sprite* Engine::getRootSpritePtr(const size_t index /*= 0*/) {
	if(index < 0 || index >= mRoots.size()) {
		DS_LOG_WARNING("Engine::getRootSprite() on invalid index " << index);
		return mRoots.front()->getSprite();
	}
	ui::Sprite*		s = mRoots[index]->getSprite();
	if(!s) DS_LOG_WARNING("Engine::getRootSprite() on null sprite");
	return s;

}

const RootList::Root& Engine::getRootBuilder(const size_t index){
	if(index < 0 || index >= mRoots.size()){
		DS_LOG_WARNING("Engine::getRootBuilder() on invalid index " << index);
		return mRoots.front()->getBuilder();;
	}
	return mRoots[index]->getBuilder();
}

void Engine::updateClient() {
	float curr = static_cast<float>(ci::app::getElapsedSeconds());
	float dt = curr - mLastTime;
	mLastTime = curr;

	checkIdle();

	{
		std::lock_guard<std::mutex> lock(mTouchMutex);
		mMouseBeginEvents.lockedUpdate();
		mMouseMovedEvents.lockedUpdate();
		mMouseEndedEvents.lockedUpdate();
	}

	mMouseBeginEvents.update(curr);
	mMouseMovedEvents.update(curr);
	mMouseEndedEvents.update(curr);

	mUpdateParams.setDeltaTime(dt);
	mUpdateParams.setElapsedTime(curr);

	mAutoUpdateClient.update(mUpdateParams);

	for (auto it=mRoots.begin(), end=mRoots.end(); it!=end; ++it) {
		(*it)->updateClient(mUpdateParams);
	}
}

void Engine::updateServer() {
	if(mCachedWindowW != ci::app::getWindowWidth() || mCachedWindowH != ci::app::getWindowHeight()) {
		mCachedWindowW = ci::app::getWindowWidth();
		mCachedWindowH = ci::app::getWindowHeight();
		mTouchTranslator.setScale(	mData.mSrcRect.getWidth() / static_cast<float>(mCachedWindowW),
									mData.mSrcRect.getHeight() / static_cast<float>(mCachedWindowH));
	}

	const float		curr = static_cast<float>(ci::app::getElapsedSeconds());
	const float		dt = curr - mLastTime;
	mLastTime = curr;

	checkIdle();

	//////////////////////////////////////////////////////////////////////////
	{
		std::lock_guard<std::mutex> lock(mTouchMutex);
		mMouseBeginEvents.lockedUpdate();
		mMouseMovedEvents.lockedUpdate();
		mMouseEndedEvents.lockedUpdate();

		mTouchBeginEvents.lockedUpdate();
		mTouchMovedEvents.lockedUpdate();
		mTouchEndedEvents.lockedUpdate();

		mTuioObjectsBegin.lockedUpdate();
		mTuioObjectsMoved.lockedUpdate();
		mTuioObjectsEnded.lockedUpdate();
	} // unlock touch mutex
	//////////////////////////////////////////////////////////////////////////

	mMouseBeginEvents.update(curr);
	mMouseMovedEvents.update(curr);
	mMouseEndedEvents.update(curr);

	mTouchBeginEvents.update(curr);
	mTouchMovedEvents.update(curr);
	mTouchEndedEvents.update(curr);

	mTuioObjectsBegin.update(curr);
	mTuioObjectsMoved.update(curr);
	mTuioObjectsEnded.update(curr);

	mUpdateParams.setDeltaTime(dt);
	mUpdateParams.setElapsedTime(curr);

	mAutoUpdateServer.update(mUpdateParams);

	for (auto it=mRoots.begin(), end=mRoots.end(); it!=end; ++it) {
		(*it)->updateServer(mUpdateParams);
	}
}

void Engine::markCameraDirty() {
	for (auto it=mRoots.begin(), end=mRoots.end(); it!=end; ++it) {
		(*it)->markCameraDirty();
	}
}

PerspCameraParams Engine::getPerspectiveCamera(const size_t index) const {
	const PerspRoot*			root = nullptr;
	if (index < mRoots.size()) root = dynamic_cast<const PerspRoot*>(mRoots[index].get());
	if (root) {
		return root->getCamera();
	}
	DS_LOG_ERROR(" Engine::getPerspectiveCamera() on invalid root (" << index << ")");
	return PerspCameraParams();
}

const ci::CameraPersp& Engine::getPerspectiveCameraRef(const size_t index) const {
	const PerspRoot*			root = nullptr;
	if (index < mRoots.size()) root = dynamic_cast<const PerspRoot*>(mRoots[index].get());
	if (root) {
		return root->getCameraRef();
	} 
	DS_LOG_ERROR(" Engine::getPerspectiveCamera() on invalid root (" << index << ")");
	static ci::CameraPersp defaultCamera;
	return defaultCamera;
}

void Engine::setPerspectiveCamera(const size_t index, const PerspCameraParams& p) {
	PerspRoot*					root = nullptr;
	if (index < mRoots.size()) root = dynamic_cast<PerspRoot*>(mRoots[index].get());
	if (root) {
		root->setCamera(p);
	} else {
		DS_LOG_ERROR(" Engine::setPerspectiveCamera() on invalid root (" << index << ")");
	}
}

void Engine::setPerspectiveCameraRef(const size_t index, const ci::CameraPersp& p){
	PerspRoot*					root = nullptr;
	if(index < mRoots.size()) root = dynamic_cast<PerspRoot*>(mRoots[index].get());
	if(root) {
		root->setCameraRef(p);
	} else {
		DS_LOG_ERROR(" Engine::setPerspectiveCameraRef() on invalid root (" << index << ")");
	}
}

float Engine::getOrthoFarPlane(const size_t index)const{
	const OrthRoot*				root = nullptr;
	if(index < mRoots.size()) root = dynamic_cast<const OrthRoot*>(mRoots[index].get());
	if(root) {
		return root->getFarPlane();
	}
	DS_LOG_ERROR(" Engine::getOrthoFarPlane() on invalid root (" << index << ")");

	return 0.0f;
}

float Engine::getOrthoNearPlane(const size_t index)const{
	const OrthRoot*				root = nullptr;
	if(index < mRoots.size()) root = dynamic_cast<const OrthRoot*>(mRoots[index].get());
	if(root) {
		return root->getNearPlane();
	}
	DS_LOG_ERROR(" Engine::getOrthoNearPlane() on invalid root (" << index << ")");
	return 0.0f;
}

void Engine::setOrthoViewPlanes(const size_t index, const float nearPlane, const float farPlane){
	OrthRoot*				root = nullptr;
	if(index < mRoots.size()) root = dynamic_cast<OrthRoot*>(mRoots[index].get());
	if(root) {
		root->setViewPlanes(nearPlane, farPlane);
		root->setCinderCamera();
		return;
	}
	DS_LOG_ERROR(" Engine::setOrthoViewPlanes() on invalid root (" << index << ")");
}
void Engine::clearAllSprites(const bool clearDebug) {
	for (auto it=mRoots.begin(), end=mRoots.end(); it != end; ++it) {
		if((*it)->getBuilder().mDebugDraw && !clearDebug) continue;
		(*it)->clearChildren();
	}
}

void Engine::registerForTuioObjects(std::shared_ptr<ci::tuio::Receiver> tuioReceiver) {
	if (mSettings.getBool("touch:tuio:receive_objects", 0, false)) {
		const auto makeHandler = [this] (auto& eventQueue) {
			return [this, &eventQueue](const auto& o) {
				eventQueue.incoming(ds::TuioObject(o.getClassId(), o.getPosition(), o.getAngle(),
						o.getVelocity(), o.getRotationVelocity()
				));
			};
		};

		if (tuioReceiver) {
			tuioReceiver->setAddedFn  <ci::tuio::Object2d>(makeHandler(mTuioObjectsBegin));
			tuioReceiver->setUpdatedFn<ci::tuio::Object2d>(makeHandler(mTuioObjectsMoved));
			tuioReceiver->setRemovedFn<ci::tuio::Object2d>(makeHandler(mTuioObjectsEnded));
		}
	}
}

void Engine::drawClient() {
	ci::gl::enableAlphaBlending();

	ci::gl::clear(ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f));

	for(auto it = getRoots().begin(), end = getRoots().end(); it != end; ++it){
		(*it)->drawClient(getDrawParams(), getAutoDrawService());
	}
}

void Engine::drawServer() {
	ci::gl::enableAlphaBlending();

	ci::gl::clear(ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f));

	for(auto it = getRoots().cbegin(), end = getRoots().cend(); it != end; ++it){
		(*it)->drawServer(getDrawParams());
	}
}

ds::sprite_id_t Engine::nextSpriteId() {
	static ds::sprite_id_t              ID = 0;
	++ID;
	// Skip negative values.
	if (ID <= EMPTY_SPRITE_ID) ID = EMPTY_SPRITE_ID + 1;
	return ID;
}

void Engine::registerSprite(ds::ui::Sprite& s) {
	if (s.getId() == ds::EMPTY_SPRITE_ID) {
		DS_LOG_WARNING_M("Engine::registerSprite() on empty sprite ID", ds::ENGINE_LOG);
		assert(false);
		return;
	}
	mSprites[s.getId()] = &s;
}

void Engine::unregisterSprite(ds::ui::Sprite& s) {
	if (mSprites.empty()) return;
	if (s.getId() == ds::EMPTY_SPRITE_ID) {
		DS_LOG_WARNING_M("Engine::unregisterSprite() on empty sprite ID", ds::ENGINE_LOG);
		assert(false);
		return;
	}
	auto it = mSprites.find(s.getId());
	if (it != mSprites.end()) mSprites.erase(it);
}

ds::ui::Sprite* Engine::findSprite(const ds::sprite_id_t id) {
	if (mSprites.empty()) return nullptr;
	auto it = mSprites.find(id);
	if (it == mSprites.end()) return nullptr;
	return it->second;
}

void Engine::spriteDeleted(const ds::sprite_id_t&) {
	// Only a server cares about this; everything else just
	// deletes in-place.
}

ci::Color8u Engine::getUniqueColor() {
	int32_t			i = (mUniqueColor.r << 16) | (mUniqueColor.g << 8) | mUniqueColor.b;
	++i;
	mUniqueColor.r = (i>>16)&0xff;
	mUniqueColor.g = (i>>8)&0xff;
	mUniqueColor.b = (i)&0xff;
	return mUniqueColor;
}

void Engine::touchesBegin(const ds::ui::TouchEvent &e) {
	mTouchBeginEvents.incoming(mTouchTranslator.toWorldSpace(e));
}

void Engine::touchesMoved(const ds::ui::TouchEvent &e) {
	mTouchMovedEvents.incoming(mTouchTranslator.toWorldSpace(e));
}

void Engine::touchesEnded(const ds::ui::TouchEvent &e) {
	mTouchEndedEvents.incoming(mTouchTranslator.toWorldSpace(e));
}

std::shared_ptr<ci::tuio::Receiver>	Engine::getTuioClient(const int tuioIndex) {
	if (tuioIndex >= 0 && tuioIndex < mTuioInputs.size())
		return mTuioInputs[tuioIndex]->getReceiver();

	return mTuioInput->getReceiver();
}

void Engine::mouseTouchBegin(const ci::app::MouseEvent &e, int id) {
	if (ds::ui::TouchMode::hasMouse(mTouchMode)) {
		mMouseBeginEvents.incoming(MousePair(alteredMouseEvent(e), id));
	}
}

void Engine::mouseTouchMoved(const ci::app::MouseEvent &e, int id) {
	if (ds::ui::TouchMode::hasMouse(mTouchMode)) {
		mMouseMovedEvents.incoming(MousePair(alteredMouseEvent(e), id));
	}
}

void Engine::mouseTouchEnded(const ci::app::MouseEvent &e, int id) {
	if (ds::ui::TouchMode::hasMouse(mTouchMode)) {
		mMouseEndedEvents.incoming(MousePair(alteredMouseEvent(e), id));
	}
}

ci::app::MouseEvent Engine::alteredMouseEvent(const ci::app::MouseEvent& e) const {
	if(mTouchManager.getInputMode() != ds::ui::TouchManager::kInputNormal) return e;

	// Note -- breaks the button and modifier checks, because cinder doesn't give me access to the raw data.
	// Currently I believe that's fine -- and since our target is touch platforms without those things
	// hopefully it always will be.
	// Note that you CAN get to this if you want to interpret what's there. I *think* I saw that
	// the newer version of cinder gave access so hopefully can just wait for that if we need it.

	// Transform the mouse event coordinate from window/screen space to world
	// coordinates.  Note: We are using the actual window size here, not the
	// mDstRect size, which may be different if the window gets automagically
	// resized for some reason.  (For example, in fullscreen mode, or a window
	// mode on a screen that is not big enough for the mDstRect)
	const ci::vec2 srcOffset = mData.mSrcRect.getUpperLeft();

	auto screenSize = glm::vec2( ci::app::getWindowSize() );
	const ci::vec2 screenScale = screenSize / mData.mSrcRect.getSize();

	const ci::vec2 mouseWorldPos = srcOffset + (ci::vec2(e.getX(), e.getY()) / screenScale);
	const ci::ivec2 pos((int)mouseWorldPos.x, (int)mouseWorldPos.y);

	return ci::app::MouseEvent(e.getWindow(),	0, pos.x, pos.y,
												0, e.getWheelIncrement(), e.getNativeModifiers());
}

void Engine::injectTouchesBegin(const ds::ui::TouchEvent& e){
	touchesBegin(e);
}

void Engine::injectTouchesMoved(const ds::ui::TouchEvent& e){
	touchesMoved(e);
}

void Engine::injectTouchesEnded(const ds::ui::TouchEvent& e){
	touchesEnded(e);
}

void Engine::injectObjectsBegin(const ds::TuioObject& o) {
	mTuioObjectsBegin.incoming(o);
}

void Engine::injectObjectsMoved(const ds::TuioObject& o) {
	mTuioObjectsMoved.incoming(o);
}

void Engine::injectObjectsEnded(const ds::TuioObject& o) {
	mTuioObjectsEnded.incoming(o);
}

ds::ResourceList& Engine::getResources() {
	return mResources;
}

const ds::FontList& Engine::getFonts() const {
	return mFonts;
}

ds::FontList& Engine::editFonts() {
	return mFonts;
}

const ds::ColorList& Engine::getColors() const {
	return mColors;
}

ds::ColorList& Engine::editColors() {
	return mColors;
}

void Engine::stopServices() {
	if (mData.mServices.empty()) return;

	for (auto it=mData.mServices.begin(), end=mData.mServices.end(); it!=end; ++it) {
		ds::EngineService*	s = it->second;
		if (s) s->stop();
	}
}

void Engine::setHideMouse(const bool doMouseHide){
	mHideMouse = doMouseHide;
}

bool Engine::getHideMouse() const {
	return mHideMouse;
}

ds::ui::Sprite* Engine::getHit(const ci::vec3& point) {
	for (auto it=mRoots.rbegin(), end=mRoots.rend(); it!=end; ++it) {
		ds::ui::Sprite* s = (*it)->getHit(point);
		if (s) return s;
	}
	return nullptr;
}

void Engine::clearFingers( const std::vector<int> &fingers ) {
	mTouchManager.clearFingers(fingers);
}

void Engine::nextTouchMode() {
	mSettings.getSetting("touch:mode", 0).mRawValue = ds::ui::TouchMode::toString(ds::ui::TouchMode::next(mTouchMode));
	setupTouch(mDsApp);
	//setTouchMode();
}

void Engine::setTouchSmoothing(const bool doSmoothing){
	mTouchManager.setTouchSmoothing(doSmoothing);
}

void Engine::setTouchSmoothFrames(const int smoothFrames){
	mTouchManager.setTouchSmoothFrames(smoothFrames);
}

const bool Engine::getTouchSmoothing(){
	return mTouchManager.getTouchSmoothing();
}

void Engine::writeSprites(std::ostream &s) const {
#ifdef _DEBUG
	for (auto it=mRoots.begin(), end=mRoots.end(); it!=end; ++it) {
		EngineRoot*		er(it->get());
		ds::ui::Sprite*	sprite(er ? er->getSprite() : nullptr);
		if (sprite) sprite->write(s, 0);
	}
#endif
}

void Engine::checkIdle() {
	bool newIdle = true;
	const size_t numRoots = getRootCount();
	for(int i = 0; i < numRoots - 1; i++) {
		if(getRootBuilder(i).mDebugDraw) continue;
		if(!getRootSprite(i).isIdling()) {
			newIdle = false;
			break;
		}
	}
	
	if(newIdle != mIdling) {
		mIdling = newIdle;
		if(mIdling) {
			getNotifier().notify(ds::app::IdleStartedEvent());
		} else {
			getNotifier().notify(ds::app::IdleEndedEvent());
		}
	}

}

bool Engine::isIdling(){
	return mIdling;
}

void Engine::startIdling() {
	// force idle mode to start again
	const size_t numRoots = getRootCount();
	for(size_t i = 0; i < numRoots - 1; i++) {
		// don't clear the last root, which is the debug draw
		if(getRootBuilder(i).mDebugDraw) continue;
		getRootSprite(i).startIdling();
	}
	mIdling = true;
	getNotifier().notify(ds::app::IdleStartedEvent());
}

void Engine::resetIdleTimeout() {
	float curr = static_cast<float>(ci::app::getElapsedSeconds());
	mLastTime = curr;
	mLastTouchTime = curr;

	const size_t numRoots = getRootCount();
	for(size_t i = 0; i < numRoots - 1; i++) {
		// don't clear the last root, which is the debug draw
		if(getRootBuilder(i).mDebugDraw) continue;
		getRootSprite(i).resetIdleTimer();
	}

	mIdling = false;
	getNotifier().notify(ds::app::IdleEndedEvent());
}

void Engine::setTouchMode(const ds::ui::TouchMode::Enum &mode) {
	mTouchMode = mode;
	mTouchManager.setTouchMode(mode);
}

ci::app::WindowRef Engine::getWindow(){
	return mCinderWindow;
}

bool Engine::getRotateTouchesDefault(){
	return mRotateTouchesDefault;
}

/**
 * \class Channel
 */
Engine::Channel::Channel() {
}

Engine::Channel::Channel(const std::string &description)
		: mDescription(description) {
}

} // namespace ds


namespace {

ds::EngineRoot*		find_master(const ds::RootList::Root::Type t, std::vector<std::unique_ptr<ds::EngineRoot>>& list) {
	for (auto it=list.begin(), end=list.end(); it!=end; ++it) {
		ds::EngineRoot*		r(it->get());
		if (!r) continue;
		if (r->getBuilder().mType == t && r->getBuilder().mMaster == ds::RootList::Root::kMaster) {
			return r;
		}
	}
	return nullptr;
}

void				root_setup(std::vector<std::unique_ptr<ds::EngineRoot>>& dst) {
	// Go through each of my roots, searching for a master. If I have a master, hook up all my slaves.
	for (auto it=dst.begin(), end=dst.end(); it!=end; ++it) {
		ds::EngineRoot*		r(it->get());
		if (!r) continue;
		if (r->getBuilder().mMaster == ds::RootList::Root::kSlave) {
			ds::EngineRoot*	master = find_master(r->getBuilder().mType, dst);
			if (master && master != r) {
				r->slaveTo(master);
			}
		}
	}
}

}
