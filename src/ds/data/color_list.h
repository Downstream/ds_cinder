#pragma once

#include <string>
#include <unordered_map>
#include "cinder/Color.h"

namespace ds {

class ColorList
{
public:
	ColorList();

	void					clear();
	bool					empty() const;

	/// Short name can be supplied by the app and used to refer to colors from now on.
	/// Often it might be something in a settings file.
	void					install(const ci::ColorA& color, const std::string& shortName);

	/// Clients give either a shortname and I give them a color
	const ci::ColorA&		getColorFromName(const std::string&) const;
	const ci::ColorA&		getColorFromName(const std::wstring&) const;

private:
	std::unordered_map<std::string, ci::ColorA> mData;
};

} // namespace ds
