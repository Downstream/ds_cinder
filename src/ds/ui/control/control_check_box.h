#pragma once
#ifndef DS_UI_CONTROL_CONTROL_CHECK_BOX
#define DS_UI_CONTROL_CONTROL_CHECK_BOX

#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/border.h>
#include <ds/ui/sprite/text.h>
#include <functional>

namespace ds {
namespace ui {



///  *--------------* <--- This sprite (invisible)
///  |              | <--- Touch padding
///  |   *------*   | <--- Outer box (always visible)
///  |   | **** |   | <--- Inner box (visible if checked)
///  |   | **** |   | <--- Distance between inner box and outer box is boxPadding
///  |   *------*   |		
///  |	            |      Default is 50x50, touchPadding=15, boxPadding=5
///  *--------------*

/// An onscreen checkbox to control a bool
class ControlCheckBox : public ds::ui::Sprite {
public:

	/// Make a checkbox with a light grey border and x. Automatically sizes to 50x50, change the size of this sprite to control the box size (the height is used for box size calculation)
	ControlCheckBox(ds::ui::SpriteEngine& engine);

	/// The checkbox has been 
	void								setCheckboxUpdatedCallback(std::function<void(const bool isChecked)> func);

	/// Set the checked state of the box
	void								setCheckBoxValue(const bool isChecked);
	const bool							getCheckBoxValue(){ return mIsChecked; }

	/// Get the sprite that's the box in the middle
	/// Don't release this. 
	/// Add any UI for the background to this sprite, or modify the visible parameters (transparent, color, opacity, etc)
	ds::ui::Sprite*						getInnerBoxSprite();

	/// Get the sprite that's the outer box. Automatically sized to the size of this sprite
	/// Don't release this. 
	/// Add any UI for the nub to this sprite or modify the visible parameters
	ds::ui::Border*						getOuterBoxSprite();

	/// Gets the label sprite, returns nullptr if setLabelTextConfig() hasn't been called
	ds::ui::Text*						getLabelSprite();

	/// Adds space between this (the touch handler) and the box 
	void								setTouchPadding(const float touchPadding);

	/// Adds space the inner box and the outer box
	void								setBoxPadding(const float touchPadding);

	/// Adds a label (true / false, or whatever you set in setLabelLabels()) with the given text config
	void								setLabelTextStyle(const std::string& styleName);
	void								setLabelTextStyle(const ds::ui::TextStyle& textConfig);
	void								setLabelTextStyle(const std::string& fontName, const float& fontSize, const ci::ColorA& fontColor);

	/// Extra distance between the check box and the label (default is 0)
	void								setLabelPadding(const float labelPadding);

	/// Sets the text that the label displays when checked/unchecked
	void								setLabelLabels(const std::wstring& trueLabel, const std::wstring& falseText);
	void								setTrueLabel(const std::wstring& trueLabel);
	void								setFalseLabel(const std::wstring& falseText);

protected:
	virtual void						onSizeChanged();
	virtual void						layout();
	void								updateBox();

	std::function<void(const bool)>		mCheckBoxUpdatedCallback;
	std::function<void()>				mVisualUpdateCallback;

	ds::ui::Border*						mOuterBox;
	ds::ui::Sprite*						mInnerBox;
	ds::ui::Text*						mLabel;
	std::wstring						mTrueText;
	std::wstring						mFalseText;

	float								mLabelPadding;
	float								mTouchPadding;
	float								mBoxPad;
	bool								mIsChecked;

};

} // namespace ui
} // namespace ds
#endif