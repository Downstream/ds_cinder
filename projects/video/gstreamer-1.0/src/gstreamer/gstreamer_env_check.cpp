#include "stdafx.h"

#include "gstreamer_env_check.h"

#include <Poco/Path.h>

#include <regex>

#include <cinder/Filesystem.h>
#include <ds/util/file_meta_data.h>
#include <ds/app/environment.h>
#include <ds/debug/logger.h>

namespace {
// Replaces forward slash with back slash and replaces all multiple repeats of back slashes with a single back slash
void normalizePath(std::string& path) {
	try {
		static std::regex path_anomalies("\\\\+|/");
		path = std::regex_replace(path, path_anomalies, "/");
		path = Poco::Path::expand(path);
	}
	catch (...) {}
}
// Long story short, this is here to prevent a Release mode exception in VS2013
std::string getEnv(const std::string& name) {
	auto _env_var_ptr = std::getenv(name.c_str());
	return std::string{ _env_var_ptr == nullptr ? "" : _env_var_ptr };
}}

namespace ds {
namespace gstreamer {

bool EnvCheck::addGStreamerBinPath(){

	// Some installs were having issues with the registry getting funked up on multiple runs
	// If we manually set the registry to the app directory, it can avoid some issues
	// std::string registryPath{ getEnv("GST_REGISTRY_1_0") };
	// if(registryPath.empty()) {
	// 	std::string localRegistryPath = ds::Environment::expand("%LOCAL%/cache/.gstcache");
	// 	ds::Environment::replaceEnvironmentVariable("GST_REGISTRY_1_0", localRegistryPath);
	// }


	std::string path_variable{ getEnv("PATH") };
	std::string plugin_variable{ getEnv("GST_PLUGIN_PATH") };

	// There's 2 methods to including GStreamer in a release.
	// 1. Copy all the dll's / exe's from the gstreamer bin directory (c:/Program Files(x86)/gstreamer/x86/bin by default (may be on a different drive)) to %APP%/dll
	//	  Copy all the dll's (you can ignore the folders) from the plugin directory (c:\gstreamer\1.0\x86\lib\gstreamer-1.0) to "%APP%/dll/gst-plugins"
	//    These dll's can be included in your released packed so a local gstreamer install is not required

	// 2. Install the Gstreamer runtime msi on the target machine ( https://gstreamer.freedesktop.org/data/pkg/windows/1.8.2/gstreamer-1.0-x86-1.8.2.msi ) as of this writing

	bool addedLocalDlls = false;

	std::string localDllPath = ds::Environment::expand("%APP%/dll/");
	std::string localPlugins = ds::Environment::expand("%APP%/dll/gst_plugins");
	if(safeFileExistsCheck(localDllPath) && safeFileExistsCheck(localPlugins)){
		if(path_variable.find(localDllPath) == std::string::npos){
			DS_LOG_INFO( "GStreamer Env Check: Adding local dlls to path for video playback");
			ds::Environment::addToFrontEnvironmentVariable("PATH", localDllPath);
		}

		if(plugin_variable.find(localPlugins) == std::string::npos) {
			DS_LOG_INFO("GStreamer Env Check: Adding local gst plugins");
			ds::Environment::replaceEnvironmentVariable("GST_PLUGIN_PATH", localPlugins);
		}
		addedLocalDlls = true;
	} 

#ifdef _WIN64
	std::string gstreamer_path = getEnv("GSTREAMER_1_0_ROOT_X86_64");
#else 
	std::string gstreamer_path = getEnv("GSTREAMER_1_0_ROOT_X86");
#endif
	std::string gstreamer_bin_path = gstreamer_path + "\\bin";
	normalizePath(gstreamer_bin_path);



	// Only add the environment varible version if we don't have local dll's
	if(!addedLocalDlls && path_variable.find(gstreamer_bin_path) == std::string::npos && ds::safeFileExistsCheck(gstreamer_bin_path, true)) {
		ds::Environment::addToFrontEnvironmentVariable("PATH", gstreamer_bin_path);
	} else if(!addedLocalDlls){
		DS_LOG_WARNING("No gstreamer bin path found! That's a problem if you wanna see any videos!")
	}
		
	if(!addedLocalDlls && getEnv("GST_PLUGIN_PATH").empty()){
		std::string gstreamer_plugin_path = gstreamer_path + "\\lib\\gstreamer-1.0";
		normalizePath(gstreamer_plugin_path);

		if(ds::safeFileExistsCheck(gstreamer_plugin_path)){
			ds::Environment::replaceEnvironmentVariable("GST_PLUGIN_PATH", gstreamer_plugin_path);
		} else {
			DS_LOG_WARNING("No gstreamer plugin path found! That's a problem if you wanna see any videos!")
		}
	}

	if(addedLocalDlls || ds::safeFileExistsCheck(gstreamer_path, true)){
		return true;
	} 
	
	return false;	

}

}} //!ds::gstreamer
