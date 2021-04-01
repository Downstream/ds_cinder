#include "stdafx.h"

#include "ds/data/color_list.h"

namespace ds {

namespace {
const ci::ColorA		EMPTY_COL(255, 0, 255, 255);
}

ColorList::ColorList(){
}

void ColorList::clear(){
	mData.clear();
}

bool ColorList::empty() const{
	return (mData.empty());
}

void ColorList::install(const ci::ColorA& color, const std::string& shortName){
	mData[shortName] = color;
}

const ci::ColorA& ColorList::getColorFromName(const std::string& n) const{
	if(mData.empty()) return EMPTY_COL;
	auto findy = mData.find(n);
	if(findy != mData.end()){
		return findy->second;
	}
	return EMPTY_COL;
}

const ci::ColorA& ColorList::getColorFromName(const std::wstring& namey) const {
	return getColorFromName(ds::utf8_from_wstr(namey));
}


std::string ColorList::getNameFromColor(const ci::ColorA& theColor) const {
	if (mData.empty()) return "";
	for (auto it : mData) {
		if (it.second == theColor) return it.first;
	}

	return "";
}

} // namespace ds
