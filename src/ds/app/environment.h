#pragma once
#ifndef DS_APP_ENVIRONMENT_H_
#define DS_APP_ENVIRONMENT_H_

#include <string>

namespace ds {

namespace cfg {
class Settings;
}

/**
 * \class Environment
 * Access to the environment data, i.e. file paths etc.
 * Ideally I'd use boost fs::path but I'm keeping it simple as I port.
 */
class Environment {
public:
	Environment() = delete;
	Environment(const Environment&) = delete;

	/// Return the same path but with any environment variables expanded. Current variables:
	///	%APP% -- expanded to app folder
	///	%LOCAL% -- expanded to downstream documents folder
	///  %PP% -- expand the project path, i.e. "%LOCAL%/cache/%PP%/images/"
	///  %CFG_FOLDER% -- expand to the configuration folder, if it exists
	/// "%DOCUMENTS%" -- expand to current user documents folder
	static std::string			expand(const std::string& path);

	/// Return the path but with any applicable environment variables inserted. See expand for variables
	static std::string			contract(const std::string& fullPath);

	/// Answer an app folder -- currently only SETTINGS() is valid for arg 1.
	/// If fileName is valid, then it will be appended to the found app folder, if it exists.
	/// This function assumes that I don't actually know the location of the folderName
	/// relative to my appPath, and searches up the appPath looking for it.  This makes
	/// it so no configuration is needed between dev and production environments.
	/// If verify is true, then verify that the folder or file exists, otherwise answer a blank string.
	static std::string			getAppFolder(	const std::string& folderName, const std::string& fileName = "",
												const bool verify = false);

	/// Answer a complete path to a local settings file.  Supply an empty file name
	/// to just get the local settings folder.
	static std::string			getLocalSettingsPath(const std::string& fileName);

	/// Convenience to load in a settings file, first from the app path, then the local path
	static void					loadSettings(const std::string& settingsName, const std::string& filename, ds::cfg::Settings&);

	/// Check if there are settings at the appropriate paths
	static bool					hasSettings(const std::string& filename);
	
	/// Convenience to save a settings file to the local path
	static void					saveSettings(const std::string& filename, ds::cfg::Settings&);

	/// Utility to replace the value of an environment variable
	static void					replaceEnvironmentVariable(const std::string& variable, const std::string& value);

	/// Utility to add a value to an environment variable. 
	/// This adds the value to the end of any existing value
	static void					addToEnvironmentVariable(const std::string& variable, const std::string& value);

	/// Utility to add a value to an environment variable. 
	/// This adds the value to the beginning of any existing value
	static void					addToFrontEnvironmentVariable(const std::string& variable, const std::string& value);

	/// DEPRECATED
	static const std::string&	SETTINGS();
	static const std::string&	RESOURCES();
	/// Answer the Downstream documents folder.
	/// Obsolete -- use ds::Environment::expand("%LOCAL%");
	static std::string			getDownstreamDocumentsFolder();
	/// Answer the current project path
	/// Obsolete -- use ds::Environment::expand("%PP%");
	static std::string			getProjectPath();
	/// Path is relative to the app folder. It can end in a folder or file leaf.
	/// Obsolete -- use ds::Environment::expand("%APP%/path");
	static std::string			getAppFile(const std::string& path);
	/// Obsolete -- use ds::Environment::expand("%LOCAL%/resources/folderName/fileName");
	static std::string			getLocalResourcesFolder(const std::string& folderName, const std::string& fileName = "");
	/// OK, this API has become pretty messy. This gets to the local folder, which can have
	/// optional category (SETTINGS() etc.), project_path, and file name
	/// Obsolete -- use ds::Environment::expand("%LOCAL%/settings/%PP%/filename");
	static std::string			getLocalFile(const std::string& category, const bool includeProjectPath, const std::string& filename);

	/// Utility to get command-line parameters
	static std::vector<std::string>
								getCommandLineParams();

	/// Utility to get the current clipboard contents as a string
	static std::string			getClipboard();

	static void					setConfigDirFileExpandOverride(const bool doOverride);
private:
	friend class App;
	static bool					initialize();
};

} // namespace ds

#endif // DS_APP_ENVIRONMENT_H_
