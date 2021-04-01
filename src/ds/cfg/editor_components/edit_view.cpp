#include "stdafx.h"

#include "edit_view.h"

#include <ds/math/math_defs.h>
#include "ds/app/blob_reader.h"
#include "ds/app/blob_registry.h"

#include <cinder/Clipboard.h>

#include <ds/ui/soft_keyboard/soft_keyboard_builder.h>

#include "ds/cfg/settings.h"

namespace ds{
namespace cfg{

EditView::EditView(ds::ui::SpriteEngine& e)
	: ds::ui::LayoutSprite(e)
	, mTheSetting(nullptr)
	, mSettingValue(nullptr)
	, mSettingName(nullptr)
	, mSettingComment(nullptr)
	, mSettingDefault(nullptr)
	, mSettingMin(nullptr)
	, mSettingMax(nullptr)
	, mSettingPossibles(nullptr)
	, mSettingSource(nullptr)
	, mCancelButton(nullptr)
	, mApplyButton(nullptr)
	, mSaveButton(nullptr)
	, mButtonHolder(nullptr)
	, mEntryEditor(nullptr)
	, mKeyboard(nullptr)
	, mCheckBox(nullptr)
	, mSlider(nullptr)
	, mPossibleIndex(0)
{
	setSize(600.0f, 200.0f);
	setShrinkToChildren(ds::ui::LayoutSprite::kShrinkHeight);
	setLayoutType(ds::ui::LayoutSprite::kLayoutVFlow);
	enable(true); // block touchies
	setScale(mEngine.getSrcRect().getWidth() / mEngine.getDstRect().getWidth());

	ds::ui::Sprite* backgroundSprite = new ds::ui::Sprite(mEngine);
	backgroundSprite->setColor(ci::Color::black());
	backgroundSprite->setOpacity(0.9f);
	backgroundSprite->setTransparent(false);
	backgroundSprite->setBlendMode(ds::ui::BlendMode::MULTIPLY);
	backgroundSprite->mLayoutUserType = ds::ui::LayoutSprite::kFillSize;
	addChildPtr(backgroundSprite);


	mSettingName = addTextSprite("Arial Bold", 24.0f, 1.0f, false);
	mSettingName->setColor(ci::Color(0.9f, 0.282f, 0.035f));

	mSettingValue = addTextSprite("Arial Bold", 21.0f, 1.0f, false);
	mSettingComment = addTextSprite("Arial", 12.0f, 0.75f, false);
	mSettingDefault = addTextSprite("Arial", 12.0f, 0.4f, true);

	mSettingMin = addTextSprite("Arial", 12.0f, 0.4f, true);
	mSettingMax = addTextSprite("Arial", 12.0f, 0.4f, true);
	mSettingPossibles = addTextSprite("Arial", 12.0f, 0.4f, false);
	mSettingSource = addTextSprite("Arial", 12.0f, 0.4f, false);

	mButtonHolder = new ds::ui::LayoutSprite(mEngine);
	mButtonHolder->setLayoutType(ds::ui::LayoutSprite::kLayoutHFlow);
	mButtonHolder->mLayoutHAlign = ds::ui::LayoutSprite::kRight;
	mButtonHolder->mLayoutBPad = 10.0f;
	mButtonHolder->mLayoutRPad = 10.0f;
	mButtonHolder->setShrinkToChildren(ds::ui::LayoutSprite::kShrinkBoth);
	mButtonHolder->setSpacing(10.0f);
	addChildPtr(mButtonHolder);

	mCancelButton = addTextSprite("Arial", 18.0f, 0.8f, true);
	mCancelButton->setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3& pos){
		stopEditing();
	});
	mCancelButton->setText("Close");
	mButtonHolder->addChildPtr(mCancelButton);

	mApplyButton = addTextSprite("Arial", 18.0f, 1.0f, true);
	mApplyButton->setColor(ci::Color(0.9f, 0.282f, 0.035f));
	mApplyButton->setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3& pos){
		applySetting(true);
	});
	mApplyButton->setText("Apply");
	mButtonHolder->addChildPtr(mApplyButton);

	mSaveButton = addTextSprite("Arial", 18.0f, 1.0f, true);
	mSaveButton->setColor(ci::Color(0.93f, 0.62f, 0.49f));
	mSaveButton->setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3& pos){
		applySetting(false);
	});
	mSaveButton->setText("Save");
	mButtonHolder->addChildPtr(mSaveButton);

}

void EditView::applySetting(const bool notify){
	if(mTheSetting && mEntryEditor){
		mTheSetting->mRawValue = ds::utf8_from_wstr(mEntryEditor->getCurrentText());
		mTheSetting->mOriginalValue.clear();
		
		mTheSetting->replaceSettingVariablesAndExpressions();
	}
	if(mSettingUpdatedCalback){
		mSettingUpdatedCalback(mTheSetting);
	}
	if(notify){
		mEngine.getNotifier().notify(ds::cfg::Settings::SettingsEditedEvent(mParentSettingsName, mTheSetting->mName));
	}
}

ds::ui::Text* EditView::addTextSprite(const std::string& fontName, const float fontSize, const float opacity, const bool clickSetValue){
	ds::ui::Text* newText = new ds::ui::Text(mEngine);
	newText->setFont(fontName);
	newText->setFontSize(fontSize);
	newText->setOpacity(opacity);
	newText->mLayoutTPad = 5.0f;
	newText->mLayoutLPad = 5.0f;
	newText->mLayoutRPad = 5.0f;
	newText->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
	if(clickSetValue){
		newText->enable(true);
		newText->enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
		newText->setTapCallback([this, newText](ds::ui::Sprite* bs, const ci::vec3& pos){
			std::wstring theText = newText->getText();
			auto splitty = ds::split(theText, L": ", true);
			if(mEntryEditor && splitty.size() > 1){
				mEntryEditor->setCurrentText(splitty[1]);
				updateValue(splitty[1]);
			}
		});
	}
	addChildPtr(newText);
	return newText;
}

void EditView::setSetting(Settings::Setting* theSetting, const std::string& parentSetingsName){
	mTheSetting = theSetting;
	mParentSettingsName = parentSetingsName;

	if(!mSettingName || !mSettingValue || !mTheSetting || !mSettingComment || !mSettingDefault || !mSettingMin || !mSettingMax || !mSettingSource || !mSettingPossibles) return;


	mSettingName->setText(theSetting->mName);
	//mSettingValue->setText(theSetting->mRawValue);	
	mSettingComment->setText(theSetting->mComment);
	mSettingDefault->setText("Default: " + theSetting->mDefault);
	mSettingMin->setText("Min: " + theSetting->mMinValue);
	mSettingMax->setText("Max: " + theSetting->mMaxValue);
	mSettingPossibles->setText("Possible Values: " + theSetting->mPossibleValues);
	mSettingSource->setText("Source: " + theSetting->mSource);

	if(!mSlider){
		mSlider = new ds::ui::ControlSlider(mEngine, false);
		mSlider->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
		mSlider->mLayoutLPad = 5.0f;
		mSlider->mLayoutRPad = 5.0f;
		mSlider->setSliderInterpolation(ds::ui::ControlSlider::kSliderTypeQuadratic);

		mSlider->setSliderUpdatedCallback([this](double sliderPercent, double sliderValue, bool finishedAdjusting){
			if(mTheSetting && mEntryEditor){
				if(mTheSetting->mType == ds::cfg::SETTING_TYPE_INT){
					int theActualValue = static_cast<int>(round(sliderValue));
					mEntryEditor->setCurrentText(std::to_wstring(theActualValue));
				} else if(mTheSetting->mType == ds::cfg::SETTING_TYPE_FLOAT){
					float theActualValue = static_cast<float>(sliderValue);
					mEntryEditor->setCurrentText(std::to_wstring(theActualValue));
				} else {
					mEntryEditor->setCurrentText(std::to_wstring(sliderValue));
				}
			}
		});
		addChildPtr(mSlider);
	}

	if(!mCheckBox){
		mCheckBox = new ds::ui::ControlCheckBox(mEngine);
		mCheckBox->mLayoutLPad = 5.0f;
		mCheckBox->mLayoutRPad = 5.0f;
		mCheckBox->setLabelTextStyle("Arial Bold", 18.0f, ci::ColorA(1.0f, 1.0f, 1.0f));
		mCheckBox->mLayoutUserType = ds::ui::LayoutSprite::kFixedSize;
		mCheckBox->setCheckboxUpdatedCallback([this](const bool isChecked){
			if(isChecked){
				mEntryEditor->setCurrentText(L"true");
			} else {
				mEntryEditor->setCurrentText(L"false");
			}
		});
		addChildPtr(mCheckBox);
	}

	if(!mEntryEditor){
		ds::ui::EntryFieldSettings efs;
		efs.mFieldSize = ci::vec2(600.0f, 40.0f);

		if(!mEngine.getEngineCfg().hasTextStyle("settings_editor:edit_view:entry:field")) {
			ds::ui::TextStyle textCfg;
			textCfg.mFont = "Arial";
			textCfg.mSize = 18.0f;
			textCfg.mColor = ci::Color::white();
			textCfg.mLeading = 1.0f;
			mEngine.getEngineCfg().setTextStyle("settings_editor:edit_view:entry:field", textCfg);
		}
		efs.mTextConfig = "settings_editor:edit_view:entry:field";
		efs.mCursorOffset.x = 0.0f;
		mEntryEditor = new ds::ui::EntryField(mEngine, efs);
		mEntryEditor->mLayoutTPad = 5.0f;
		mEntryEditor->mLayoutLPad = 5.0f;
		mEntryEditor->mLayoutRPad = 5.0f;
		mEntryEditor->mLayoutBPad = 0.0f;
		addChildPtr(mEntryEditor);
	}

	if(!mKeyboard){
		ds::ui::SoftKeyboardSettings sks;
		sks.mGraphicKeys = true;
		sks.mGraphicKeySize = 32.0f;
		sks.mKeyTouchPadding = 2.5f;
		sks.mGraphicType = ds::ui::SoftKeyboardSettings::kSolid;
		if(!mEngine.getEngineCfg().hasTextStyle("settings_editor:edit_view:keyboard:key_up")) {
			ds::ui::TextStyle textCfg;
			textCfg.mFont = "Arial";
			textCfg.mSize = 14.0f;
			textCfg.mColor = ci::Color::white();
			textCfg.mLeading = 1.0f;
			mEngine.getEngineCfg().setTextStyle("settings_editor:edit_view:keyboard:key_up", textCfg);
		}
		if(!mEngine.getEngineCfg().hasTextStyle("settings_editor:edit_view:keyboard:key_dn")) {
			ds::ui::TextStyle textCfg;
			textCfg.mFont = "Arial";
			textCfg.mSize = 14.0f;
			textCfg.mColor = ci::Color(0.5f, 0.5f, 0.5f);
			textCfg.mLeading = 1.0f;
			mEngine.getEngineCfg().setTextStyle("settings_editor:edit_view:keyboard:key_dn", textCfg);
		}
		sks.mKeyDnTextConfig = "settings_editor:edit_view:keyboard:key_dn";
		sks.mKeyUpTextConfig = "settings_editor:edit_view:keyboard:key_up";
		sks.mKeyUpColor = ci::Color(0.25f, 0.25f, 0.25f);
		sks.mKeyDownColor = ci::Color::white();
		sks.normalizeSettings();
		mKeyboard = ds::ui::SoftKeyboardBuilder::buildExtendedKeyboard(mEngine, sks, this);
		mKeyboard->mLayoutTPad = 0.0f;
		mKeyboard->mLayoutLPad = 5.0f;
		mKeyboard->mLayoutRPad = 5.0f;
		mKeyboard->mLayoutBPad = 10.0f;
	}

	if(mEntryEditor && mKeyboard && mSlider && mTheSetting){

		mKeyboard->setKeyPressFunction([this](const std::wstring& character, ds::ui::SoftKeyboardDefs::KeyType keyType){
			if(mEntryEditor){
				if(keyType == ds::ui::SoftKeyboardDefs::kEnter){
					stopEditing();
				} else {
					mEntryEditor->keyPressed(character, keyType);
				}
			}
		});

		mEntryEditor->setNativeKeyboardCallback([this](ci::app::KeyEvent& event)->bool{
			if(event.getCode() == ci::app::KeyEvent::KEY_ESCAPE){
				stopEditing();
				return true;
			}

			if(mNextSettingCallback &&
			   (event.getCode() == ci::app::KeyEvent::KEY_DOWN
			   || event.getCode() == ci::app::KeyEvent::KEY_TAB
			   )){
				applySetting(false);
				mNextSettingCallback(true);
				return true;
			}

			if(mNextSettingCallback &&
			   (event.getCode() == ci::app::KeyEvent::KEY_RETURN
			   || event.getCode() == ci::app::KeyEvent::KEY_KP_ENTER
			   )){
				applySetting(true);
				return true;
			}

			if(mNextSettingCallback &&
			   event.getCode() == ci::app::KeyEvent::KEY_UP){
				mNextSettingCallback(false);
				return true;
			}

			if(event.getCode() == ci::app::KeyEvent::KEY_v && event.isControlDown()){
				if(ci::Clipboard::hasString()){
					mEntryEditor->pasteText(ds::wstr_from_utf8(ci::Clipboard::getString()));
				}

				return true;
			}

			return false;

		});

		mEntryEditor->setTextUpdatedCallback([this](const std::wstring& line){
			updateValue(line);
			//mEngine.getNotifier().notify(Settings::SettingsEditedEvent("", ""));
		});
		if (mTheSetting->mOriginalValue.empty()) {
			mEntryEditor->setCurrentText(ds::wstr_from_utf8(mTheSetting->mRawValue));
		} else
		{
			mEntryEditor->setCurrentText(ds::wstr_from_utf8(mTheSetting->mOriginalValue));
		}
		mEntryEditor->show();
		mKeyboard->show();

		mPossibleValues = theSetting->getPossibleValues();
		mPossibleIndex = 0;

		mSlider->hide();
		mSlider->setScale(0.0f);

		mCheckBox->hide();
		mCheckBox->setScale(0.0f);

		if(theSetting->mType == ds::cfg::SETTING_TYPE_BOOL){
			mEntryEditor->setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3& pos){
				if(mEntryEditor->getCurrentText() == L"true"){
					mEntryEditor->setCurrentText(L"false");
				} else {
					mEntryEditor->setCurrentText(L"true");
				}
			});

			if(mEntryEditor->getCurrentText().find(L"t") != std::wstring::npos){
				mCheckBox->setCheckBoxValue(true);
			} else {
				mCheckBox->setCheckBoxValue(false);
			}

			mEntryEditor->hide();
			mKeyboard->hide();
			mCheckBox->show();
			mCheckBox->setScale(1.0f);

		} else if(mPossibleValues.size() > 1){
			for(int i = 0; i < mPossibleValues.size(); i++){
				if(theSetting->mRawValue == mPossibleValues[i]){
					mPossibleIndex = i;
					break;
				}
			}

			mEntryEditor->setTapCallback([this](ds::ui::Sprite*, const ci::vec3&){
				if(mPossibleValues.size() < 1) return;
				mPossibleIndex++;
				if(mPossibleIndex > mPossibleValues.size() - 1){
					mPossibleIndex = 0;
				}
				if(mPossibleIndex < 0) mPossibleIndex = 0;
				mEntryEditor->setCurrentText(ds::wstr_from_utf8(mPossibleValues[mPossibleIndex]));
			});

		} else if(  (theSetting->mType == ds::cfg::SETTING_TYPE_DOUBLE
				  || theSetting->mType == ds::cfg::SETTING_TYPE_FLOAT
				  || theSetting->mType == ds::cfg::SETTING_TYPE_INT)
				  && !theSetting->mMinValue.empty() && !theSetting->mMaxValue.empty()
				  ){
			mSlider->show();
			mSlider->setScale(1.0f);
			mSlider->setSliderLimits(ds::string_to_double(theSetting->mMinValue), ds::string_to_double(theSetting->mMaxValue));
			mSlider->setSliderValue(ds::wstring_to_double(mEntryEditor->getCurrentText()));
		} else {
			mEntryEditor->setTapCallback(nullptr);
		}

		mEntryEditor->autoRegisterOnFocus(true);
		mEntryEditor->focus();
	}

	if(mButtonHolder){
		mButtonHolder->sendToFront();
	}
	show();
	runLayout();

}

void EditView::updateValue(const std::wstring& theValue){
	if(mSettingValue){
		//	mSettingValue->setText(line);
	}

	runLayout();
}

void EditView::stopEditing(){

	hide();
	if(mEntryEditor){
		mEntryEditor->unfocus();
	}
}

}
}
