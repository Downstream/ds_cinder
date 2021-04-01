#pragma once

#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/text.h>
#include <ds/util/string_util.h>

#include "ds/ui/soft_keyboard/soft_keyboard_defs.h"

namespace ds {
namespace ui {

/**  EntryField
*		A text field with a blinking cursor for text entry.
*/

class EntryFieldSettings {
public:
	EntryFieldSettings()
		: mTextConfig("entry_field:text")
		, mCursorSize(2.0f, 36.0f)
		, mFieldSize(500.0f, 100.0f)
		, mCursorOffset(2.0f, -5.0f)
		, mCursorColor(1.0f, 1.0f, 1.0f)
		, mBlinkRate(0.5f)
		, mAnimationRate(0.3f)
		, mTextOffset(0.0f, 0.0f)
		, mPasswordMode(false)
		, mSearchMode(false)
		, mAutoResize(false)
		, mAutoExpand(false)
	{}

	std::string mTextConfig;
	ci::vec2	mTextOffset;
	ci::vec2	mFieldSize;
	ci::vec2	mCursorSize;
	ci::Color	mCursorColor;
	ci::vec2	mCursorOffset; 
	bool		mPasswordMode; // characters show as stars
	bool		mSearchMode; // enter key sends a text updated callback but doesn't insert text
	bool		mAutoResize; // if the entry field resizes, sets the size of the TextSprite resize limits as well, default off
	bool		mAutoExpand; // resizes the entry field to the height of the entered text. disables Auto Resize, default off
	float		mBlinkRate;
	float		mAnimationRate;
};

/// Abstract base class for a generic interface for registering on the Engine
/// Extend this class if you want to be able to register on the engine for normal keyboard input (not just soft keyboard input)
class IEntryField : public ds::ui::Sprite {
public:
	IEntryField(ds::ui::SpriteEngine& engine) : ds::ui::Sprite(engine){};
	~IEntryField() {
		if(mEngine.getRegisteredEntryField() == this) {
			mEngine.registerEntryField(nullptr);
		}
	}

	virtual void						keyPressed(ci::app::KeyEvent& keyEvent) = 0;
	virtual void						keyPressed(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType) = 0;
};

class EntryField : public IEntryField {
public:

	EntryField(ds::ui::SpriteEngine&, EntryFieldSettings& settings);
	~EntryField();

	/// This field will be in "focus", making the cursor blink and look like this is activated
	/// Does no checking about the number of fields in focus, and any number could be in focus at once (multiple user scenario)
	void								focus();

	/// This field will lose focus, which hides the cursor, and this field won't be active
	void								unfocus();

	/// If this field is in focus
	bool								getIsInFocus(){ return mInFocus; }

	/// If true, when you call "focus", it will register this field in the main engine class to grab keyboard input
	/// Default is true
	void								autoRegisterOnFocus(const bool doAutoRegister);

	/// Get the current entered text string
	const std::wstring					getCurrentText();
	const std::string					getCurrentTextString() { return ds::utf8_from_wstr(getCurrentText()); }

	/// In case you want to do more formatting, etc. Use with caution, do not release
	ds::ui::Text*						getTextSprite() { return mTextSprite; }

	/// Set the the current text string. Does not call any callbacks.
	void								setCurrentText(const std::wstring& curTxtStr);

	/// Clears the current text string. Does not call any callbacks.
	void								resetCurrentText();

	/// Inserts the string at the current cursor location
	void								pasteText(const std::wstring& insertText);

	/// Settings can be set at any time to update graphic properties
	void								setEntryFieldSettings(EntryFieldSettings& newSettings);
	EntryFieldSettings					getEntryFieldSettings();

	/// Handles key input like a keyboard would
	virtual void						keyPressed(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType) override;
	virtual void						keyPressed(ci::app::KeyEvent& keyEvent) override;

	/// This is called back before the key is handled internally, in case you want to intercept some keys.
	/// Return true if the key has been handled, return false to pass the key into the entry field. In most cases, it just types the letter.
	void								setNativeKeyboardCallback(std::function<bool(ci::app::KeyEvent& keyEvent)> func);

	/// Set a lambda function when the above keypressed function is called
	void								setKeyPressedCallback(std::function<void(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType)> keyPressedFunc);

	/// Set a lambda function that's called just after onTextUpdated()
	void								setTextUpdatedCallback(std::function<void(const std::wstring& fullStr)> func);

	/// Gets the index of the cursor position in the visible text string (might have weird results when using `<span>` tags in Text.
	size_t								getCursorIndex(){ return mCursorIndex; }
	/// Set the index of the cursor. Will bounds check the cursor
	void								setCursorIndex(const size_t index);

	/// Sets the position of the cursor based on the given global position
	void								setCursorPosition(const ci::vec3& globalPos);

	/// Gets the position of the cursor in global space
	const ci::vec3						getCursorPosition();

protected:

	/// Override to know when this field gains focus.
	virtual void						onFocus(){}

	/// Override to know when this field loses focus
	virtual void						onUnFocus(){}

	virtual void						onSizeChanged() override;

	/// Any time the text has changed, do not override
	void								textUpdated();

	/// The cursor position has been changed, do not override. Also called on text updated
	void								cursorUpdated();

	/// Override this to know when any text has changed
	virtual void						onTextUpdated(){}

	/// A series of tweens that fades the cursor on, waits, then fades it off
	void								blinkCursor();

	void								handleTouchInput(ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti);

	/// Sets the text sprite's text (bullets for pswd, normal text for normal text)
	void								applyText(const std::wstring& theStr);

	bool								mInFocus;
	bool								mAutoRegisterOnFocus;
	size_t								mCursorIndex;

	std::wstring						mCurrentText;
	ds::ui::Text*						mTextSprite;
	ds::ui::Sprite*						mCursor;
	std::vector<ds::ui::Sprite*>		mSelectionIndicators;
	EntryFieldSettings					mEntryFieldSettings;
	std::function<bool(ci::app::KeyEvent&)> mNativeKeyCallback;
	std::function<void(const std::wstring&, const ds::ui::SoftKeyboardDefs::KeyType)> mKeyPressedFunction;
	std::function<void(const std::wstring&)> mTextUpdateFunction;

};

} // namespace ui
} // namespace ds

