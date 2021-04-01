#include "stdafx.h"

#include "ds/app/environment.h"
#include "ds/util/file_meta_data.h"

#include <boost/algorithm/string.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Environment.h>
#include <Poco/Format.h>
#include <Poco/Process.h>

#include "ds/app/app.h"
#include "ds/app/engine/engine_settings.h"
#include "ds/util/string_util.h"

#ifdef CINDER_MSW
#include "cinder/Clipboard.h"
#include <windows.h>
#include <KnownFolders.h>
#include <Shlobj.h>
#else
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#endif

static std::string    folder_from(const Poco::Path&, const std::string& folder, const std::string& fileName);

namespace ds {

namespace {
std::string				DOCUMENTS("%DOCUMENTS%");
std::string				DOWNSTREAM_DOCUMENTS("%LOCAL%");
bool					USE_CFG_FILE_OVERRIDE = false;
#ifdef WIN32
const std::string		ENV_PATH_SEPARATOR = ";";
#else
const std::string		ENV_PATH_SEPARATOR = ":";
#endif

bool					sInitialized = false;
}

bool Environment::initialize() {
	if (sInitialized)
		return true;
	sInitialized = true;

	std::string homePath = Poco::Path::home();

	/// In windows, the user can change the documents directory. 
	/// To maintain consistency with other things (like DsNode), we have to get the actual documents directory
	/// Also if the user sets up windows with OneDrive, the documents directory will be USER/OneDrive/Documents instead of USER/Documents
	/// This should account for that
#ifdef CINDER_MSW
	PWSTR   ppszPath;    // variable to receive the path memory block pointer.

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &ppszPath);

	std::wstring myPath;
	if(SUCCEEDED(hr)) {
		myPath = ppszPath;      // make a local copy of the path
	}

	CoTaskMemFree(ppszPath);
	homePath = ds::utf8_from_wstr(myPath);
#else 
	Poco::Path newPath(homePath);
	newPath.append("Documents");
	homePath = newPath.toString();
#endif

	Poco::Path p(homePath);
	DOCUMENTS = p.toString();

	p.append("downstream");
	DOWNSTREAM_DOCUMENTS = p.toString();

	return true;
}


void Environment::setConfigDirFileExpandOverride(const bool doOverride) {
	USE_CFG_FILE_OVERRIDE = doOverride;
}

const std::string& Environment::SETTINGS() {
	static const std::string    SZ("settings");
	return SZ;
}

const std::string& Environment::RESOURCES() {
	static const std::string    SZ("resources");
	return SZ;
}

std::string Environment::expand(const std::string& _path) {
	if (!sInitialized) ds::Environment::initialize();

	std::string		p(_path);

	if(USE_CFG_FILE_OVERRIDE && p.find("%APP%") != std::string::npos){
		std::string tempP = p;
		boost::replace_first(tempP, "%APP%", "%APP%/settings/%CFG_FOLDER%");
		boost::replace_all(tempP, "%APP%", ds::App::envAppDataPath());
		boost::replace_all(tempP, "%CFG_FOLDER%", EngineSettings::getConfigurationFolder());
		std::string tempPath = Poco::Path(tempP).toString();
		if(ds::safeFileExistsCheck(tempPath)){
			return tempPath;
		}
	}

	boost::replace_all(p, "%APP%", ds::App::envAppDataPath());
	boost::replace_all(p, "%PP%", EngineSettings::envProjectPath());
	boost::replace_all(p, "%LOCAL%", getDownstreamDocumentsFolder());
	boost::replace_all(p, "%CFG_FOLDER%", EngineSettings::getConfigurationFolder());
	boost::replace_all(p, "%DOCUMENTS%", DOCUMENTS);
	// This can result in double path separators, so flatten
	return Poco::Path(p).toString();
}


std::string Environment::contract(const std::string& fullPath){
	if (!sInitialized) ds::Environment::initialize();

	std::string		p(fullPath);
	boost::replace_all(p, ds::App::envAppDataPath(), "%APP%");
	boost::replace_all(p, EngineSettings::envProjectPath(), "%PP%");
	boost::replace_all(p, getDownstreamDocumentsFolder(), "%LOCAL%");
	boost::replace_all(p, EngineSettings::getConfigurationFolder(), "%CFG_FOLDER%");
	boost::replace_all(p, DOCUMENTS, "%DOCUMENTS%");
	// This can result in double path separators, so flatten
	return Poco::Path(p).toString();
}

std::string Environment::getAppFolder(const std::string& folderName, const std::string& fileName, const bool verify) {
	Poco::Path				p(ds::App::envAppDataPath());
	if (!folderName.empty()) p.append(folderName);
	if (!fileName.empty()) p.append(fileName);
	const std::string		ans(p.toString());
	if (verify) {
		try {
			Poco::File		f(ans);
			if (!f.exists()) return "";
		} catch (std::exception const&) {
		}
	}
	return ans;
}

std::string Environment::getAppFile(const std::string& path) {
	Poco::Path      p(ds::App::envAppDataPath());
	p.append(path);
	return p.toString();
}

std::string Environment::getLocalResourcesFolder(const std::string& folderName, const std::string& fileName /*= ""*/)
{
	if(EngineSettings::envProjectPath().empty()){
		return "";
	}
	Poco::Path p(getDownstreamDocumentsFolder());
	p.append(RESOURCES()).append(EngineSettings::envProjectPath());
	std::string     ans;
	// A couple things limit the search -- the directory can't get
	// too short, and right now nothing is more then 3 steps from the appPath.
	int             count = 0;
	while((ans = folder_from(p, folderName, fileName)).empty()) {
		p.popDirectory();
		if(count++ >= 3 || p.depth() < 2)
			return "";
	}
	return ans;
}

std::string Environment::getDownstreamDocumentsFolder()
{
	if (!sInitialized) ds::Environment::initialize();
	return DOWNSTREAM_DOCUMENTS;
}

std::string Environment::getLocalSettingsPath(const std::string& fileName)
{
	if(EngineSettings::envProjectPath().empty()) return "";
	Poco::Path			p(getDownstreamDocumentsFolder());
	p.append(SETTINGS()).append(EngineSettings::envProjectPath()).append(fileName);
	return p.toString();
}

void Environment::loadSettings(const std::string& settingsName, const std::string& filename, ds::cfg::Settings& settings) {
	settings.setName(settingsName);
	settings.readFrom(ds::Environment::getAppFolder(ds::Environment::SETTINGS(), filename), false);
	settings.readFrom(ds::Environment::getLocalSettingsPath(filename), true);
	if (!ds::EngineSettings::getConfigurationFolder().empty()) {
		const std::string		app = ds::Environment::expand("%APP%/settings/%CFG_FOLDER%/" + filename);
		const std::string		local = ds::Environment::expand("%LOCAL%/settings/%PP%/%CFG_FOLDER%/" + filename);
		settings.readFrom(app, true);
		settings.readFrom(local, true);
	}
}

bool Environment::hasSettings(const std::string& filename) {
	if (ds::safeFileExistsCheck(ds::Environment::getAppFolder(ds::Environment::SETTINGS(), filename))) return true;
	if (ds::safeFileExistsCheck(ds::Environment::getLocalSettingsPath(filename))) return true;
	if (ds::safeFileExistsCheck(ds::Environment::expand("%APP%/settings/%CFG_FOLDER%/" + filename))) return true;
	if (ds::safeFileExistsCheck(ds::Environment::expand("%LOCAL%/settings/%PP%/%CFG_FOLDER%/" + filename))) return true;

	return false;
}

void Environment::saveSettings(const std::string& filename, ds::cfg::Settings& settings) {
	if(!ds::EngineSettings::getConfigurationFolder().empty()) {
		const std::string		local = ds::Environment::expand("%LOCAL%/settings/%PP%/%CFG_FOLDER%/" + filename);
		settings.writeTo(local);
	}
}


std::string Environment::getLocalFile(	const std::string& category,
										const bool includeProjectPath,
										const std::string& filename)
{
	Poco::Path			p(getDownstreamDocumentsFolder());
	if (!category.empty()) {
		p.append(category);
		if (includeProjectPath && !EngineSettings::envProjectPath().empty()) {
			p.append(EngineSettings::envProjectPath());
			if (!filename.empty()) {
				p.append(filename);
			}
		}
	}
	return p.toString();
}

std::string Environment::getProjectPath() {
	return EngineSettings::envProjectPath();
}

void Environment::replaceEnvironmentVariable(const std::string& variable, const std::string& value) {
	Poco::Environment::set(variable, value);
}


void Environment::addToEnvironmentVariable(const std::string& variable, const std::string& value) {
	std::string new_path = Poco::Environment::get(variable, "");

	if(new_path.length() > 0) {
		new_path += ENV_PATH_SEPARATOR;
	}
	new_path += value;

	Poco::Environment::set(variable, new_path);
}

void Environment::addToFrontEnvironmentVariable(const std::string& variable, const std::string& value) {
	std::string old_path = Poco::Environment::get(variable, "");
	std::string new_path = value + ENV_PATH_SEPARATOR + old_path;
	Poco::Environment::set(variable, new_path);
}

// Platform dependent code for getting the command line args
#ifdef CINDER_MSW
#include <winsock2.h> // need to include winsock2 before windows
#include <windows.h>
#include <shellapi.h>

std::vector<std::string> Environment::getCommandLineParams()
{
	std::vector<std::string> ret;
	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if(szArglist != NULL) {
		for(int i = 0; i < nArgs; ++i) ret.push_back(ds::utf8_from_wstr(szArglist[i]));
	}
	LocalFree(szArglist);

	return ret;
}
#else
// On Linux, we need to process the contents of cmdline from the /proc filesystem
std::vector<std::string> Environment::getCommandLineParams()
{
	auto filename = Poco::format("/proc/%d/cmdline", Poco::Process::id());
	auto ifs = std::ifstream( filename, std::ios::in|std::ios::binary );
	std::ostringstream ss;
	ss << ifs.rdbuf();

	return ds::split( ss.str(), std::string("\0", 1), true );
}
#endif

std::string Environment::getClipboard() {
#ifdef CINDER_MSW
	return ci::Clipboard::getString();
#else
	auto window = (GLFWwindow*)ci::app::getWindow()->getNative();
	return std::string( glfwGetClipboardString(window) );
#endif
}


} // namespace ds

static std::string    folder_from(const Poco::Path& parentP, const std::string& folder, const std::string& fileName) {
	Poco::Path          p(parentP);
	p.append(folder);

	const Poco::File    f(p);
	if(f.exists() && f.isDirectory()) {
		if(fileName.empty()) return f.path();
		p.append(fileName);
		return p.toString();
	}
	return "";
}
