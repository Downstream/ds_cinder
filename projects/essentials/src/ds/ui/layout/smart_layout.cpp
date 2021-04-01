#include "stdafx.h"

#include "smart_layout.h"

#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/button/image_button.h>
#include <ds/ui/button/layout_button.h>
#include <ds/ui/button/sprite_button.h>
#include <ds/ui/interface_xml/interface_xml_importer.h>
#include <ds/ui/scroll/smart_scroll_list.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/sprite/text.h>
#include <ds/ui/util/text_model.h>
#include <ds/util/string_util.h>
#include <ds/util/file_meta_data.h>


namespace ds {
namespace ui {

SmartLayout::SmartLayout(ds::ui::SpriteEngine& engine, const std::string& xmlLayoutFile,
						 const std::string xmlFileLocation, const bool loadImmediately)
  : ds::ui::LayoutSprite(engine)
  , mLayoutFile(xmlFileLocation + xmlLayoutFile)
  , mNeedsLayout(false)
  , mInitialized(false)
  , mEventClient(engine) {
	if (loadImmediately) {
		initialize();
	}
}

SmartLayout::SmartLayout(ds::ui::SpriteEngine& engine)
  : ds::ui::LayoutSprite(engine)
  , mLayoutFile("")
  , mInitialized(false)
  , mNeedsLayout(false)
  , mEventClient(engine) {}

void SmartLayout::setLayoutFile(const std::string& xmlLayoutFile, const std::string xmlFileLocation,
								const bool loadImmediately) {
	mInitialized = false;
	mLayoutFile  = xmlFileLocation + xmlLayoutFile;

	if (loadImmediately) {
		initialize();
	}
}

void SmartLayout::initialize() {
	mSpriteMap.clear();
	clearChildren();
	ds::ui::XmlImporter::loadXMLto(this, ds::Environment::expand(mLayoutFile), mSpriteMap, nullptr, "", true);

	// Auto clear mNeedsLayout if client app runs layout manually
	setLayoutUpdatedFunction([this] { mNeedsLayout = false; });

	runLayout();
	mInitialized = true;
}

bool SmartLayout::hasSprite(const std::string& spriteName) { return mSpriteMap.find(spriteName) != mSpriteMap.end(); }

ds::ui::Sprite* SmartLayout::getSprite(const std::string& spriteName) {
	auto findy = mSpriteMap.find(spriteName);
	if (findy != mSpriteMap.end()) {
		return findy->second;
	}
	return nullptr;
}

void SmartLayout::setSpriteText(const std::string& spriteName, const std::string& theText) {
	ds::ui::Text* spr = getSprite<ds::ui::Text>(spriteName);

	if (spr) {
		spr->setText(theText);
		mNeedsLayout = true;
	} else {
		DS_LOG_VERBOSE(2, "Failed to set Text for Sprite: " << spriteName);
	}
}

void SmartLayout::setSpriteText(const std::string& spriteName, const std::wstring& theText) {
	return setSpriteText(spriteName, ds::utf8_from_wstr(theText));
}

void SmartLayout::setSpriteFont(const std::string& spriteName, const std::string& textCfgName) {
	ds::ui::Text* spr = getSprite<ds::ui::Text>(spriteName);

	if (spr) {
		spr->setTextStyle(textCfgName);
		mNeedsLayout = true;
	} else {
		DS_LOG_WARNING("Failed to set Font " << textCfgName << " for Sprite: " << spriteName);
	}
}

void SmartLayout::setSpriteImage(const std::string& spriteName, const std::string& imagePath, bool cache, bool skipMetaData) {
	
		int flags = 0;
		if (cache) flags = ds::ui::Image::IMG_CACHE_F;
		if (skipMetaData) flags = flags | ds::ui::Image::IMG_SKIP_METADATA_F;

		setSpriteImage(spriteName, imagePath, flags);
}

void SmartLayout::setSpriteImage(const std::string& spriteName, const std::string& imagePath, int flags) {
	ds::ui::Image* sprI = getSprite<ds::ui::Image>(spriteName);

	if (sprI) {
		
		//bool cache =  flags & ds::ui::Image::IMG_CACHE_F;
		bool skipMetaData =  flags & ds::ui::Image::IMG_SKIP_METADATA_F;
		//bool mipmap = flags & ds::ui::Image::IMG_ENABLE_MIPMAP_F;
		//bool preload = flags & ds::ui::Image::IMG_PRELOAD_F;
		
		sprI->setImageFile(imagePath, flags);

		if (skipMetaData) {
			sprI->setStatusCallback([this](const ds::ui::Image::Status& status) {
				mNeedsLayout = true;
			});
		}
		else {
			mNeedsLayout = true;
		}

	}
	else {
		DS_LOG_VERBOSE(2, "Failed to set Image for Sprite: " << spriteName);
	}
}

void SmartLayout::setSpriteImage(const std::string& spriteName, ds::Resource imageResource, bool cache, bool skipMetaData) {
	
		int flags = 0;
		if (cache) flags = ds::ui::Image::IMG_CACHE_F;
		if (skipMetaData) flags = flags | ds::ui::Image::IMG_SKIP_METADATA_F;

		setSpriteImage(spriteName, imageResource, flags);
}

void SmartLayout::setSpriteImage(const std::string& spriteName, ds::Resource imageResource, int flags) {
	ds::ui::Image* sprI = getSprite<ds::ui::Image>(spriteName);

	if (sprI) {
		//bool cache = flags & ds::ui::Image::IMG_CACHE_F;
		bool skipMetaData = flags & ds::ui::Image::IMG_SKIP_METADATA_F;
		//bool mipmap = flags & ds::ui::Image::IMG_ENABLE_MIPMAP_F;
		//bool preload = flags & ds::ui::Image::IMG_PRELOAD_F;

		sprI->setImageResource(imageResource, flags);

		if (skipMetaData) {
			sprI->setStatusCallback([this](const ds::ui::Image::Status& status) {
				mNeedsLayout = true;
			});
		}
		else {
			mNeedsLayout = true;
		}
	}
	else {
		DS_LOG_VERBOSE(2, "Failed to set Image for Sprite: " << spriteName);
	}
}

void SmartLayout::setSpriteTapFn(const std::string&											  spriteName,
								 const std::function<void(ds::ui::Sprite*, const ci::vec3&)>& tapCallback) {
	ds::ui::Sprite* spr = getSprite(spriteName);
	if (spr && tapCallback) {
		spr->enable(true);
		spr->setTapCallback(tapCallback);
	}
}

void SmartLayout::setSpriteClickFn(const std::string& spriteName, const std::function<void()>& clickCallback) {
	if (!clickCallback) return;
	auto ib = getSprite<ds::ui::ImageButton>(spriteName);
	if (ib) {
		ib->setClickFn(clickCallback);
		return;
	}
	auto lb = getSprite<ds::ui::LayoutButton>(spriteName);
	if (lb) {
		lb->setClickFn(clickCallback);
		return;
	}
	auto sb = getSprite<ds::ui::SpriteButton>(spriteName);
	if (sb) {
		sb->setClickFn(clickCallback);
		return;
	}
}

void SmartLayout::setContentModel(ds::model::ContentModelRef& theData) {
	mContentModel = theData;
	for (auto child : mSpriteMap) {
		// Handle model
		auto theModel = child.second->getUserData().getString("model");
		if (!theModel.empty()) {
			applyModelToSprite(child.second, child.first, theModel);
			continue;
		}

		// Handle each_model
		auto eachModel = child.second->getUserData().getString("each_model");
		if (!eachModel.empty()) {
			applyEachModelToSprite(child.second, eachModel);
			continue;
		}
	}

	runLayout();

	if (mContentUpdatedCallback) {
		mContentUpdatedCallback();
	}
}

void SmartLayout::applyModelToSprite(ds::ui::Sprite* child, const std::string& childName, const std::string& model) {
	if (!child) return;

	for (auto mit : ds::split(model, "; ", true)) {
		auto keyVals = ds::split(mit, ":", true);
		if (keyVals.size() == 2) {
			auto childProps = ds::split(keyVals[1], "->", true);

			if (childProps.empty() || childProps.size() > 2) {
				DS_LOG_WARNING("SmartLayout::setData() Invalid syntax for child / property mapping: " << model);
				continue;
			}

			// Get the right content model
			ds::model::ContentModelRef theNode = mContentModel;
			if (childProps[0] != "this") {
				theNode = mContentModel.getChildByName(childProps[0]);
			}


			if (childProps.size() == 1) {  // Handle model types that only require a model & not a property
				auto sprPropToSet = keyVals[0];
				if (sprPropToSet == "text_model") {
					auto fmt = child->getUserData().getString("model_format");

					std::string formattedModel = ds::ui::processTextModel(fmt, theNode);

					ds::ui::XmlImporter::setSpriteProperty(*child, "text", formattedModel);
				} else if(sprPropToSet == "visible_if_exists") {
					if(theNode.empty()) {
						child->hide();
					} else {
						child->show();
					}
				} else if(sprPropToSet == "hidden_if_exists") {
					if(theNode.empty()) {
						child->show();
					} else {
						child->hide();
					}
				}
			} else if (childProps.size() == 2) {  // Handle 'model->property' models
				auto		sprPropToSet = keyVals[0];
				auto		theProp		 = childProps[1];
				std::string actualValue  = "";
				
				if (sprPropToSet.rfind( "resource",0)==0) {

					int  flags = 0;
					if (sprPropToSet.find("_") != std::string::npos) {

						auto theFlags = ds::split(sprPropToSet, "_", true);
						for (auto val : theFlags) {

							if (val == "cache" || val == "c") {
								flags |= ds::ui::Image::IMG_CACHE_F;

							}
							else if (val == "mipmap" || val == "m") {
								flags |= ds::ui::Image::IMG_ENABLE_MIPMAP_F;

							}
							else if (val == "preload" || val == "p") {
								flags |= ds::ui::Image::IMG_PRELOAD_F;

							}
							else if (val == "skipmeta" || val == "s") {
								flags |= ds::ui::Image::IMG_SKIP_METADATA_F;

							}
							else if (val != "resource" ) {
								DS_LOG_WARNING("Trying to set unknown flags to src/filename attribute: _" << val
									<< "_ on sprite of type: " << typeid(child).name());
							}
						}
					}
					if(flags==0)
					{
						child->setResource(theNode.getProperty(theProp).getResource());
					}
					else {
						setSpriteImage(childName, theNode.getProperty(theProp).getResource(), flags);
					}
				} else if(sprPropToSet == "media_player_src") {
					auto theResource = theNode.getProperty(theProp).getResource();
					if(theResource.empty()) {
						theResource = ds::Resource(ds::Environment::expand(theNode.getPropertyString(theProp)));
					} else if(theResource.getType() == ds::Resource::IMAGE_TYPE) {
						ds::ImageMetaData metaData;
						metaData.add(ds::Environment::expand(theResource.getAbsoluteFilePath()),
									 ci::vec2(theResource.getWidth(), theResource.getHeight()));
					}

					if(!theResource.empty()) {
						child->setResource(theResource);
					}
				} else if(sprPropToSet == "visible_if_exists"){
					if(theNode.getPropertyString(theProp).empty()) {
						child->hide();
					} else {
						child->show();
					}
				} else if(sprPropToSet == "hidden_if_exists"){
					if(theNode.getPropertyString(theProp).empty()) {
						child->show();
					} else {
						child->hide();
					}
				} else if (sprPropToSet.rfind("_",0)==0) {
					auto click_data = theNode.getPropertyString(theProp);
					if (!click_data.empty()) {
						child->getUserData().setString(sprPropToSet, click_data);
					}
				} else {
					actualValue = theNode.getPropertyString(theProp);

					ds::ui::XmlImporter::setSpriteProperty(*child, sprPropToSet, actualValue);
				}
			}
		} else {
			DS_LOG_WARNING("SmartLayout::setData() Invalid syntax for prop / model mapping: " << model);
		}
	}
}

void SmartLayout::applyEachModelToSprite(ds::ui::Sprite* child, const std::string& eachModel) {
	if (!child) return;

	auto pairy = ds::split(eachModel, ":");
	if (pairy.size() == 2) {

		auto theNode = mContentModel;
		if (pairy[1] != "this") {
			theNode = mContentModel.getChildByName(pairy[1]);
		}

		if (auto smartScroller = dynamic_cast<ds::ui::SmartScrollList*>(child)) {
			smartScroller->setItemLayoutFile(pairy[0]);
			smartScroller->setContentList(theNode);
		} else {
			if (theNode.getChildren().empty()) return;

			child->clearChildren();

			int limit = child->getUserData().getInt("each_model_limit", 0, 0);
			if (limit == 0) limit = -1;

			for (auto baby : theNode.getChildren()) {
				if(limit-- == 0) break;

				auto babySprite = new ds::ui::SmartLayout(mEngine, pairy[0]);
				child->addChildPtr(babySprite);
				babySprite->setContentModel(baby);
				mNeedsLayout = true;
			}
		}
	}
}


void SmartLayout::tryAddChild(const std::string spriteName, std::function<ds::ui::Sprite*(void)> spriteGenerator) {
	ds::ui::Sprite* spr = getSprite(spriteName);
	if (spr && spriteGenerator) {
		spr->addChildPtr(spriteGenerator());
		mNeedsLayout = true;
	} else {
		DS_LOG_WARNING("Failed to add child to " << spriteName);
	}
}

void SmartLayout::addSpriteChild(const std::string spriteName, ds::ui::Sprite* newChild) {
	DS_LOG_WARNING("SmartLayout::addSpriteChild depricated. Use SmartLayout::tryAddChild instead.");
	ds::ui::Sprite* spr = getSprite(spriteName);
	if (spr && newChild) {
		spr->addChildPtr(newChild);
		mNeedsLayout = true;
	} else {
		DS_LOG_WARNING("Failed to add child to " << spriteName);
	}
}

void SmartLayout::onUpdateServer(const ds::UpdateParams& p) {
	if (mNeedsLayout) {
		runLayout();
		mNeedsLayout = false;
	}
}


}  // namespace ui
}  // namespace ds
