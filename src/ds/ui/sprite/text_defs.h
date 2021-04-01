#pragma once

#include <string>

namespace ds {
namespace ui {

namespace Alignment {
	enum Enum {
		kLeft,
		kRight,
		kCenter,
		kJustify
	};

	Alignment::Enum	fromString(const std::string&);
	std::string toString(Alignment::Enum& align);
}

enum class EllipsizeMode : int {
	kEllipsizeNone = 0, // Doesn't add ellipses
	kEllipsizeStart = 1, // Adds ellipses to the beginning of the text if it doesn't fit in the resize limit
	kEllipsizeMiddle = 2, // Adds ellipses to the middle of the text if it doesn't fit in the resize limit
	kEllipsizeEnd = 3 // Adds ellipses to the end of the text if it doesn't fit in the resize limit
};


enum class WrapMode : int {
	kWrapModeWord = 0, // Wraps only on word boundaries
	kWrapModeChar = 1, // Wraps on each character
	kWrapModeOff = 2, //Turns of word wrap
	kWrapModeWordChar  // Wraps on words, and falls back to char if it doesn't fit 
};


/**
* \class TextStyle
* Store config settings for a text style
*/
struct TextStyle{
	TextStyle();
	TextStyle(const std::string& font, const std::string& configName, const double size, const double leading,
		const double letterSpacing, const ci::ColorA&, const ds::ui::Alignment::Enum& = ds::ui::Alignment::kLeft);


	/// String options (these are obvious, right?)
	/*
		"name:",
		"color:",
		"font:", "font_name:",
		"size:", "font_size:",
		"letter_spacing:", "font_letter_spacing:",
		"leading:", "font_leading:",
		"align:", "text_align:", "alignment:"
		"fit_font_size_range:", "fit_size_range:"
		"fit_font_sizes:", "fit_sizes:" 
		*/

	/// Creates a TextStyle from a semicolon-separated string such as 
	/// "name:Default; font:Arial; size:20; color:white; letter_spacing:0.0; leading:1.0; fit_font_size_range:12, 36; fit_font_sizes:12, 24, 27, 30, 36; align:center"
	static TextStyle textStyleFromSetting(ds::ui::SpriteEngine& engine, const std::string& settingString);

	/// Creates a semicolor-separated string 
	static std::string settingFromTextStyle(ds::ui::SpriteEngine& engine, TextStyle& theStyle, const bool includeName = false);

	/// Returns a vector of doubles from a comma-separated size list, e.g. "12, 24, 36"
	static std::vector<double> getSizesFromString(std::string value);

	/// The name of this text style such as "case_study:title" or "media_viewer:page_count"
	std::string				mName;

	/// The font face name, e.g. "Arial", "Helvetica Neue Bold"
	std::string				mFont;
	
	/// The font size in pixels
	double					mSize;

	/// The space between rows of text as a percentage of 1.0.
	/// 1.0 will be the default space, higher than 1.0 is more space, less than 1.0 is less space
	/// 1.2 is 120% of the normal space
	double					mLeading;

	/// The space between each letter, defaulting at 0.0
	/// Negative numbers tighten the space
	/// Positive numbers add more space between letters
	double					mLetterSpacing;

	/// The default color of the font, can be overridden by <span> tags
	ci::ColorA	  			mColor;
	std::string				mColorName;

	/// How the text aligns, left, center, right, justify
	ds::ui::Alignment::Enum	mAlignment;

	/// Options of sizes when fitting the text to the resize limit, optional for fit sizing
	/// This allows you to keep the text at pre-determined sizes and still expand or contract to fit a space
	std::vector<double>		mFitSizes;

	/// Upper limit of font size when fitting to resize limits
	double					mFitMaxTextSize;

	/// Lower limit of font size when fitting to resize limits
	double					mFitMinTextSize;
};

} // namespace ui
} // namespace ds
