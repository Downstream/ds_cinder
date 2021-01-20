#include "stdafx.h"

#include "control_check_box.h"

#include <ds/math/math_func.h>

namespace ds{
namespace ui{

ControlCheckBox::ControlCheckBox(ds::ui::SpriteEngine& engine)
	: Sprite(engine, 50.0f, 50.0f)
	, mInnerBox(nullptr)
	, mOuterBox(nullptr)
	, mLabel(nullptr)
	, mIsChecked(false)
	, mBoxPad(5.0f)
	, mTouchPadding(15.0f)
	, mLabelPadding(0.0f)
	, mTrueText(L"True")
	, mFalseText(L"False")
{

	// Set some defaults
	// You can change these by getting the nub and background sprites and changing them
	mOuterBox = new ds::ui::Border(mEngine);
	mOuterBox->mExportWithXml = false;
	mOuterBox->setColor(ci::Color(0.4f, 0.4f, 0.4f));
	mOuterBox->setBorderWidth(1.0f);
	addChildPtr(mOuterBox);

	mInnerBox = new ds::ui::Sprite(mEngine);
	mInnerBox->mExportWithXml = false;
	mInnerBox->setTransparent(false);
	mInnerBox->setColor(ci::Color(0.6f, 0.6f, 0.6f));
	addChildPtr(mInnerBox);

	enable(true);
	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	setTapCallback([this](ds::ui::Sprite* bs, const ci::vec3&){
		setCheckBoxValue(!mIsChecked);
	});

	layout();
	updateBox();
}


void ControlCheckBox::layout(){

	const float boxSize = getHeight() - mTouchPadding * 2.0f;
	if(mInnerBox && mOuterBox){
		mOuterBox->setSize(boxSize, boxSize);
		mOuterBox->setPosition(mTouchPadding, mTouchPadding);
		
		mInnerBox->setSize(boxSize - mBoxPad * 2.0f, boxSize - mBoxPad * 2.0f);
		mInnerBox->setPosition(mBoxPad + mTouchPadding, mBoxPad + mTouchPadding);
	}

	if(mLabel){
		mLabel->setPosition(getHeight() + mLabelPadding, getHeight() / 2.0f - mLabel->getHeight() / 2.0f);
	}
}

void ControlCheckBox::onSizeChanged(){
	layout();
}

void ControlCheckBox::setCheckboxUpdatedCallback(std::function<void(const bool)> func){
	mCheckBoxUpdatedCallback = func;
}

void ControlCheckBox::setCheckBoxValue(const bool checkValue){
	mIsChecked = checkValue;

	updateBox();

	if(mCheckBoxUpdatedCallback) mCheckBoxUpdatedCallback(mIsChecked);
}

void ControlCheckBox::updateBox(){
	if(mLabel){
		if(mIsChecked){
			mLabel->setText(mTrueText);
		} else {
			mLabel->setText(mFalseText);
		}
	}

	if(mInnerBox){
		if(mIsChecked){
			mInnerBox->show();
		} else {
			mInnerBox->hide();
		}
	}

	if(mVisualUpdateCallback){
		mVisualUpdateCallback();
	}

	if(mLabel){
		setSize(getHeight() + mLabel->getWidth() + mLabelPadding, getHeight());
	} else {
		layout();
	}
}

ds::ui::Sprite* ControlCheckBox::getInnerBoxSprite(){
	return mInnerBox;
}

ds::ui::Border* ControlCheckBox::getOuterBoxSprite(){
	return mOuterBox;
}


ds::ui::Text* ControlCheckBox::getLabelSprite() {
	return mLabel;
}

void ControlCheckBox::setTouchPadding(const float touchPadding){
	mTouchPadding = touchPadding;
	layout();
}

void ControlCheckBox::setBoxPadding(const float boxPadding){
	mBoxPad = boxPadding;
	layout();
}


void ControlCheckBox::setLabelTextStyle(const std::string& styleName){
	const ds::ui::TextStyle& theStyle = mEngine.getTextStyle(styleName);
	setLabelTextStyle(theStyle);
}

void ControlCheckBox::setLabelTextStyle(const std::string& fontName, const float& fontSize, const ci::ColorA& fontColor){
	ds::ui::TextStyle textStyle = ds::ui::TextStyle(fontName, fontName, fontSize, 1.0f, 0.0f, fontColor);
	setLabelTextStyle(textStyle);
}

void ControlCheckBox::setLabelTextStyle(const ds::ui::TextStyle& textConfig) {
	if (!mLabel) {
		mLabel = new ds::ui::Text(mEngine);
		addChildPtr(mLabel);
	}

	mLabel->setTextStyle(textConfig);
	updateBox();

}

void ControlCheckBox::setLabelPadding(const float labelPadding){
	mLabelPadding = labelPadding;
	updateBox();
}

void ControlCheckBox::setLabelLabels(const std::wstring& trueLabel, const std::wstring& falseText){
	mTrueText = trueLabel;
	mFalseText = falseText;
	updateBox();
}

void ControlCheckBox::setTrueLabel(const std::wstring& trueLabel){
	mTrueText = trueLabel;
	updateBox();
}

void ControlCheckBox::setFalseLabel(const std::wstring& falseText){
	mFalseText = falseText;
	updateBox();

}

} // namespace ui

} // namespace ds