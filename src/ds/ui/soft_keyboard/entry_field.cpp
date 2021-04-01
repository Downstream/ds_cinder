#include "stdafx.h"

#include "entry_field.h"

#include <ds/ui/sprite/sprite_engine.h>
#include <ds/app/engine/engine_cfg.h>
#include <ds/ui/sprite/image.h>
#include <ds/app/environment.h>
#include <ds/debug/logger.h>

#include "ds/ui/soft_keyboard/soft_keyboard_button.h"

namespace ds {
namespace ui {

EntryField::EntryField(ds::ui::SpriteEngine& engine, EntryFieldSettings& settings)
	: IEntryField(engine)
	, mCursor(nullptr)
	, mTextSprite(nullptr)
	, mInFocus(false)
	, mAutoRegisterOnFocus(true)
	, mCursorIndex(0)
{
	mTextSprite = new ds::ui::Text(engine);
	mTextSprite->setAllowMarkup(false);
	addChildPtr(mTextSprite);

	mCursor = new ds::ui::Sprite(engine);
	mCursor->setTransparent(false);
	addChildPtr(mCursor);

	// In case you wanna see the size of this sucka
	//setTransparent(false);
	//setColor(ci::Color(0.3f, 0.0f, 0.0f));

	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	setProcessTouchCallback([this](ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti) {
		handleTouchInput(bs, ti);
	});

	setEntryFieldSettings(settings);
}

EntryField::~EntryField() {
	unfocus();
}

void EntryField::setEntryFieldSettings(EntryFieldSettings& newSettings) {
	mEntryFieldSettings = newSettings;

	if(mTextSprite) {
		mTextSprite->setResizeLimit(mEntryFieldSettings.mFieldSize.x, mEntryFieldSettings.mFieldSize.y);
		mTextSprite->setTextStyle(newSettings.mTextConfig);
		mTextSprite->setPosition(mEntryFieldSettings.mTextOffset);
	}

	setSize(mEntryFieldSettings.mFieldSize.x, mEntryFieldSettings.mFieldSize.y);

	if(mCursor) {
		mCursor->setColor(newSettings.mCursorColor);
		mCursor->setSize(newSettings.mCursorSize);
	}

	textUpdated();
}


ds::ui::EntryFieldSettings EntryField::getEntryFieldSettings() {
	return mEntryFieldSettings;
}

void EntryField::onSizeChanged() {
	if (mTextSprite) {
		if (mEntryFieldSettings.mAutoExpand) {
			mTextSprite->setResizeLimit(getWidth(), 0.0f);
			auto newTextHeight = mTextSprite->getHeight();
			if(newTextHeight != getHeight()){
				setSize(getWidth(), newTextHeight);
			}

		} else if (mEntryFieldSettings.mAutoResize) {
			mTextSprite->setResizeLimit(getWidth(), getHeight());
		}
	}
}

const std::wstring EntryField::getCurrentText() {
	return mCurrentText;
}

void EntryField::setCurrentText(const std::wstring& crTxStr) {
	mCurrentText = crTxStr;

	applyText(crTxStr);

	mCursorIndex = crTxStr.size();

	textUpdated();
}

void EntryField::applyText(const std::wstring& theStr) {
	if(mTextSprite) {
		if(mEntryFieldSettings.mPasswordMode) {
			std::wstring bullets;
			for(int i = 0; i < theStr.size(); i++) {
				bullets.append(L"*");
			}
			mTextSprite->setText(bullets);
		} else {
			mTextSprite->setText(theStr);
		}
	}
}

void EntryField::keyPressed(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType) {
	std::wstring currentCharacter = keyCharacter;
	std::wstring currentFullText = getCurrentText();

	if(mEntryFieldSettings.mSearchMode && keyType == SoftKeyboardDefs::kEnter) {
		textUpdated();
		if(mKeyPressedFunction) {
			mKeyPressedFunction(keyCharacter, keyType);
		}
		return;
	}

	/// Arrow keys
	if(keyType == SoftKeyboardDefs::kArrow) {
		if(keyCharacter == L"<" || keyCharacter == L"^") {
			mCursorIndex--;
		} else if(keyCharacter == L">" || keyCharacter == L"v") {
			mCursorIndex++;
		}
		cursorUpdated();

		/// Ignore function keys (F1 - F12) 
	} else if(keyType == SoftKeyboardDefs::kFunction) {
	} else if(keyType == SoftKeyboardDefs::kSpecial) {
		if(keyCharacter == L"Home" || keyCharacter == L"PgUp") {
			mCursorIndex = 0;
			cursorUpdated();
		} else if(keyCharacter == L"End" || keyCharacter == L"PgDn") {
			mCursorIndex = currentFullText.size();
			cursorUpdated();
		}


		/// If the cursor is at the end, add text to the end
	} else if(mCursorIndex == currentFullText.size()) {

		handleKeyPressGeneric(keyType, currentCharacter, currentFullText);

		setCurrentText(currentFullText);


		/// If End or PgDn was clicked, move the cursor to the end

		/// The cursor is not at the end, handle 'inserts'
	} else {


		std::wstring preString = currentFullText.substr(0, mCursorIndex);
		std::wstring posString = currentFullText.substr(mCursorIndex);

		/// delete forwards
		if(keyType == ds::ui::SoftKeyboardDefs::kFwdDelete) {
			if(!posString.empty()) {
				posString = posString.substr(1);
			}

			/// insert normal text or backspace
		} else {
			handleKeyPressGeneric(keyType, currentCharacter, preString);
		}

		std::wstringstream wss;
		wss << preString << posString;

		mCurrentText = wss.str();
		applyText(wss.str());

		if(keyType == ds::ui::SoftKeyboardDefs::kDelete) {
			if(!mCurrentText.empty()) {
				mCursorIndex--;
			}

			// cursor index stays the same
		} else if(keyType == SoftKeyboardDefs::kShift
				  || keyType == SoftKeyboardDefs::kFwdDelete
				  ) {
			// nothin!
		} else {
			mCursorIndex++;
		}

		textUpdated();
	}

	if(mKeyPressedFunction) {
		mKeyPressedFunction(keyCharacter, keyType);
	}
}

void EntryField::keyPressed(ci::app::KeyEvent& keyEvent) {
	bool handled = false;
	if(mNativeKeyCallback) {
		handled = mNativeKeyCallback(keyEvent);
	}

	if(handled) {
		return;
	}

	std::stringstream ss;
	ss << keyEvent.getChar();
	std::wstring keyCharacter = ds::wstr_from_utf8(ss.str());

	// TODO: differentiate delete forwards and delete back
	if(keyEvent.getCode() == ci::app::KeyEvent::KEY_BACKSPACE || keyEvent.getCode() == ci::app::KeyEvent::KEY_DELETE) {
		keyPressed(keyCharacter, ds::ui::SoftKeyboardDefs::kDelete);

		// TODO: handle up / down keys for lines and page up / down for pages
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_RIGHT || keyEvent.getCode() == ci::app::KeyEvent::KEY_DOWN) {
		setCursorIndex(mCursorIndex + 1);
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_LEFT || keyEvent.getCode() == ci::app::KeyEvent::KEY_UP) {
		setCursorIndex(mCursorIndex - 1);
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_RSHIFT || keyEvent.getCode() == ci::app::KeyEvent::KEY_LSHIFT) {
		// we're just gonna ignore these guys
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_HOME || keyEvent.getCode() == ci::app::KeyEvent::KEY_PAGEUP) {
		setCursorIndex(0);
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_END || keyEvent.getCode() == ci::app::KeyEvent::KEY_PAGEDOWN) {
		setCursorIndex(getCurrentText().size());
	} else if(keyEvent.getCode() == ci::app::KeyEvent::KEY_RETURN){
		keyPressed(keyCharacter, ds::ui::SoftKeyboardDefs::kEnter);
	} else {
		keyPressed(keyCharacter, ds::ui::SoftKeyboardDefs::kLetter);
	}
}

void EntryField::setNativeKeyboardCallback(std::function<bool(ci::app::KeyEvent& keyEvent)> func) {
	mNativeKeyCallback = func;
}

void EntryField::setKeyPressedCallback(std::function<void(const std::wstring& keyCharacter, const ds::ui::SoftKeyboardDefs::KeyType keyType)> keyPressedFunc) {
	mKeyPressedFunction = keyPressedFunc;
}

void EntryField::setTextUpdatedCallback(std::function<void(const std::wstring& fullStr)> func) {
	mTextUpdateFunction = func;
}

void EntryField::setCursorIndex(const size_t index) {
	mCursorIndex = index;
	if(mCursorIndex < 0) mCursorIndex = 0;
	if(mCursorIndex > getCurrentText().size()) mCursorIndex = getCurrentText().size();

	cursorUpdated();
}

void EntryField::setCursorPosition(const ci::vec3& globalPos) {
	ci::vec3 loccy = mTextSprite->globalToLocal(globalPos);
	mCursorIndex = mTextSprite->getCharacterIndexForPosition(ci::vec2(loccy));

	cursorUpdated();
}

const ci::vec3 EntryField::getCursorPosition() {
	if(mCursor){
		return mCursor->getGlobalPosition();
	}

	return ci::vec3();
}

void EntryField::resetCurrentText() {
	setCurrentText(L"");
	if(mKeyPressedFunction) {
		mKeyPressedFunction(L"", ds::ui::SoftKeyboardDefs::kDelete);
	}
}

void EntryField::pasteText(const std::wstring& insertText) {
	if(insertText.empty()) return;
	int postCursorIndex = static_cast<int>(mCursorIndex + insertText.size());
	std::wstring preText = mCurrentText.substr(0, mCursorIndex);
	std::wstring postText = mCurrentText.substr(mCursorIndex);
	std::wstringstream wss;
	wss << preText << insertText << postText;
	setCurrentText(wss.str());
	setCursorIndex(postCursorIndex);
}

void EntryField::focus() {
	if(mAutoRegisterOnFocus) {
		mEngine.registerEntryField(this);
	}

	if(mInFocus) return;
	mInFocus = true;

	enable(true);

	if(mCursor) {
		mCursor->show();
		mCursor->setOpacity(0.0f);
		blinkCursor();
	}

	onFocus();
}

void EntryField::unfocus() {
	if(mEngine.getRegisteredEntryField() == this) {
		mEngine.registerEntryField(nullptr);
	}
	if(!mInFocus) return;
	mInFocus = false;

	enable(false);

	if(mCursor) {
		mCursor->tweenOpacity(0.0f, mEntryFieldSettings.mAnimationRate, 0.0f, ci::easeNone, [this] {
			mCursor->hide();
		});
	}
}

void EntryField::autoRegisterOnFocus(const bool doAutoRegister) {
	mAutoRegisterOnFocus = doAutoRegister;
}

void EntryField::textUpdated() {
	if (mEntryFieldSettings.mAutoExpand) {
		auto newTextHeight = mTextSprite->getHeight();
		if (newTextHeight != getHeight()) {
			setSize(getWidth(), newTextHeight);
		}
	}

	cursorUpdated();
	onTextUpdated();

	if(mTextUpdateFunction) {
		mTextUpdateFunction(mCurrentText);
	}
}

void EntryField::cursorUpdated() {
	if(mTextSprite && mCursor) {
		if(mCursorIndex < 0) mCursorIndex = 0;
		if(mCursorIndex > getCurrentText().size()) {
			mCursorIndex = getCurrentText().size();
		}
		ci::vec2 cursorPos = mTextSprite->getPositionForCharacterIndex(static_cast<int>(mCursorIndex));
		mCursor->setPosition(cursorPos.x + mEntryFieldSettings.mCursorOffset.x, cursorPos.y + mEntryFieldSettings.mCursorOffset.y);
	}
}

void EntryField::handleTouchInput(ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti) {
	if(ti.mPhase != ds::ui::TouchInfo::Removed && mTextSprite) {
		setCursorPosition(ti.mCurrentGlobalPoint);
	}
}

void EntryField::blinkCursor() {
	if(!mCursor) return;
	mCursor->tweenOpacity(1.0f, mEntryFieldSettings.mAnimationRate, 0.0f, ci::easeNone, [this] {
		mCursor->tweenOpacity(1.0f, mEntryFieldSettings.mBlinkRate, 0.0f, ci::easeNone, [this] {
			mCursor->tweenOpacity(0.0f, mEntryFieldSettings.mAnimationRate, 0.0f, ci::easeNone, [this] {
				mCursor->tweenOpacity(0.0f, mEntryFieldSettings.mBlinkRate, 0.0f, ci::easeNone, [this] {
					blinkCursor();
				});
			});
		});
	});

}
}
}
