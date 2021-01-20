#pragma once

#include <ds/ui/layout/smart_layout.h>

namespace downstream {

/**
* \class downstream::ScrollController
*			Adds a bunch of scroll things to the screen
*/
class ScrollController : public ds::ui::SmartLayout {
public:
	ScrollController(ds::ui::SpriteEngine& eng);

	void setData();
};

} 


