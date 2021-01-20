#pragma once

#include <map>
#include <vector>
#include <cinder/Color.h>
#include <cinder/Rect.h>
#include "ds/data/resource.h"

#include <ds/app/event.h>

namespace cinder {
class XmlTree;
}

namespace ds {
class App;
namespace ui {
class SpriteEngine;
}

namespace cfg {

extern const std::string&				SETTING_TYPE_UNKNOWN;
extern const std::string&				SETTING_TYPE_BOOL;
extern const std::string&				SETTING_TYPE_INT;
extern const std::string&				SETTING_TYPE_FLOAT;
extern const std::string&				SETTING_TYPE_DOUBLE;
extern const std::string&				SETTING_TYPE_STRING;
extern const std::string&				SETTING_TYPE_WSTRING;
extern const std::string&				SETTING_TYPE_COLOR; // covers color and colora
extern const std::string&				SETTING_TYPE_VEC2;
extern const std::string&				SETTING_TYPE_VEC3;
extern const std::string&				SETTING_TYPE_RECT;
extern const std::string&				SETTING_TYPE_SECTION_HEADER; // A meta type of setting for ui display
extern const std::string&				SETTING_TYPE_TEXT_STYLE; // A semicolon-separated list of text style info (e.g. font:Arial; size:20)

/**
* \class Settings
* \brief Store generic settings info.
*/
class Settings {
public:
	Settings();



	/// An actual setting with some metadata. 
	struct Setting {
		Setting() : mType(SETTING_TYPE_UNKNOWN), mReadIndex(-1){};

		/// Type conversion happens at read time for all getters
		bool							getBool() const;
		int								getInt() const;
		float							getFloat() const;
		double							getDouble() const;

		/// The Engine is supplied to look up named colors
		const ci::Color					getColor(ds::ui::SpriteEngine&) const;
		const ci::ColorA				getColorA(ds::ui::SpriteEngine&) const;

		const std::string&				getString() const;
		const std::wstring				getWString() const;

		const ci::vec2					getVec2() const;
		const ci::vec3					getVec3() const;
		const cinder::Rectf				getRect() const;

		std::vector<std::string>		getPossibleValues() const;
		/// Goes through each setting to replace variables and parse expressions
		void							replaceSettingVariablesAndExpressions();


		/// \code <setting name="the_name" value="sample" type="string" comment="Detailed description" default="null" /> \endcode
		/// \code <setting name="the_name" value="1.0" type="double" min_value="0.01" max_value="10.0" default="1.5" /> \endcode

		/// The name of the setting. Multiple values are getted by the index value in the getters below
		std::string						mName;

		/// The raw value attribute is saved as a string and type converted when got
		std::string						mRawValue;

		/// The raw value attribute is saved as a string and type converted when got
		std::string						mOriginalValue;

		/// A helpful description of this setting for any GUI settings editors or just reading the xml
		std::string						mComment;

		/// Optional type hinting for GUI editors or human readers. All getters are still available regardless of this value
		std::string						mType;

		/// Optional defaults, min and max for GUI editors or as a hint when editing. 
		/// Getting the value does NOT clip to the min/max
		std::string						mDefault;
		std::string						mMinValue;
		std::string						mMaxValue;

		/// Optional comma-separated list of possible values. E.g. "borderless, window, fullscreen"
		std::string						mPossibleValues;

		/// auto determined by the file path of the settings file
		std::string						mSource;

		/// an id that's auto-assigned to this setting to determine overall sort order
		unsigned int					mReadIndex;
	};
	/// static method to merge settings
	void mergeSettings(Settings& mergeIn);

	/// The name of these settings (e.g. engine, layout, text, etc)
	void								setName(const std::string& theType){ mName = theType; }
	const std::string&					getName(){ return mName;  }

	/// TODO: add ability to load all settings locations right from here
	/// Read from an xml from the full file path. If append is true, will merge with any existing settings
	void								readFrom(const std::string& fullFilePath, const bool append = true);
	void								readFrom(ci::XmlTree& tree, const std::string& fullFilePath, const bool append = true);

	/// Writes the current settings out the file path
	void								writeTo(const std::string&fullFilePath);

	/// If there's no settings available
	bool								empty() const;

	/// Remove all settings
	void							  	clear();

	/// All getters type convert from the raw string value when you get the value
	/// If the setting doesn't exist when you get it, it will be created. Supplying the default value will apply that value to the new setting

	/// \code <setting name="the_name" value="true" type="bool" /> // index 0 \endcode
	/// \code <setting name="the_name" value="false" type="bool" /> // index 1 \endcode
	const bool							getBool(const std::string& name, const int index = 0);
	const bool							getBool(const std::string& name, const int index, const bool defaultValue);


	/// \code <setting name="the_name" value="1" type="int" min_value="0" max_value="1000" default="5" />  \endcode
	const int							getInt(const std::string& name, const int index = 0);
	const int							getInt(const std::string& name, const int index, const int defaultValue);

	/// \code <setting name="the_name" value="1.0" type="float" />  \endcode
	const float							getFloat(const std::string& name, const int index = 0);
	const float							getFloat(const std::string& name, const int index, const float defaultValue);

	/// \code <setting name="the_name" value="10.0000000000000000001" type="double" />  \endcode
	const double						getDouble(const std::string& name, const int index = 0);
	const double						getDouble(const std::string& name, const int index, const double defaultValue);

	/// Color format: \#AARRGGBB OR \#RRGGBB OR AARRGGBB OR RRGGBB. Example: ff0033 or \#9933ffbb 
	/// Can also use named engine colors like "red" or "horrible_off_pink_brand_color"
	/// This will ignore the alpha value when returning the color
	/// \code <setting name="the_name" value="123456" type="color" />  \endcode
	const ci::Color						getColor(ds::ui::SpriteEngine& engine, const std::string& name, const int index = 0);
	const ci::Color						getColor(ds::ui::SpriteEngine& engine, const std::string& name, const int index, const ci::Color& defaultValue);

	/// Color format: \#AARRGGBB OR \#RRGGBB OR AARRGGBB OR RRGGBB. Example: ff0033 or \#9933ffbb 
	/// Can also use named engine colors like "red" or "horrible_off_pink_brand_color"
	/// This one retains the alpha value
	/// \code <setting name="the_name" value="12345678" type="colora" />  \endcode
	const ci::ColorA					getColorA(ds::ui::SpriteEngine& engine, const std::string& name, const int index = 0);
	const ci::ColorA					getColorA(ds::ui::SpriteEngine& engine, const std::string& name, const int index, const ci::ColorA& defaultValue);

	/// \code <setting name="the_name" value="What about the droid attack on the wookie army?" type="string" />  \endcode
	const std::string&					getString(const std::string& name, const int index = 0);
	const std::string&					getString(const std::string& name, const int index, const std::string& defaultValue);

	/// \code <setting name="the_name" value="I hate sand, it's course and rough and irritating!" type="string" />  \endcode
	const std::wstring					getWString(const std::string& name, const int index = 0);
	const std::wstring					getWString(const std::string& name, const int index, const std::wstring& defaultValue);

	/// vec2 format value="X, Y" The space after the comma is required. Y defaults to 0.0 if it's not present
	/// \code <setting name="the_name" value="140, 100" type="vec2" />  \endcode
	const ci::vec2						getVec2(const std::string& name, const int index = 0);
	const ci::vec2						getVec2(const std::string& name, const int index, const ci::vec2& defaultValue);

	/// vec3 format value="X, Y, Z" The space after the commas are required. Y and Z default to 0.0 if not present
	/// \code <setting name="the_name" value="-1.0, -1000.0, 50" type="vec3" />  \endcode
	const ci::vec3						getVec3(const std::string& name, const int index = 0);
	const ci::vec3						getVec3(const std::string& name, const int index, const ci::vec3& defaultValue);

	/// rect format value="L, T, W, H" The space after the commas are required.
	/// \code <setting name="the_name" value="0, 0, 1920, 1080" type="rect" />  \endcode
	const cinder::Rectf					getRect(const std::string& name, const int index = 0);
	const cinder::Rectf					getRect(const std::string& name, const int index, const ci::Rectf& defaultValue);

	/// Gets a reference to a raw setting for full access to properties like comments, min, max, etc.
	/// Returns a new setting with the name specified (though the index is ignored when creating a new setting)
	Setting&							getSetting(const std::string& name, const int index);

	/// Gets a reference to a raw setting for full access to properties like comments, min, max, etc.
	/// Returns a new setting with the name specified (though the index is ignored when creating a new setting). Applies the default to the new setting
	Setting&							getSetting(const std::string& name, const int index, const std::string& defaultRawValue);

	/// Gets a reference to a raw setting, but also applies all values. This is great for making canonical settings in c++ instead of storing them in xml
	Setting&							getSetting(const std::string& name, const int index, const std::string& settingType, const std::string& commentValue, 
												   const std::string& defaultRawValue = "", const std::string& minValue = "", const std::string& maxValue = "", const std::string& possibleValues = "");

	/// Appends the setting to the end of the setting list.
	/// Note: set mReadIndex correctly if you want to insert this new setting inside the overall list
	void								addSetting(const Setting& newSetting);

	/// Returns the index of the name of the setting in the vector of all settings (not the index of the same setting)
	/// Return -1 if the setting isn't there or there are no settings
	/// NOTE: This is NOT NOT NOT the same index as above. This is the index of ALL setting names. The indices above are for settings with the same name
	int									getSettingIndex(const std::string& name) const;

	/// Iterate over all the settings, optionally filtering by a specific type
	/// The order they are returned is the overall order they were originally read in (the same order returned by getReadSortedSettings())
	void								forEachSetting(const std::function<void(Setting&)>&, const std::string& typeFilter = "");

	/// Prints out information for all settings
	void								printAllSettings();

	/// If a setting with this name exists
	bool								hasSetting(const std::string& name) const;

	/// How many settings there are with this name
	size_t								countSetting(const std::string& name) const;

	/// Validate if the type string is known (int, float, string, section_header, etc)
	static bool							validateType(const std::string& inputType);

	/// Returns a reference to a vector of all Settings sorted by the order they were added (or if their mReadIndex value was changed later)
	/// This vector may change at any time, so careful here
	std::vector<Setting>&				getReadSortedSettings();

	/// Goes through each setting to replace variables and parse expressions
	void								replaceSettingVariablesAndExpressions();

	class SettingsEditedEvent : public ds::RegisteredEvent<SettingsEditedEvent> {
	public:
		SettingsEditedEvent(const std::string& settingsType, const std::string& settingName)
			: mSettingsType(settingsType), mSettingName(settingName){}
		const std::string& mSettingsType;
		const std::string& mSettingName;
	};

protected:
	friend class ds::App;

	/// The first vector is all settings
	/// The pair is to match the name of the setting
	/// The inner vector is for a series of settings with the same name (to support the index calls in the getSetting() calls)
	std::vector<std::pair<std::string, std::vector<Setting>>>			mSettings;

	std::string															mName;
	unsigned int														mReadIndex;
	std::vector<Setting>												mSortedSettings; // rebuilt every call of getReadSortedIndex()

	/// Used in the read function
	void								directReadFrom(const std::string& filename, const bool clear); 
	void								directReadFromXml(ci::XmlTree& tree, const std::string& referenceFilename ,const bool clear); 


};

} // namespace cfg
} // namespace ds
