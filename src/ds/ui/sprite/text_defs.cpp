#include "stdafx.h"

#include "ds/ui/sprite/text_defs.h"
#include "ds/util/color_util.h"

namespace ds {
namespace ui {

// Alignment
Alignment::Enum Alignment::fromString(const std::string& str) {
	if(!str.empty()) {
		if(str[0] == 'R' || str[0] == 'r') return kRight;
		if(str[0] == 'C' || str[0] == 'c') return kCenter;
		if(str[0] == 'J' || str[0] == 'j') return kJustify;
	}
	return Alignment::kLeft;
}


std::string Alignment::toString(Alignment::Enum& align) {
	if (align == Alignment::kLeft) return "left";
	if (align == Alignment::kRight) return "right";
	if (align == Alignment::kCenter) return "center";
	if (align == Alignment::kJustify) return "justify";
	return "left";
}

TextStyle::TextStyle(const std::string& font, const std::string& configName, const double size, const double leading,
	const double letterSpacing, const ci::ColorA& c, const ds::ui::Alignment::Enum& alignment)
	: mFont(font)
	, mName(configName)
	, mSize(size)
	, mLeading(leading)
	, mLetterSpacing(letterSpacing)
	, mColor(c)
	, mAlignment(alignment) 
	, mFitMinTextSize(0.0)
	, mFitMaxTextSize(0.0)
{}


/// The canonical defaults
TextStyle::TextStyle()
	: mFont("Sans")
	, mName("default")
	, mSize(12.0)
	, mLeading(1.0)
	, mLetterSpacing(0.0)
	, mColor(ci::ColorA::white())
	, mColorName("white")
	, mAlignment(Alignment::kLeft)
	, mFitMinTextSize(0.0)
	, mFitMaxTextSize(0.0) 
{}

std::vector<double> TextStyle::getSizesFromString(std::string value){
	std::regex e3(",+");
	auto itr = std::sregex_token_iterator(value.begin(), value.end(), e3, -1);
	std::vector<double> size_values;
	double font_value;

	for (; itr != std::sregex_token_iterator(); ++itr) {
		if (ds::string_to_value<double>(itr->str(), font_value)) {
			size_values.push_back(font_value);
		}
	}

	return size_values;
}


ds::ui::TextStyle TextStyle::textStyleFromSetting(ds::ui::SpriteEngine& engine, const std::string& settingString) {
	TextStyle theStyle;
	// find all the settings in the string
	std::vector<std::string> settings = ds::split(settingString, "; ", true);

	if (settings.empty()) return theStyle;

	for (auto it : settings) {

		auto foundy = it.find_first_of(":");
		if (foundy == std::string::npos) {
			DS_LOG_WARNING("TextStyle: Syntax error: colon not found in text style for setting: " << settingString);
			continue;
		}

		/// note that we don't just split on ":" for the key/values because a lot of text style names have colons in them
		auto theKey = it.substr(0, foundy + 1);
		auto remainderStr = it.substr(foundy + 1);

		 
		if(theKey == "name:"){
			theStyle.mName = remainderStr;

		} else if (theKey == "color:") {
			theStyle.mColor = parseColor(remainderStr, engine);
			theStyle.mColorName = remainderStr;

		} else if(theKey == "font:" || theKey == "font_name:") {
			theStyle.mFont = remainderStr;

		} else if (theKey == "size:" || theKey == "font_size:") {
			theStyle.mSize = ds::string_to_double(remainderStr);

		} else if (theKey == "letter_spacing:" || theKey == "font_letter_spacing:") {
			theStyle.mLetterSpacing = ds::string_to_double(remainderStr);

		} else if (theKey == "leading:" || theKey == "font_leading:") {
			theStyle.mLeading = ds::string_to_double(remainderStr);

		} else if (theKey == "align:" || theKey == "text_align:" || theKey == "alignment:") {
			theStyle.mAlignment = Alignment::fromString(remainderStr);

		} else if (theKey == "fit_font_size_range:" || theKey == "fit_size_range:") {
			ci::vec3 v = parseVector(remainderStr);
			theStyle.mFitMinTextSize = v.x;
			theStyle.mFitMaxTextSize = v.y;

		} else if (theKey == "fit_font_sizes:" || theKey == "fit_sizes:") {
			theStyle.mFitSizes = getSizesFromString(remainderStr);
		}
		
	}

	return theStyle;
}


std::string TextStyle::settingFromTextStyle(ds::ui::SpriteEngine& engine, TextStyle& theStyle, const bool includeName) {
	std::string theStyleString;
	std::vector<std::string> items;
	TextStyle aDefaultStyle;

	if (includeName && theStyle.mName != aDefaultStyle.mName) {
		items.emplace_back("name:" + theStyle.mName);
	}

	if (theStyle.mFont != aDefaultStyle.mFont) {
		items.emplace_back("font:" + theStyle.mFont);
	}

	if (theStyle.mSize != aDefaultStyle.mSize) {
		items.emplace_back("size:" + ds::to_string_with_precision(theStyle.mSize, 1));
	}

	if (theStyle.mColor != aDefaultStyle.mColor) {
		items.emplace_back("color:" + unparseColor(theStyle.mColor, engine));
	}

	if (theStyle.mLetterSpacing != aDefaultStyle.mLetterSpacing) {
		items.emplace_back("letter_spacing:" + ds::to_string_with_precision(theStyle.mLetterSpacing, 3));
	}

	if (theStyle.mLeading != aDefaultStyle.mLeading) {
		items.emplace_back("leading:" + ds::to_string_with_precision(theStyle.mLeading, 3));
	}

	if (theStyle.mAlignment != aDefaultStyle.mAlignment) {
		items.emplace_back("align:" + Alignment::toString(theStyle.mAlignment));
	}

	if (theStyle.mFitMinTextSize != aDefaultStyle.mFitMinTextSize
		|| theStyle.mFitMaxTextSize != aDefaultStyle.mFitMaxTextSize) {
		items.emplace_back("fit_size_range:" + std::to_string(theStyle.mFitMinTextSize) + ", " + std::to_string(theStyle.mFitMaxTextSize));
	}

	if (!theStyle.mFitSizes.empty()) {
		std::string theSizes;
		for (int i = 0; i < theStyle.mFitSizes.size(); i++) {
			if (i != 0) {
				theSizes.append(", ");
			}
			theSizes.append(std::to_string(theStyle.mFitSizes[i]));
		}

		items.emplace_back("fit_sizes:" + theSizes);
	}

	for (auto it : items) {
		if(it != items.front()){
			theStyleString.append("; ");
		}
		theStyleString.append(it);
	}

	return theStyleString;
}

} // namespace ui
} // namespace ds
