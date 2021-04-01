#include "stdafx.h"

#include "settings.h"

#include <cinder/Xml.h>
#include <Poco/File.h>
#include <Poco/String.h>
#include "ds/ui/sprite/sprite_engine.h"
#include "ds/debug/logger.h"
#include "ds/debug/debug_defines.h"
#include "ds/util/string_util.h"
#include "ds/util/color_util.h"
#include "ds/util/file_meta_data.h"

namespace ds {
namespace cfg {

const std::string&				SETTING_TYPE_UNKNOWN = "unknown";
const std::string&				SETTING_TYPE_BOOL = "bool";
const std::string&				SETTING_TYPE_INT = "int";
const std::string&				SETTING_TYPE_FLOAT = "float";
const std::string&				SETTING_TYPE_DOUBLE = "double";
const std::string&				SETTING_TYPE_STRING = "string";
const std::string&				SETTING_TYPE_WSTRING = "wstring";
const std::string&				SETTING_TYPE_COLOR = "color";
const std::string&				SETTING_TYPE_COLORA = "colora";
const std::string&				SETTING_TYPE_VEC2 = "vec2";
const std::string&				SETTING_TYPE_VEC3 = "vec3";
const std::string&				SETTING_TYPE_RECT = "rect";
const std::string&				SETTING_TYPE_SECTION_HEADER = "section_header";
const std::string&				SETTING_TYPE_TEXT_STYLE = "text_style";

namespace {

static const int	SETTINGS_INCREMENT = 200;

static std::vector<std::string>	SETTING_TYPES;
void initialize_types(){
	if(SETTING_TYPES.empty()){
		SETTING_TYPES.emplace_back(SETTING_TYPE_UNKNOWN);
		SETTING_TYPES.emplace_back(SETTING_TYPE_BOOL);
		SETTING_TYPES.emplace_back(SETTING_TYPE_INT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_FLOAT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_DOUBLE);
		SETTING_TYPES.emplace_back(SETTING_TYPE_STRING);
		SETTING_TYPES.emplace_back(SETTING_TYPE_WSTRING);
		SETTING_TYPES.emplace_back(SETTING_TYPE_COLOR);
		SETTING_TYPES.emplace_back(SETTING_TYPE_COLORA);
		SETTING_TYPES.emplace_back(SETTING_TYPE_VEC2);
		SETTING_TYPES.emplace_back(SETTING_TYPE_VEC3);
		SETTING_TYPES.emplace_back(SETTING_TYPE_RECT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_SECTION_HEADER);
		SETTING_TYPES.emplace_back(SETTING_TYPE_TEXT_STYLE);
	}
}

static void merge_settings(
	std::vector<std::pair<std::string, std::vector<ds::cfg::Settings::Setting>>>& dst,
	const std::vector<std::pair<std::string, std::vector<ds::cfg::Settings::Setting>>>& src){

	int highestReadIndex = 0;

	for(auto& dit : dst) {
		for (auto vit : dit.second){
			if(vit.mReadIndex > highestReadIndex) highestReadIndex = vit.mReadIndex;
		}
	}

	highestReadIndex += SETTINGS_INCREMENT;

	for(auto sit : src) {
		bool found = false;
		for(auto& dit : dst){
			if(dit.first == sit.first){

				int thisReadIndex = 0;
				for(int i = 0; i < sit.second.size(); i++) {
					if(i < dit.second.size()) {
						int readIndex = dit.second[i].mReadIndex;
						dit.second[i] = sit.second[i];
						dit.second[i].mReadIndex = readIndex;

						if(readIndex > thisReadIndex) thisReadIndex = readIndex;
					} else {
						dit.second.emplace_back(sit.second[i]);
						if(thisReadIndex > 0) {
							dit.second.back().mReadIndex = thisReadIndex;
							thisReadIndex += 1;
						}

					}
				}
			

				found = true;
				break;
			}
		}

		if(!found){
			for (auto it : sit.second){
				it.mReadIndex = highestReadIndex;
				highestReadIndex += SETTINGS_INCREMENT;
			}

			dst.emplace_back(sit);
		}
	}
}

}

bool Settings::Setting::getBool() const {
	return parseBoolean(mRawValue);
}

int Settings::Setting::getInt() const{
	return ds::string_to_int(mRawValue);
}

float Settings::Setting::getFloat() const{
	return ds::string_to_float(mRawValue);
}

double Settings::Setting::getDouble() const{
	return ds::string_to_double(mRawValue);
}

const ci::Color Settings::Setting::getColor(ds::ui::SpriteEngine& eng) const{
	return ds::parseColor(mRawValue, eng);
}

const ci::ColorA Settings::Setting::getColorA(ds::ui::SpriteEngine& eng) const{
	return ds::parseColor(mRawValue, eng);
}

const std::string& Settings::Setting::getString() const{
	return mRawValue;
}

const std::wstring Settings::Setting::getWString() const{
	return ds::wstr_from_utf8(mRawValue);
}

const ci::vec2 Settings::Setting::getVec2() const {
	return ci::vec2(parseVector(mRawValue));
}

const ci::vec3 Settings::Setting::getVec3() const {
	return parseVector(mRawValue);
}

const cinder::Rectf Settings::Setting::getRect() const{
	return parseRect(mRawValue);
}

std::vector<std::string> Settings::Setting::getPossibleValues() const{
	std::vector<std::string> possibles = ds::split(mPossibleValues, ", ", true);
	return possibles;
}

void Settings::Setting::replaceSettingVariablesAndExpressions()
{

	auto testValue = mRawValue;
	std::string value = ds::cfg::SettingsVariables::replaceVariables(testValue);
	value = ds::cfg::SettingsVariables::parseAllExpressions(value);
	if (value != mRawValue) {
		//save the originalRawValue
		mOriginalValue = mRawValue;
		mRawValue = value;
	} else if(!mOriginalValue.empty())
	{
		std::string value = ds::cfg::SettingsVariables::replaceVariables(mOriginalValue);
		value = ds::cfg::SettingsVariables::parseAllExpressions(value);
		mRawValue = value;
	}
}

Settings::Settings()
	: mReadIndex(1)
{
	initialize_types();
}

void Settings::mergeSettings(Settings & mergeIn)
{
	merge_settings(mSettings, mergeIn.mSettings);
}

void Settings::readFrom(const std::string& filename, const bool append){
	if(!append) {
		directReadFrom(filename, true);
		return;
	}

	Settings		s;
	s.directReadFrom(filename, false);

	merge_settings(mSettings, s.mSettings);
}

void Settings::readFrom(ci::XmlTree& tree, const std::string& filename, const bool append, ds::ui::SpriteEngine* engPtr) {
	if (!append) {
		directReadFromXml(tree,filename, true,engPtr);
		return;
	}

	Settings		s;
	s.directReadFromXml(tree, filename, false,engPtr);

	merge_settings(mSettings, s.mSettings);
}

void Settings::directReadFrom(const std::string& filename, const bool clearAll){
	if(clearAll) clear();

	if(filename.empty()) {
		return;
	}

	if(!safeFileExistsCheck(filename, false)){
		return;
	}


	ci::XmlTree xml;
	try{
		xml = ci::XmlTree(ci::loadFile(filename));
	} catch(std::exception& e){
		DS_LOG_WARNING("Exception loading settings from " << filename << " " << e.what());
		return;
	}

	directReadFromXml(xml, filename, clearAll);
}

void Settings::directReadFromXml(ci::XmlTree& tree, const std::string& referenceFilename,  const bool clearAll,ds::ui::SpriteEngine* engPtr) {
	if (clearAll) clear();

	ci::XmlTree& xml = tree;
	
	auto xmlEnd = xml.end();
	for (auto it = xml.begin("settings/setting"); it != xmlEnd; ++it) {
		if (!it->hasAttribute("name")) {
			DS_LOG_WARNING("Missing a name attribute for a setting!");
			continue;
		}

		if (engPtr) {
			if (it->hasAttribute("target")) {
				auto target = it->getAttributeValue<std::string>("target");
				if (!engPtr->hasLayoutTarget(target)) {
					continue;
				}
			}
		}

		std::string theName = it->getAttributeValue<std::string>("name");

		Setting theSetting;
		theSetting.mName = theName;
		theSetting.mReadIndex = mReadIndex;
		mReadIndex += SETTINGS_INCREMENT; // leave space for other settings to be inserted
		if (it->hasAttribute("value"))		theSetting.mRawValue = it->getAttributeValue<std::string>("value");
		if (it->hasAttribute("comment"))		theSetting.mComment = it->getAttributeValue<std::string>("comment");
		if (it->hasAttribute("default"))		theSetting.mDefault = it->getAttributeValue<std::string>("default");
		if (it->hasAttribute("min_value"))	theSetting.mMinValue = it->getAttributeValue<std::string>("min_value");
		if (it->hasAttribute("max_value"))	theSetting.mMaxValue = it->getAttributeValue<std::string>("max_value");
		if (it->hasAttribute("type"))		theSetting.mType = it->getAttributeValue<std::string>("type");
		if (it->hasAttribute("possibles"))	theSetting.mPossibleValues = it->getAttributeValue<std::string>("possibles");
		
		
		if (!validateType(theSetting.mType)) {
			DS_LOG_WARNING("Unknown setting type for " << theName << " type:" << theSetting.mType << " source: " << referenceFilename);
		}

		theSetting.mSource = referenceFilename;

		auto settingIndex = getSettingIndex(theName);
		if (settingIndex > -1 && !mSettings.empty()) {
			mSettings[settingIndex].second.push_back(theSetting);
		}
		else {
			std::vector<Setting> newSettingVec;
			newSettingVec.push_back(theSetting);
			mSettings.emplace_back(std::pair<std::string, std::vector<Setting>>(theName, newSettingVec));
		}
	}
}


void Settings::writeTo(const std::string& filename){

	ci::XmlTree xml = ci::XmlTree::createDoc();
	ci::XmlTree rootNode;
	rootNode.setTag("settings");
	auto& sortedSettings = getReadSortedSettings();
	for(auto sit : sortedSettings){
		ci::XmlTree settingNode;
		settingNode.setTag("setting");
		settingNode.setAttribute("name", sit.mName);
		settingNode.setAttribute("value", sit.mOriginalValue.empty()?sit.mRawValue:sit.mOriginalValue);
		if(!sit.mType.empty()) settingNode.setAttribute("type", sit.mType);
		if(!sit.mComment.empty()) settingNode.setAttribute("comment", sit.mComment);
		if(!sit.mDefault.empty()) settingNode.setAttribute("default", sit.mDefault);
		if(!sit.mMinValue.empty()) settingNode.setAttribute("min_value", sit.mMinValue);
		if(!sit.mMaxValue.empty()) settingNode.setAttribute("max_value", sit.mMaxValue);
		if(!sit.mPossibleValues.empty()) settingNode.setAttribute("possibles", sit.mPossibleValues);
		rootNode.push_back(settingNode);
	}
	

	xml.push_back(rootNode);
	xml.write(ci::writeFile(filename), true);
}

bool Settings::empty() const {
	return mSettings.empty();
}

void Settings::clear() {
	mSettings.clear();
}


const bool Settings::getBool(const std::string& name, const int index) {
	return getSetting(name, index).getBool();
}

const bool Settings::getBool(const std::string& name, const int index, const bool defaultValue){
	/// std::to_string was converting bool to int and returning 1 or 0
	std::string defaultString = "false";
	if(defaultValue) defaultString = "true";
	return getSetting(name, index, defaultString).getBool();
}

const int Settings::getInt(const std::string& name, const int index){
	return getSetting(name, index).getInt();
}

const int Settings::getInt(const std::string& name, const int index, const int defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getInt();
}

const float Settings::getFloat(const std::string& name, const int index){
	return getSetting(name, index).getFloat();
}

const float Settings::getFloat(const std::string& name, const int index, const float defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getFloat();
}

const double Settings::getDouble(const std::string& name, const int index){
	return getSetting(name, index).getDouble();
}

const double Settings::getDouble(const std::string& name, const int index, const double defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getDouble();
}

const ci::Color Settings::getColor(ds::ui::SpriteEngine& engine, const std::string& name, const int index){
	return getSetting(name, index).getColor(engine);
}

const ci::Color Settings::getColor(ds::ui::SpriteEngine& engine, const std::string& name, const int index, const ci::Color& defaultValue){
	return getSetting(name, index, ds::unparseColor(defaultValue)).getColor(engine);
}

const ci::ColorA Settings::getColorA(ds::ui::SpriteEngine& engine, const std::string& name, const int index){
	return getSetting(name, index).getColorA(engine);
}

const ci::ColorA Settings::getColorA(ds::ui::SpriteEngine& engine, const std::string& name, const int index, const ci::ColorA& defaultValue){
	return getSetting(name, index, ds::unparseColor(defaultValue)).getColorA(engine);
}

const std::string& Settings::getString(const std::string& name, const int index){
	return getSetting(name, index).getString();
}

const std::string& Settings::getString(const std::string& name, const int index, const std::string& defaultValue){
	return getSetting(name, index, defaultValue).getString();
}

const std::wstring Settings::getWString(const std::string& name, const int index){
	return getSetting(name, index).getWString();
}

const std::wstring Settings::getWString(const std::string& name, const int index, const std::wstring& defaultValue){
	return getSetting(name, index, ds::utf8_from_wstr(defaultValue)).getWString();
}

const ci::vec2 Settings::getVec2(const std::string& name, const int index){
	return getSetting(name, index).getVec2();
}

const ci::vec2 Settings::getVec2(const std::string& name, const int index, const ci::vec2& defaultValue){
	return getSetting(name, index, ds::unparseVector(defaultValue)).getVec2();
}

const ci::vec3 Settings::getVec3(const std::string& name, const int index){
	return getSetting(name, index).getVec3();
}

const ci::vec3 Settings::getVec3(const std::string& name, const int index, const ci::vec3& defaultValue){
	return getSetting(name, index, ds::unparseVector(defaultValue)).getVec3();
}

const cinder::Rectf Settings::getRect(const std::string& name, const int index){
	return getSetting(name, index).getRect();
}

const cinder::Rectf Settings::getRect(const std::string& name, const int index, const ci::Rectf& defaultValue){
	return getSetting(name, index, ds::unparseRect(defaultValue)).getRect();
}

bool Settings::validateType(const std::string& inputType) {
	return std::find(SETTING_TYPES.begin(), SETTING_TYPES.end(), inputType) != SETTING_TYPES.end();
}

bool sortByReadIndex(ds::cfg::Settings::Setting& a, ds::cfg::Settings::Setting& b){
	if(a.mReadIndex < b.mReadIndex){
		return true;
	} else if(a.mReadIndex == b.mReadIndex){
		return a.mName < b.mName;
	}

	return false;
}

std::vector<ds::cfg::Settings::Setting>& Settings::getReadSortedSettings(){
	mSortedSettings.clear();

	for(auto it : mSettings){
		auto theVec = it.second;
		for(auto sit : theVec){
			mSortedSettings.push_back(sit);
		}
	}

	std::sort(mSortedSettings.begin(), mSortedSettings.end(), sortByReadIndex);

	return mSortedSettings;
}

bool Settings::hasSetting(const std::string& name) const {
	for(auto it : mSettings){
		if(it.first == name) return true;
	}

	return false;
}

size_t Settings::countSetting(const std::string& name) const {
	for(auto it : mSettings) {
		if(it.first == name) return it.second.size();
	}

	return 0;
}

int Settings::getSettingIndex(const std::string& name) const {
	for(int i = 0; i < mSettings.size(); i++){
		if(mSettings[i].first == name) return i;
	}

	return -1;
}

void Settings::forEachSetting(const std::function<void(Setting&)>& func, const std::string& typeFilter /*= ""*/) {
	auto& theThingies = getReadSortedSettings();
	for(auto sit : theThingies){
		if(!typeFilter.empty() && typeFilter != sit.mType) continue;
		func(sit);
	}
}

ds::cfg::Settings::Setting& Settings::getSetting(const std::string& name, const int index) {
	return getSetting(name, index, "");
}

ds::cfg::Settings::Setting& Settings::getSetting(const std::string& name, const int index, const std::string& defaultRawValue){
	auto settingIndex = getSettingIndex(name);

	if(settingIndex > -1 && index > -1 && !mSettings[settingIndex].second.empty() && index < mSettings[settingIndex].second.size()){
		return mSettings[settingIndex].second[index];
	}

	// create a new blank setting and return that
	std::vector<Setting> settings;
	settings.push_back(Setting());
	settings.back().mName = name;
	settings.back().mRawValue = defaultRawValue;
	settings.back().mReadIndex = mReadIndex;
	mReadIndex += SETTINGS_INCREMENT;
	mSettings.emplace_back(std::pair<std::string, std::vector<Setting>>(name, settings));

	return mSettings.back().second.back();

}

ds::cfg::Settings::Setting& Settings::getSetting(const std::string& name, const int index, const std::string& settingType, 
												 const std::string& commentValue, const std::string& defaultRawValue /*= ""*/, 
												 const std::string& minValue /*= ""*/, const std::string& maxValue /*= ""*/, const std::string& possibleValues /*= ""*/){
	auto& theSetting = getSetting(name, index, defaultRawValue);
	theSetting.mType = settingType;
	theSetting.mComment = commentValue;
	theSetting.mDefault = defaultRawValue;
	theSetting.mMinValue = minValue;
	theSetting.mMaxValue = maxValue;
	theSetting.mPossibleValues = possibleValues;
	return theSetting;
}

void Settings::addSetting(const Setting& newSetting){
	auto settingIndex = getSettingIndex(newSetting.mName);

	if(settingIndex > -1 && !mSettings.empty()){
		mSettings[settingIndex].second.push_back(newSetting);
	} else {
		std::vector<Setting> theSettings;
		theSettings.push_back(newSetting);
		auto hintInsert = mSettings.end();
		mSettings.insert(hintInsert, std::pair<std::string, std::vector<Setting>>(newSetting.mName, theSettings));
	}
}

void Settings::printAllSettings(){
	std::cout << "Settings: " << std::endl;
	auto& theVec = getReadSortedSettings();
	for(auto sit : theVec){
		std::cout << std::endl << "\t" << sit.mName << ": \t" << sit.mRawValue << std::endl;
		std::cout << "\t\t source: \t" << sit.mSource << std::endl;

		if(!sit.mComment.empty()) std::cout << "\t\t comment: \t" << sit.mComment << std::endl;
		if(!sit.mType.empty()) std::cout << "\t\t type: \t\t" << sit.mType << std::endl;
		if(!sit.mDefault.empty()) std::cout << "\t\t default: \t" << sit.mDefault << std::endl;
		if(!sit.mMinValue.empty()) std::cout << "\t\t min: \t\t" << sit.mMinValue << std::endl;
		if(!sit.mMaxValue.empty()) std::cout << "\t\t max: \t\t" << sit.mMaxValue << std::endl;
		if(!sit.mPossibleValues.empty()) std::cout << "\t\t possibles: \t\t" << sit.mPossibleValues << std::endl;
	}
	
}

void Settings::replaceSettingVariablesAndExpressions() {
	for (auto& it : mSettings) {
		auto& theVec = it.second;
		for (auto& theSetting : theVec) {
			theSetting.replaceSettingVariablesAndExpressions();
		}
	}
}

} // namespace cfg
} // namespace ds