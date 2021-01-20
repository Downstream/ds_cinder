#pragma once
#ifndef DS_UTIL_COLORUTIL_H_
#define DS_UTIL_COLORUTIL_H_

#include <cinder/Color.h>
#include <ds/ui/sprite/sprite_engine.h>

namespace ds {


	/** Read a colour from a string of the format \code \#rrggbb, xrrggbb or rrggbb. For example: \#ffcc00. \endcode Returns black if the color can't be parsed */
	ci::Color  parse_color(const std::string& input);

	/** Read a colour from a string of the format \code \#rrggbb, xrrggbb or rrggbb. For example: \#ffcc00. \endcode Returns the defaultColor if the string can't be found.*/
	ci::Color  parse_color(const std::string& input, const ci::Color& defaultColor);

	/** Read a colour from a string of the format \code \#rrggbbaa, xrrggbbaa or rrggbbaa. For example: \#ffcc0033. \endcode Returns black if the color can't be parsed */
	ci::ColorA parse_colora(const std::string& input);

	/** Read a colour from a string of the format  \code \#rrggbbaa, xrrggbbaa or rrggbbaa. For example: \#ffcc0033. \endcode  Returns the defaultColor if the string can't be found. */
	ci::ColorA parse_colora(const std::string& input, const ci::ColorA& defaultColor);


	/** Returns a string of the format \code \#AARRGGBB from a ci::Color */
	std::string ARGBToHex(ci::ColorA color);
	/** Returns a string of the format \code \#AARRGGBB from a ci::Color */
	std::string ARGBToHex(int aNum, int rNum, int gNum, int bNum);
	/** Returns a string of the format \code \#AARRGGBB from a ci::Color */
	std::string RGBToHex(ci::Color color);
	/** Returns a string of the format \code \#AARRGGBB from a ci::Color */
	std::string RGBToHex(int rNum, int gNum, int bNum);

	/** Straight-up string to ci::ColorA conversion, Color format: \#AARRGGBB OR \#RRGGBB OR AARRGGBB OR RRGGBB. Example: ff0033 or \#9933ffbb */
	ci::ColorA parseHexColor(const std::string &color);
	/** Checks engine named colors, then does a string convert if it's not found. Color format: \#AARRGGBB OR \#RRGGBB OR AARRGGBB OR RRGGBB. Example: ff0033 or \#9933ffbb */
	ci::ColorA parseColor(const std::string &color, const ds::ui::SpriteEngine& engine);
	ci::ColorA parseColor(const std::wstring &color, const ds::ui::SpriteEngine& engine);

	/** Returns a string of the format \code \#AARRGGBB from a ci::Color */
	std::string unparseColor(const ci::ColorA& color);
	/** Returns the name of an engine color if the color matches one, or a string of the format \code \#AARRGGBB from a ci::Color */
	std::string unparseColor(const ci::ColorA& color, ds::ui::SpriteEngine& engine);

} // namespace ds

#endif // DS_UTIL_COLORUTIL_H_
