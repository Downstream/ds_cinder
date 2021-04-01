#include "stdafx.h"

#include "soft_keyboard_button.h"

#include <ds/app/engine/engine_cfg.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/sprite/image.h>
#include <ds/ui/sprite/border.h>
#include <ds/ui/sprite/circle_border.h>
#include <ds/ui/sprite/circle.h>

#include <ds/app/environment.h>
#include <ds/debug/logger.h>

namespace ds {
namespace ui {
SoftKeyboardButton::SoftKeyboardButton(ds::ui::SpriteEngine& engine, const std::wstring& characterlow, const std::wstring& characterUp, const SoftKeyboardDefs::KeyType keyType, SoftKeyboardSettings& settings)
	: ds::ui::Sprite(engine)
	, mCharacterLower(characterlow)
	, mCharacterUpper(characterUp)
	, mButtonBehaviour(*this)
	, mAnimDuration(0.0f)
	, mUpper(false)
	, mKeyType(keyType)
	, mText(nullptr)
	, mPressed(false)
	, mGraphic(nullptr)
	, mUpImg(nullptr)
	, mDownImg(nullptr)
{
	mLayoutFixedAspect = true;
	mText = new ds::ui::Text(mEngine);
	addChildPtr(mText);

	setCenter(0.5f, 0.5f);


	mButtonBehaviour.setOnClickFn([this]() {onClicked(); });
	// Purely for visual state
	mButtonBehaviour.setOnDownFn([this](const ds::ui::TouchInfo&) {showDown(); });
	mButtonBehaviour.setOnEnterFn([this]() {showDown(); });
	mButtonBehaviour.setOnExitFn([this]() {showUp(); });
	mButtonBehaviour.setOnUpFn([this]() {showUp(); });

	setSoftKeyboardSettings(settings);
}

const std::wstring& SoftKeyboardButton::getCharacter() {
	if(mUpper) return mCharacterUpper;
	else return mCharacterLower;
}

const SoftKeyboardDefs::KeyType& SoftKeyboardButton::getKeyType() {
	return mKeyType;
}

void SoftKeyboardButton::setSoftKeyboardSettings(SoftKeyboardSettings& softKeySettings) {
	if(mText){
		mTextConfigUp = softKeySettings.mKeyUpTextConfig;
		mTextConfigDown = softKeySettings.mKeyDnTextConfig;
		mTextOffset = softKeySettings.mKeyTextOffset;
	}

	mKeyUpColor = softKeySettings.mKeyUpColor;
	mKeyDnColor = softKeySettings.mKeyDownColor;

	if(mGraphic) {
		mGraphic->release();
		mGraphic = nullptr;
	}

	if(mUpImg) {
		mUpImg->release();
		mUpImg = nullptr;
	}

	if(mDownImg) {
		mDownImg->release();
		mDownImg = nullptr;
	}

	if(softKeySettings.mGraphicKeys) {

		// circular keys look bad when stretched (if they stretch at all)
		bool hideStretchedGraphics = false;

		if(softKeySettings.mGraphicType == SoftKeyboardSettings::kBorder) {
			mGraphic = new ds::ui::Border(mEngine, softKeySettings.mGraphicBorderWidth);
		} else if(softKeySettings.mGraphicType == SoftKeyboardSettings::kSolid) {
			mGraphic = new ds::ui::Sprite(mEngine);
			mGraphic->setTransparent(false);
		} else if(softKeySettings.mGraphicType == SoftKeyboardSettings::kCircularBorder) {
			hideStretchedGraphics = true;
			mGraphic = new ds::ui::CircleBorder(mEngine, softKeySettings.mGraphicBorderWidth);
		} else if(softKeySettings.mGraphicType == SoftKeyboardSettings::kCircularSolid) {
			hideStretchedGraphics = true;
			mGraphic = new ds::ui::Circle(mEngine, true, softKeySettings.mGraphicKeySize / 2.0f);
		}

		addChildPtr(mGraphic);
		mGraphic->sendToBack();
		mGraphic->setCornerRadius(softKeySettings.mGraphicRoundedCornerRadius);


		ci::vec2 keySize = ci::vec2(softKeySettings.mGraphicKeySize, softKeySettings.mGraphicKeySize);

		if(mKeyType == SoftKeyboardDefs::kNumber || mKeyType == SoftKeyboardDefs::kFunction || mKeyType == SoftKeyboardDefs::kArrow || mKeyType == SoftKeyboardDefs::kEscape) {
		} else if(mKeyType == SoftKeyboardDefs::kLetter) {
		} else if (mKeyType == SoftKeyboardDefs::kSpace) {
			keySize.x = 5.0f * keySize.x + 4.0f * softKeySettings.mKeyTouchPadding;
		} else if(mKeyType == SoftKeyboardDefs::kDelete || mKeyType == SoftKeyboardDefs::kFwdDelete) {
			if(mKeyType == SoftKeyboardDefs::kDelete && mCharacterLower.empty() && mCharacterUpper.empty()) {
				mCharacterLower = L"delete";
				mCharacterUpper = L"DELETE";
			}
			
			keySize.x = 2.5f * keySize.x + 3.0f * softKeySettings.mKeyTouchPadding;
		} else if (mKeyType == SoftKeyboardDefs::kShift) {
			keySize.x = (5.5f * keySize.x + 7.0f * softKeySettings.mKeyTouchPadding) / 2.0f;
		} else if(mKeyType == SoftKeyboardDefs::kTab || mKeyType == SoftKeyboardDefs::kDotCom || mKeyType == SoftKeyboardDefs::kSpecial) {
			keySize.x = 1.5f * keySize.x + softKeySettings.mKeyTouchPadding;
		} else if (mKeyType == SoftKeyboardDefs::kEnter) {
			keySize.x = (4.5f * keySize.x + 5.0f * softKeySettings.mKeyTouchPadding) / 2.0f;
		} else {
			DS_LOG_WARNING("Warning: key type not supported in SoftKeyboardButton");
		}

		if(hideStretchedGraphics && keySize.x != keySize.y) {
			mGraphic->hide();
		}
		mGraphic->setSize(keySize);
		mGraphic->setPosition(softKeySettings.mKeyTouchPadding, softKeySettings.mKeyTouchPadding);
		setSize(keySize.x + softKeySettings.mKeyTouchPadding * 2.0f, keySize.y + softKeySettings.mKeyTouchPadding * 2.0f);


	} else {


		std::string upImgPath = "";
		std::string dnImgPath = "";
		if(mKeyType == SoftKeyboardDefs::kNumber || mKeyType == SoftKeyboardDefs::kFunction || mKeyType == SoftKeyboardDefs::kArrow || mKeyType == SoftKeyboardDefs::kEscape) {
			upImgPath = softKeySettings.mKeyNumberUpImage;
			dnImgPath = softKeySettings.mKeyNumberDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kLetter) {
			upImgPath = softKeySettings.mKeyLetterUpImage;
			dnImgPath = softKeySettings.mKeyLetterDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kSpace) {
			upImgPath = softKeySettings.mKeySpaceUpImage;
			dnImgPath = softKeySettings.mKeySpaceDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kDelete || mKeyType == SoftKeyboardDefs::kFwdDelete) {
			upImgPath = softKeySettings.mKeyDeleteUpImage;
			dnImgPath = softKeySettings.mKeyDeleteDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kShift) {
			upImgPath = softKeySettings.mKeyShiftUpImage;
			dnImgPath = softKeySettings.mKeyShiftDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kTab || mKeyType == SoftKeyboardDefs::kDotCom || mKeyType == SoftKeyboardDefs::kSpecial) {
			upImgPath = softKeySettings.mKeyTabUpImage;
			dnImgPath = softKeySettings.mKeyTabDnImage;
		} else if(mKeyType == SoftKeyboardDefs::kEnter) {
			upImgPath = softKeySettings.mKeyEnterUpImage;
			dnImgPath = softKeySettings.mKeyEnterDnImage;
		} else {
			DS_LOG_WARNING("Warning: key type not supported in SoftKeyboardButton");
		}

		mUpImg = new ds::ui::Image(mEngine, upImgPath, ds::ui::Image::IMG_CACHE_F);
		mDownImg = new ds::ui::Image(mEngine, dnImgPath, ds::ui::Image::IMG_CACHE_F);
		addChildPtr(mUpImg);
		addChildPtr(mDownImg);

		mUpImg->setColor(softKeySettings.mKeyUpColor);
		mDownImg->setColor(softKeySettings.mKeyDownColor);

		mDownImg->setPosition(floorf(softKeySettings.mKeyTouchPadding), floorf(softKeySettings.mKeyTouchPadding));
		mUpImg->setPosition(mDownImg->getPosition());
		setSize(floorf(mDownImg->getWidth() + softKeySettings.mKeyTouchPadding * 2.0f), floorf(mDownImg->getHeight() + softKeySettings.mKeyTouchPadding * 2.0f));
	}

	doLayout();
}

void SoftKeyboardButton::setClickFn(const std::function<void(void)>& fn) {
	mClickFn = fn;
}

void SoftKeyboardButton::setShifted(const bool upper) {
	mUpper = upper;

	doLayout();
}

void SoftKeyboardButton::onClicked() {
	showUp();
	if(mClickFn) mClickFn();
}

void SoftKeyboardButton::showDown() {
	mPressed = true;
	doLayout();

	if(mStateChangeFunction) {
		mStateChangeFunction(true);
	}
}

void SoftKeyboardButton::showUp() {
	mPressed = false;
	doLayout();
	if(mStateChangeFunction) {
		mStateChangeFunction(false);
	}
}

void SoftKeyboardButton::doLayout(){

	if(mUpImg && mDownImg) {
		if(mPressed) {
			if(mAnimDuration <= 0.0f) {
				mUpImg->hide();
				mUpImg->setOpacity(0.0f);
				mDownImg->show();
				mDownImg->setOpacity(1.0f);
			} else {
				mUpImg->tweenOpacity(0.0f, mAnimDuration, 0.0f, ci::EaseInCubic(), [this]() {mUpImg->hide(); });
				mDownImg->show();
				mDownImg->tweenOpacity(1.0f, mAnimDuration, 0.0f, ci::EaseOutCubic());
			}
		} else {
			if(mAnimDuration <= 0.0f) {
				mUpImg->show();
				mUpImg->setOpacity(1.0f);
				mDownImg->hide();
				mDownImg->setOpacity(0.0f);
			} else {
				mUpImg->show();
				mUpImg->tweenOpacity(1.0f, mAnimDuration, 0.0f, ci::EaseOutCubic());
				mDownImg->tweenOpacity(0.0f, mAnimDuration, 0.0f, ci::EaseInCubic(), [this]() {mDownImg->hide(); });
			}

		}

		if(mKeyType == SoftKeyboardDefs::kFwdDelete) {
			mUpImg->setRotation(180.0f);
			mUpImg->setPosition(mUpImg->getWidth(), mUpImg->getHeight());
			mDownImg->setRotation(180.0f);
			mDownImg->setPosition(mUpImg->getWidth(), mUpImg->getHeight());
		}
	}


	if(mText){
		if(mPressed){
			if(!mTextConfigDown.empty()){
				mText->setTextStyle(mEngine.getTextStyle(mTextConfigDown));
			}
		} else {
			if (!mTextConfigUp.empty()) {
				mText->setTextStyle(mEngine.getTextStyle(mTextConfigUp));
			}
		}

		if(mUpper){
			mText->setText(mCharacterUpper);
		} else {
			mText->setText(mCharacterLower);
		}

		mText->setPosition(getWidth() / 2.0f - mText->getWidth() / 2.0f + mTextOffset.x, getHeight() / 2.0f - mText->getHeight() / 2.0f + mTextOffset.y);
	}

	if(mGraphic) {
		if(mPressed) {
			mGraphic->setColor(mKeyDnColor);
		} else {
			mGraphic->setColor(mKeyUpColor);
		}
	}

}


} // namespace ui
} // namespace ds