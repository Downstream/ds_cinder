#pragma once

#include <ds/ui/panel/base_panel.h>

namespace ds {
namespace ui {

/**
* \class PanelLayouts
*			Generate layouts based on the current open viewers
*/
class PanelLayouts  {
public:

	// Tries to fit stuff as tight as possible on the whole screen
	// This is a fuzzy algorithm that may resize the panels to fit (generally proportionally)
	// There's some heuristics involved, and the algorithm will try to use between 75% and 90% of the space available
	// Panels are not rotated in any way
	// Min/max size may not be respected :( TODO
	/// \param panels is the vector of panels to layout
	/// \param totalArea is the area to fit the panels into
	/// \param padding is the space between the panels in pixels
	/// \param animationDuration is how long to take to move the panels to their destination. duration <= 0.0 will place the viewers immediately
	static bool	binPack(std::vector<ds::ui::BasePanel*> panels, const ci::Rectf totalArea, const float padding = 5.0f, const float animationDuration = 0.35f);

	// Fits the panels into rows as best as it can
	// adjusts the height of the panels to fit into the rows
	static bool	rowPack(std::vector<ds::ui::BasePanel*> panels, const ci::Rectf totalArea, const float padding = 5.0f, const float animationDuration = 0.35f, const int numRows = 2);

};

} // namespace ui
} // namespace ds

