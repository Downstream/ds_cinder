#include "stdafx.h"

#include "ds/app/engine/engine_cfg.h"
#include <cinder/app/App.h>
#include <ds/debug/logger.h>

#include <Poco/String.h>
#include "ds/app/environment.h"
#include "ds/debug/debug_defines.h"
#include "ds/app/engine/engine_settings.h"


static void read_text_defaults(std::unordered_map<std::string, ds::ui::TextStyle>& out, ds::Engine& engine);
static void read_text_cfg(const std::string& path, std::unordered_map<std::string, ds::ui::TextStyle>& out, ds::Engine& engine);
static void interpret_text_settings(ds::cfg::Settings &s, std::unordered_map<std::string, ds::ui::TextStyle>& out, ds::Engine& engine);


namespace ds {

namespace {
const std::string			EMPTY_SZ;
const std::string			ENGINE_SZ("engine");
}

/**
 * ds::EngineCfg
 */
EngineCfg::EngineCfg(ds::cfg::Settings& engine_settings)
	: mEngineSettings(engine_settings) 
	, mEmptySettings()
	, mEditEmptySettings()
{
}

ds::cfg::Settings& EngineCfg::getSettings(const std::string& name) {
	if (name.empty()) {
		DS_LOG_WARNING("EngineCfg::getSettings() on empty name");
		return mEmptySettings;
	}
	if (mSettings.empty()) {
		if (name == ENGINE_SZ) return mEngineSettings;
		DS_LOG_WARNING("EngineCfg::getSettings() on empty mSettings");
		return mEmptySettings;
	}
	auto it = mSettings.find(name);
	if (it == mSettings.end()) {
		if (name == ENGINE_SZ) return mEngineSettings;
		DS_LOG_WARNING("EngineCfg::getSettings() settings does not exist: " << name);
		return mEmptySettings;
	}
	return it->second;
}

ds::cfg::Settings& EngineCfg::getNextSettings(const std::string& name){
	if(mSettings.empty() || name.empty()){
		DS_LOG_WARNING("EngineCfg::getNextSettings() could not fulfill your request. Bummer.");
		return mEmptySettings;
	}

	if(name == ENGINE_SZ) return mSettings.begin()->second;

	bool foundTheSetting = false;
	std::string theKey = "";
	for(auto it : mSettings){
		if(foundTheSetting){
			theKey = it.first;
			break;
		}
		if(it.second.getName() == name){
			foundTheSetting = true;
		}
	}

	if(!theKey.empty()){
		return mSettings[theKey];
	}

	// wrap around
	return mEngineSettings;
}

bool EngineCfg::hasTextStyle(const std::string& name) const {
	if (name.empty() || mTextStyles.empty()) return false;

	auto it = mTextStyles.find(name);
	if (it == mTextStyles.end()) return false;

	return true;
}

const ds::ui::TextStyle& EngineCfg::getTextStyle(const std::string& name) const {

	if (name.empty()) {
		DS_LOG_WARNING("EngineCfg::getTextStyle() on empty name");
		return mDefaultTextStyle;
	}

	if (mTextStyles.empty()) {
		DS_LOG_WARNING("EngineCfg::getTextStyle() but there are no styles loaded (key=" << name << ")");
		return mDefaultTextStyle;
	}

	auto it = mTextStyles.find(name);
	if (it == mTextStyles.end()) {
		DS_LOG_WARNING("EngineCfg::getTextStyle() style " << name << " does not exist, using the default text style.");
		return mDefaultTextStyle;
	}

	return it->second;
}

void EngineCfg::setDefaultTextStyle(const ds::ui::TextStyle& theStyle) {
	mDefaultTextStyle = theStyle;
}

const ds::ui::TextStyle& EngineCfg::getDefaultTextStyle() const{
	return mDefaultTextStyle;
}

void EngineCfg::setTextStyle(const std::string& name, const ds::ui::TextStyle& t) {
	mTextStyles[name] = t;
	if (name == "default") setDefaultTextStyle(t);
}

void EngineCfg::clearTextStyles() {
	mTextStyles.clear();
}

bool EngineCfg::hasSettings(const std::string& name) const {
	return mSettings.find(name) != mSettings.cend();
}

void EngineCfg::loadSettings(const std::string& name, const std::string& filename) {
	// see if the settings exist already
	auto findy = mSettings.find(name);
	if(findy == mSettings.end()) {
		mSettings.emplace(std::make_pair(name, ds::cfg::Settings()));
	}

	findy = mSettings.find(name);
	
	ds::Environment::loadSettings(name, filename, findy->second);
}

void EngineCfg::saveSettings(const std::string& name, const std::string& filename) {
	auto findy = mSettings.find(name);
	if(findy != mSettings.end()){
		findy->second.writeTo(filename);
	} else {
		DS_LOG_WARNING("EngineCfg: Couldn't find the settings to save for name " << name);
	}
}

void EngineCfg::appendSettings(const std::string& name, const std::string& filename) {
	auto findy = mSettings.find(name);
	if(findy != mSettings.end()) {
		findy->second.readFrom(filename, true);
	} else {
		DS_LOG_WARNING("EngineCfg:appendSettings couldn't find the base settings to append to.");
	}
}

void EngineCfg::loadText(const std::string& filename, Engine& engine) {
	read_text_cfg(ds::Environment::getAppFolder(ds::Environment::SETTINGS(), filename), mTextStyles, engine);
	read_text_cfg(ds::Environment::getLocalSettingsPath(filename), mTextStyles, engine);
	if (!ds::EngineSettings::getConfigurationFolder().empty()) {
		const std::string		app = ds::Environment::expand("%APP%/settings/%CFG_FOLDER%/" + filename);
		const std::string		local = ds::Environment::expand("%LOCAL%/settings/%PP%/%CFG_FOLDER%/" + filename);
		read_text_cfg(app, mTextStyles, engine);
		read_text_cfg(local, mTextStyles, engine);
	}

	if(!mTextStyles.empty()){
		for(auto it = mTextStyles.begin(); it != mTextStyles.end(); ++it){
			mDefaultTextStyle = it->second;
			break;
		}
	}
}

} // namespace ds

static bool split_key(const std::string& key, std::string& left, std::string& right) {
	if (key.empty()) return false;
	const size_t		pos = key.find_last_of(":");
	if (pos != key.npos) {
		left = key.substr(0, pos);
		right = key.substr(pos+1, key.length()-pos+1);
		Poco::toLowerInPlace(right);
		return !left.empty() && !right.empty();
	}
	return false;
}

static void read_text_cfg(const std::string& path, std::unordered_map<std::string, ds::ui::TextStyle>& out, ds::Engine& engine) {
	ds::cfg::Settings s;
	s.readFrom(path, false);
	interpret_text_settings(s, out, engine);
}

static void interpret_text_settings(ds::cfg::Settings &s, std::unordered_map<std::string, ds::ui::TextStyle>& out, ds::Engine& engine) {
	// Do the name first, because that determines whether an entry exists.
	s.forEachSetting([&s, &out](const ds::cfg::Settings::Setting& setting) {
		std::string			left, right;
		if (split_key(setting.mName, left, right) && right == "name") {
			std::string		v = setting.getString();
			if (!v.empty()) {
				if (out.empty()) {
					out[left] = ds::ui::TextStyle(v, left, 10.0f, 1.0f, 0.0f, ci::ColorA(1.0f, 1.0f, 1.0f, 1.0f));
				} else {
					auto	found = out.find(left);
					if (found != out.end()) {
						found->second.mFont = v;
					} else {
						out[left] = ds::ui::TextStyle(v, left, 10.0f, 1.0f, 0.0f, ci::ColorA(1.0f, 1.0f, 1.0f, 1.0f));
					}
				}
			}
		}
	}, ds::cfg::SETTING_TYPE_STRING);

	// Floats (size, leading)
	s.forEachSetting([&s, &out](const ds::cfg::Settings::Setting& setting) {
		std::string			left, right;
		if (split_key(setting.mName, left, right) && !out.empty()) {
			auto			found = out.find(left);
			if (found != out.end()) {
				if(right == "size") found->second.mSize = setting.getFloat();
				else if (right == "leading") found->second.mLeading = setting.getFloat();
				else if (right == "letter_spacing") found->second.mLetterSpacing = setting.getFloat();
			}
		}
	}, ds::cfg::SETTING_TYPE_FLOAT);

	// Color (color)
	s.forEachSetting([&s, &out, &engine](const ds::cfg::Settings::Setting& setting) {
		std::string			left, right;
		if (split_key(setting.mName, left, right) && !out.empty()) {
			auto			found = out.find(left);
			if (found != out.end()) {
				if(right == "color") found->second.mColor = setting.getColorA(engine);
			}
		}
	}, ds::cfg::SETTING_TYPE_COLOR);

	// Text (alignment)
	s.forEachSetting([&s, &out](const ds::cfg::Settings::Setting& setting) {
		std::string			left, right;
		if (split_key(setting.mName, left, right) && !out.empty()) {
			auto			found = out.find(left);
			if (found != out.end()) {
				if (right == "alignment") {
					found->second.mAlignment = ds::ui::Alignment::fromString(setting.getString());
				}
			}
		}
	}, ds::cfg::SETTING_TYPE_STRING);

}
