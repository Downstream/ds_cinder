#include "stdafx.h"

#include "color_util.h"

#include <ds/data/color_list.h>
#include <ds/util/string_util.h>

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include <ds/debug/logger.h>

namespace ds {

namespace {

int				parse_component(const std::string& input, const size_t pos, const int defaultValue = 0) {
	if (input.size() <= pos+1) return defaultValue;

	unsigned int x;
	const std::string	hex(input.substr(pos, 2));
	std::stringstream ss;
	ss << std::hex << hex;
	ss >> x;
	int		ans = static_cast<int>(x);
	if (ans <= 0) return 0;
	if (ans >= 255) return 255;
	return ans;
}

}

ci::Color parse_color(const std::string& input) {
	const ci::ColorA	ca(parse_colora(input));
	return ci::Color(ca.r, ca.g, ca.b);
}

ci::Color parse_color(const std::string& input, const ci::Color& dv) {
	try {
		return parse_color(input);
	} catch (std::exception const&) {
	}
	return dv;
}

ci::ColorA parse_colora(const std::string& input) {
	if(input.size() < 3){
		DS_LOG_WARNING("parse_colora() invalid input (" + input + ")");
		return ci::ColorA::black();
	}
	// Hex
	if (input[0] == '#' || input[0] == 'x') {
		const int	r = parse_component(input, 1, 0);
		const int	g = parse_component(input, 3, 0);
		const int	b = parse_component(input, 5, 0);
		const int	a = parse_component(input, 7, 255);
		return ci::ColorA(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
	// Common input formats
	} else if (input.size() == 6) {
		const int	r = parse_component(input, 0, 0);
		const int	g = parse_component(input, 2, 0);
		const int	b = parse_component(input, 4, 0);
		return ci::ColorA(r/255.0f, g/255.0f, b/255.0f, 1.0f);
	}
	DS_LOG_WARNING("parse_color() invalid input (" + input + ")");
	return ci::ColorA::black();
}

ci::ColorA parse_colora(const std::string& input, const ci::ColorA& dv) {
	try {
		return parse_colora(input);
	} catch (std::exception const&) {
	}
	return dv;
}


std::string unparseColor(const ci::ColorA& color){
	return ARGBToHex(color);
}

std::string unparseColor(const ci::ColorA& color, ui::SpriteEngine& engine) {
	auto engineColor = engine.getColors().getNameFromColor(color);
	if(engineColor.empty()){
		return unparseColor(color);
	}

	return engineColor;	
}

std::string ARGBToHex(ci::ColorA theColor){
	return ARGBToHex((int)(theColor.a * 255.0f), (int)(theColor.r * 255.0f), (int)(theColor.g * 255.0f), (int)(theColor.b * 255.0f));
}

std::string ARGBToHex(int aNum, int rNum, int gNum, int bNum){
	std::stringstream ss;
	ss
		<< '#'
		<< std::hex << std::setfill('0')
		<< std::setw(2) << (aNum & 0xff)
		<< std::setw(2) << (rNum & 0xff)
		<< std::setw(2) << (gNum & 0xff)
		<< std::setw(2) << (bNum & 0xff);

	return ss.str();
}

std::string RGBToHex(ci::Color theColor){
	return RGBToHex((int)(theColor.r * 255.0f), (int)(theColor.g * 255.0f), (int)(theColor.b * 255.0f));
}

std::string RGBToHex(int rNum, int gNum, int bNum){
	std::stringstream ss;
	ss
		<< '#'
		<< std::hex << std::setfill('0')
		<< std::setw(2) << (rNum & 0xff)
		<< std::setw(2) << (gNum & 0xff)
		<< std::setw(2) << (bNum & 0xff);
	return ss.str();
}

// Grabs a color from the engine's supplied color list
static ci::ColorA retrieveColorFromEngine(const std::string &color, const ds::ui::SpriteEngine& engine){
	return engine.getColors().getColorFromName(color);
}

// Color format: #AARRGGBB OR #RRGGBB OR AARRGGBB OR RRGGBB. Example: ff0033 or #9933ffbb
ci::ColorA parseHexColor(const std::string &color) {

	std::string s = color;

	if(boost::starts_with(s, "#"))
		boost::erase_head(s, 1);

	std::stringstream converter(s);
	unsigned int value;
	converter >> std::hex >> value;

	float a = (s.length() > 6)
		? ((value >> 24) & 0xFF) / 255.0f
		: 1.0f;
	float r = ((value >> 16) & 0xFF) / 255.0f;
	float g = ((value >> 8) & 0xFF) / 255.0f;
	float b = ((value)& 0xFF) / 255.0f;

	return ci::ColorA(r, g, b, a);
}

ci::ColorA parseColor(const std::string &color, const ds::ui::SpriteEngine& engine){
	std::string s = color;

	//If we have colors in our engine, and this isn't a hex value
	if(!engine.getColors().empty() && !boost::starts_with(s, "#")){
		return retrieveColorFromEngine(color, engine);
	}

	return parseHexColor(color);

}

ci::ColorA parseColor(const std::wstring &color, const ui::SpriteEngine& engine){
	return parseColor(ds::utf8_from_wstr(color), engine);
}

} // namespace ds
