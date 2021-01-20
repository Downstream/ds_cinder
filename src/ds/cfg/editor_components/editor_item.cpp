#include "stdafx.h"

#include "editor_item.h"

#include <ds/math/math_defs.h>
#include "ds/app/blob_reader.h"
#include "ds/app/blob_registry.h"

#include <cinder/TriMesh.h>
#include <cinder/Triangulate.h>

#include "ds/ui/sprite/circle.h"

namespace ds{
namespace cfg{

EditorItem::EditorItem(ds::ui::SpriteEngine& e)
	: ds::ui::LayoutSprite(e)
	, mSettingValue(nullptr)
	, mSettingName(nullptr)
	, mSettingComment(nullptr)
	, mIsHeader(false)
{
	setSize(600.0f, 200.0f);
	setShrinkToChildren(ds::ui::LayoutSprite::kShrinkHeight); 
	setLayoutType(ds::ui::LayoutSprite::kLayoutVFlow);

	mSettingName = new ds::ui::Text(mEngine);
	mSettingName->setFont("Arial Bold");
	mSettingName->setFontSize(10.0f);
	mSettingName->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
	addChildPtr(mSettingName);

	mSettingValue = new ds::ui::Text(mEngine);
	mSettingValue->setFont("Arial");
	mSettingValue->setFontSize(14.0f);
	mSettingValue->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
	addChildPtr(mSettingValue);

	mSettingComment = new ds::ui::Text(mEngine);
	mSettingComment->setFont("Arial");
	mSettingComment->setFontSize(12.0f);
	mSettingComment->setOpacity(0.4f);
	mSettingComment->mLayoutUserType = ds::ui::LayoutSprite::kFlexSize;
	addChildPtr(mSettingComment);
}


void EditorItem::setSetting(Settings::Setting* theSetting){
	mOriginalSettingName = "";

	if(!mSettingName || !mSettingValue || !theSetting) return;
	mOriginalSettingName = theSetting->mName;
	mHasOriginalValue = !theSetting->mOriginalValue.empty();
	if(mHasOriginalValue)
	{
	//	DS_LOG_INFO("HAS ORIGINAL VALUE");
	}
	if(theSetting->mType == ds::cfg::SETTING_TYPE_SECTION_HEADER){
		mSettingName->setFontSize(16.0f);
		mSettingName->mLayoutTPad = 10.0f;
		mSettingName->setColor(ci::Color(0.56f, 0.7f, 0.7f));
		mIsHeader = true;
	} else {
		mSettingName->setFontSize(9.0f);
		mSettingName->mLayoutTPad = 0.0f;
		mSettingName->setColor(ci::Color(0.9f, 0.282f, 0.035f));
		mIsHeader = false;
	}

	mSettingName->setText(theSetting->mName);
	
		
	if(theSetting->mRawValue.empty() && theSetting->mType != ds::cfg::SETTING_TYPE_SECTION_HEADER){
		mSettingValue->setText("<span style='italic'>empty</span>");
	} else {
		auto ss = std::stringstream();
		ss << theSetting->mRawValue;
		if(!theSetting->mOriginalValue.empty())
		{
			ss << " (" << theSetting->mOriginalValue << ")";
		}
		mSettingValue->setText(ss.str());
	}
	//mSettingComment->setText(theSetting->mComment);
}

const std::string& EditorItem::getSettingName(){
	return mOriginalSettingName;
}

}
}
