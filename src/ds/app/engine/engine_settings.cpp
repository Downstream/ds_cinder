#include "stdafx.h"

#include "ds/app/engine/engine_settings.h"

#include <Poco/Path.h>
#include <Poco/String.h>
#include "ds/app/environment.h"
#include "ds/util/string_util.h"
#include "ds/debug/logger.h"
#include "ds/util/file_meta_data.h"

namespace {
std::string			PROJECT_PATH;
// The configuration settings
std::string			CONFIGURATION_FOLDER;
std::string			CONFIGURATION_OVERRIDE_FOLDER;

static bool			get_key_value(const std::string& arg, std::string& key, std::string& value) {
	const std::string	SEP_SZ("=");
	const std::string	SEP_SZ_ALT(":");
	size_t				sep = arg.find(SEP_SZ);
	if(sep == arg.npos) {
		sep = arg.find(SEP_SZ_ALT);
		if(sep == arg.npos)
			return false;
	}

	key = arg.substr(0, sep);
	Poco::toLowerInPlace(key);

	value = arg.substr(sep + 1, arg.size() - (sep + 1));

	return true;
}

} // anonymous namespace

namespace ds {

/**
 * \class EngineSettings
 */
EngineSettings::EngineSettings() 
	: mLoadedAnySettings(false)
{
	loadInitialSettings();
}

void EngineSettings::loadInitialSettings() {
	setName("engine");
	setDefaults();

	mLoadedAnySettings = false;
	mStartupInfo.str("");

	// Default file names.
	const std::string			DEFAULT_FILENAME("engine.xml");
	std::string					appFilename = DEFAULT_FILENAME,
		localFilename,
		commandLineAppConfig,
		projectPath;

	std::vector<std::string>	args = ds::Environment::getCommandLineParams();
	for(const auto& arg : args) {
		std::string				key, value;
		if(!get_key_value(arg, key, value)) continue;

		if(key == "app_settings") {
			appFilename = value;
			if(localFilename.empty()) localFilename = value;
		} else if(key == "local_settings") {
			localFilename = value;
		} else if(key == "local_path") {
			// The local path and filename need to be parsed here.
			Poco::Path				p(value);
			const std::string&		fn(p.getFileName());
			const std::string		full(p.toString());
			projectPath = full.substr(0, full.length() - (fn.length() + 1));
			localFilename = fn;
		} else if(key == "configuration" || key == "config") {
			commandLineAppConfig = value;
		}

	}
	if(localFilename.empty()) localFilename = DEFAULT_FILENAME;

	// I have all the argument-supplied paths and filenames.  Now I can
	// start reading my settings files.

	// APP SETTINGS
	// Find my app settings/ directory.  This will vary based on whether I'm in a dev environment or
	// in a production, but I will have a settings/ folder either at or above me.
	const std::string         appSettingsPath = ds::Environment::getAppFolder(ds::Environment::SETTINGS());
	if(appSettingsPath.empty()) {
		std::cout << "Couldn't find the application settings folder, that could be a problem." << std::endl;
	}
	Poco::Path                appP(appSettingsPath);
	appP.append(appFilename);

	std::string appFullPath = appP.toString();
	if(safeFileExistsCheck(appFullPath)) {
		mLoadedAnySettings = true;
		mStartupInfo << "EngineSettings: Reading app settings from " << appFullPath << std::endl;
		readFrom(appFullPath, true);
	}

	// LOCAL SETTINGS
	// The project path is taken from the supplied arguments, if it existed, or else it's
	// pulled from the settings I just loaded.  If neither has it, then I guess no local settings.
	if(projectPath.empty()) {
		projectPath = getString("project_path", 0, projectPath);
	} else {
		// If it exists, then make sure then any project_path in the settings is the same.  No one
		// should ever use that, but let's be safe.

		auto& theSetting = getSetting("project_path", 0);
		theSetting.mRawValue = projectPath;

	}

	if(!projectPath.empty()) {
		// Set the global project path
		PROJECT_PATH = projectPath;

		std::string localSettingsPath = ds::Environment::getLocalSettingsPath(localFilename);
		if(safeFileExistsCheck(localSettingsPath)) {
			mLoadedAnySettings = true;
			mStartupInfo << "EngineSettings: Reading app settings from " << localSettingsPath << std::endl;
			readFrom(localSettingsPath, true);
		}
	}

	// Load the configuration settings, which can be used to modify settings even more.
	// Currently used to provide alternate layout sizes.
	ds::Environment::loadSettings("configuration", "configuration.xml", mConfiguration);
	CONFIGURATION_FOLDER = commandLineAppConfig.empty()
		? mConfiguration.getString("folder", 0, "")
		: commandLineAppConfig;

	if(!CONFIGURATION_OVERRIDE_FOLDER.empty()){
		CONFIGURATION_FOLDER = CONFIGURATION_OVERRIDE_FOLDER;
	}

	// If the folder exists, then apply any changes to the engine file
	if(!CONFIGURATION_FOLDER.empty()) {
		const std::string		app = ds::Environment::expand("%APP%/settings/%CFG_FOLDER%/" + appFilename);
		const std::string		local = ds::Environment::expand("%LOCAL%/settings/%PP%/%CFG_FOLDER%/" + appFilename);

		if(safeFileExistsCheck(app)) {
			mLoadedAnySettings = true;
			mStartupInfo << "EngineSettings: Reading app settings from " << app << std::endl;
			readFrom(app, true);
		}

		if(safeFileExistsCheck(local)) {
			mLoadedAnySettings = true;
			mStartupInfo << "EngineSettings: Reading app settings from " << local << std::endl;
			readFrom(local, true);
		}
	}

}

const std::string& EngineSettings::envProjectPath() {
	return PROJECT_PATH;
}

const ds::cfg::Settings& EngineSettings::getConfiguration() {
	return mConfiguration;
}

void EngineSettings::setDefaults(){
	getSetting("SERVER SETTINGS", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, ""); // need to include the blank comment to get the correct getter
	getSetting("project_path", 0, ds::cfg::SETTING_TYPE_STRING, "Project path for locating app resources"); 
	getSetting("server:connect", 0, ds::cfg::SETTING_TYPE_BOOL, "If false, won't connect udp sender / listener for server or client", "false");
	getSetting("server:ip", 0, ds::cfg::SETTING_TYPE_STRING, "The multicast group udp address and port of the server", "239.255.42.58");
	getSetting("server:send_port", 0, ds::cfg::SETTING_TYPE_INT, "The send port of the server. Match these between server and client", "1037", "1", "99999");
	getSetting("server:listen_port", 0, ds::cfg::SETTING_TYPE_INT, "The listen port of the server (which is what the client sends on). Match these between server and client.", "1038", "1", "99999");
	getSetting("platform:architecture", 0, ds::cfg::SETTING_TYPE_STRING, "If this is a server (world engine), a client (render engine) or both (world + render). clientserver is an EngineClientServer, which both displays content and can control other instances. standalone does not transmit or receive.", "standalone", "", "", "standalone, client, server, clientserver");
	getSetting("platform:guid", 0, ds::cfg::SETTING_TYPE_STRING, "Unique identifier for network traffic (appended by additional unique values).", "Downstream");
	getSetting("xml_importer:cache", 0, ds::cfg::SETTING_TYPE_BOOL, "If the xml importer should cache xml content or reload from disk each time", "true");
	getSetting("xml_importer:target", 0, ds::cfg::SETTING_TYPE_STRING, "target for xml importer target properties to match", "default");

	getSetting("WINDOW SETTINGS", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("span_all_displays", 0, ds::cfg::SETTING_TYPE_BOOL, "Automatically spans displays and overrides world size, src/dst rect and screen:mode", "false");
	getSetting("world_dimensions", 0, ds::cfg::SETTING_TYPE_VEC2, "The size of the overall app space.", "1920, 1080");
	getSetting("src_rect", 0, ds::cfg::SETTING_TYPE_RECT, "The rectangle of the world space to render.");
	getSetting("dst_rect", 0, ds::cfg::SETTING_TYPE_RECT, "The output window size and position to render.");
	getSetting("screen:title", 0, ds::cfg::SETTING_TYPE_STRING, "The title of the window. Generally only displays if the screen mode is windowed.");
	getSetting("screen:mode", 0, ds::cfg::SETTING_TYPE_STRING,  "How the primary window displays, including fullscreen", "borderless", "", "", "window, borderless, fullscreen");
	getSetting("screen:always_on_top", 0, ds::cfg::SETTING_TYPE_BOOL, "Makes the window an always-on-top sort of window.", "false");
	getSetting("screen:auto_size", 0, ds::cfg::SETTING_TYPE_STRING, "Classic uses the old src_rect/dst_rect and span_all_displays; letterbox will letterbox to your main monitor; all_span spans all displays; main_span fills the main display. letterbox requires having a world size set.", "classic", "", "", "classic, letterbox, all_span, main_span");
	getSetting("console:show", 0, ds::cfg::SETTING_TYPE_BOOL, "Show console will create a console window, or not if this is false.", "false");
	getSetting("idle_time", 0, ds::cfg::SETTING_TYPE_DOUBLE, "Seconds before idle happens. 300 = 5 minutes.", "300", "0", "1000");
	getSetting("system:never_sleep", 0, ds::cfg::SETTING_TYPE_BOOL, "Prevent the system from sleeping or powering off the screen", "true");
	getSetting("apphost:exit_on_quit", 0, ds::cfg::SETTING_TYPE_BOOL, "Exit apphost when quitting the app", "true");
	getSetting("resetup_server_on_display_change", 0, ds::cfg::SETTING_TYPE_BOOL, "Resetup the server when the display changes", "true");
	

	getSetting("RENDER SETTINGS", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("frame_rate", 0, ds::cfg::SETTING_TYPE_INT, "Attempt to run the app at this rate", "60", "1", "1000");
	getSetting("vertical_sync", 0, ds::cfg::SETTING_TYPE_BOOL, "Attempts to align frame rate with the refresh rate of the monitor. Note that this could be overriden by the graphic card", "true");
	getSetting("auto_hide_mouse", 0, ds::cfg::SETTING_TYPE_BOOL, "True=automatically hide the mouse when mouse hasn't been moved, false=use hide_mouse setting", "true");
	getSetting("hide_mouse", 0, ds::cfg::SETTING_TYPE_BOOL, "False=cursor visible, true=no visible cursor.", "false");
	getSetting("camera:arrow_keys", 0, ds::cfg::SETTING_TYPE_FLOAT, "How much to step the camera when using the arrow keys. Set to a value above 0.025 to enable arrow key usage.", "30.0", "-1.0", "200.0");
	getSetting("platform:mute", 0, ds::cfg::SETTING_TYPE_BOOL, "Mutes all video sound if true", "false");
	getSetting("animation:duration", 0, ds::cfg::SETTING_TYPE_FLOAT, "Standard duration for animations", "0.35", "0.0", "10.0");
	getSetting("load_image:threads", 0, ds::cfg::SETTING_TYPE_INT, "Number of threads to spawn for image loading", "1", "0", "32");
	getSetting("font_scale", 0, ds::cfg::SETTING_TYPE_FLOAT, "text sprites with scale font values by this amount", "1.3333333333333", "0.001", "1000.0");

	getSetting("TOUCH SETTINGS", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("touch:mode", 0, ds::cfg::SETTING_TYPE_STRING, "Set the current touch mode: Tuio, TuioAndMouse, System, SystemAndMouse, All.", "SystemAndMouse", "", "", "Tuio, TuioAndMouse, System, SystemAndMouse, All");
	getSetting("touch:tuio:port", 0, ds::cfg::SETTING_TYPE_INT, "UDP Port to listen to tuio stream.", "3333", "1", "9999");
	getSetting("touch:tuio:receive_objects", 0, ds::cfg::SETTING_TYPE_BOOL, "Will allow tuio to receive object data.", "false");
	getSetting("touch:override_translation", 0, ds::cfg::SETTING_TYPE_BOOL, "Override the built-in touch scale and offset parsing. It's uncommon you'll need to do this. Default is to use the built-in Cinder touch translation, which is generally correct if the window is the same pixel size as the main screen and not scaled at all.", "false");
	getSetting("touch:dimensions", 0, ds::cfg::SETTING_TYPE_VEC2, "How large in screen pixels the touch input stream covers", "1920, 1080");
	getSetting("touch:offset", 0, ds::cfg::SETTING_TYPE_VEC2, "How much to offset touch input in pixels", "0, 0");
	getSetting("touch:filter_rect", 0, ds::cfg::SETTING_TYPE_RECT, "Any touches started outside this rect will be ignored, in world space. Set to 0, 0, 0, 0 to ignore.", "0, 0, 0, 0");
	getSetting("touch:debug", 0, ds::cfg::SETTING_TYPE_BOOL, "Draw circles around touch points ", "true");
	getSetting("touch:debug_circle_radius", 0, ds::cfg::SETTING_TYPE_FLOAT, "Visual settings for touch debug circles.", "15", "1", "100");
	getSetting("touch:debug_circle_color", 0, ds::cfg::SETTING_TYPE_COLOR, "The color of the touch debug circles", "#ffffff");
	getSetting("touch:debug_circle_filled", 0, ds::cfg::SETTING_TYPE_BOOL,  "If the touch debug circles are a filled or stroked circle.", "false");
	getSetting("touch:rotate_touches_default", 0, ds::cfg::SETTING_TYPE_BOOL, "Rotates touch points if the sprite getting touched is rotated. Helpful for table situations that can have sprites rotated 180 degrees or whatnot.", "false");
	getSetting("touch:tap_threshold", 0, ds::cfg::SETTING_TYPE_FLOAT, "How far a touch moves before it's not a tap, in pixels.", "30", "0", "200");
	getSetting("touch:minimum_distance", 0, ds::cfg::SETTING_TYPE_FLOAT, "How many pixels away from an existing touch point for a new touch to be considered valid.", "20.0", "1.0", "300");
	getSetting("touch:smoothing", 0, ds::cfg::SETTING_TYPE_BOOL, "Average out touch points over time for smoother input, but slightly less accurate.", "true");
	getSetting("touch:smooth_frames", 0, ds::cfg::SETTING_TYPE_INT, "How many frames to use when smoothing. Higher numbers are smoother. Lower than 3 is effectively off.", "5", "1", "64");
	getSetting("touch:swipe:queue_size", 0, ds::cfg::SETTING_TYPE_INT, "How many frames of touch swipe info to account for when calculating swipes", "4", "1", "16");
	getSetting("touch:swipe:minimum_velocity", 0, ds::cfg::SETTING_TYPE_FLOAT, "The velocity a swipe needs to exceed to count as a swipe", "800.0", "1.0", "2400");
	getSetting("touch:swipe:maximum_time", 0, ds::cfg::SETTING_TYPE_FLOAT, "How long a swipe can last to be counted as a swipe", "0.5", "0.0", "3.0");

	getSetting("RESOURCE SETTINGS ", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("resource_location", 0, ds::cfg::SETTING_TYPE_STRING, "Resource location and database for cms content");
	getSetting("resource_db", 0, ds::cfg::SETTING_TYPE_STRING, "Path of the database relative to the resource_location. E.g. ../db/database.sqlite");
	getSetting("configuration_folder:allow_expand_override", 0, ds::cfg::SETTING_TYPE_BOOL, "Allows you to place any relative file in a configuration folder. For instance, you could have a layout file specific to a particular configuration.", "false");
	getSetting("cms:url", 0, ds::cfg::SETTING_TYPE_STRING, "The URL of a Content Management System, set as DS_BASE_URL to use that env variable.", "DS_BASEURL");
	getSetting("node:refresh_rate", 0, ds::cfg::SETTING_TYPE_FLOAT, "If your app uses a NodeWatcher, how often to check for node updates", "0.1", "0.001", "10.0");
	getSetting("content:node_watch", 0, ds::cfg::SETTING_TYPE_BOOL, "If ContentWrangler should automatically listen to dsnode messages on udp localhost port 7777", "true");
	getSetting("content:model_location", 0, ds::cfg::SETTING_TYPE_STRING, "Where ContentWrangler should look for an xml file that describes a data model to load sqlite data. Specify multiple locations separated by a semicolon", "%APP%/data/model/content_model.xml");
	getSetting("content:use_wrangler", 0, ds::cfg::SETTING_TYPE_BOOL, " If ContentWrangler should be used to automatically grab data", "false");
	getSetting("auto_refresh_app", 0, ds::cfg::SETTING_TYPE_BOOL, "Listen to directory changes and auto soft-restart the app.", "false");
	getSetting("auto_refresh_directories", 0, ds::cfg::SETTING_TYPE_STRING, "Semi-colon separated list of directories to listen to to restart the app. If auto_refresh_app is off, will still listen to these directories", "%APP%");

	getSetting("LOGGER", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("logger:level", 0, ds::cfg::SETTING_TYPE_STRING, "What level of log to log.", "all", "", "", "all, none, info, warning, error, fatal");
	getSetting("logger:module", 0, ds::cfg::SETTING_TYPE_STRING, "all,none, or numbers (i.e. 0,1,2,3).  Applications map the numbers to specific modules.", "all");
	getSetting("logger:async", 0, ds::cfg::SETTING_TYPE_STRING, "Whether to save logs on another thread or the main one.", "true");
	getSetting("logger:file", 0, ds::cfg::SETTING_TYPE_STRING, "Filename and location", "%LOCAL%/logs/");
	getSetting("logger:verbose_level", 0, ds::cfg::SETTING_TYPE_INT, "How much verbose output to log. 0=nothing, 9=everything", "0", "0", "9");

	getSetting("METRICS", 0, ds::cfg::SETTING_TYPE_SECTION_HEADER, "");
	getSetting("metrics:active", 0, ds::cfg::SETTING_TYPE_BOOL, "Enable telegraf metrics sending", "true");
	getSetting("metrics:send_base_info", 0, ds::cfg::SETTING_TYPE_BOOL, "Send common engine info like fps and number of sprites", "true");
	getSetting("metrics:base_info_send_delay", 0, ds::cfg::SETTING_TYPE_DOUBLE, "How often to send the base info in seconds", "5.0", "0.5", "100");
	getSetting("metrics:send_touch_info", 0, ds::cfg::SETTING_TYPE_BOOL, "Record touch data or not", "false");
	getSetting("metrics:udp_host", 0, ds::cfg::SETTING_TYPE_STRING, "The host name to send udp metrics info to", "127.0.0.1");
	getSetting("metrics:udp_port", 0, ds::cfg::SETTING_TYPE_STRING, "The port to send udp metrics info to", "8094");

}


void EngineSettings::setConfigurationOverride(std::string overrideFolder) {
	CONFIGURATION_OVERRIDE_FOLDER = overrideFolder;
	CONFIGURATION_FOLDER = overrideFolder;
}

const std::string& EngineSettings::getConfigurationFolder() {
	return CONFIGURATION_FOLDER;
}

void EngineSettings::printStartupInfo(){
	if(mLoadedAnySettings){
		DS_LOG_INFO(std::endl << mStartupInfo.str());
	} else {
		DS_LOG_WARNING("No settings files loaded. Using default values.");
	}
}

} // namespace ds

