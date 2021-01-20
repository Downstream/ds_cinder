#include "stdafx.h"

#include "scroll_controller.h"

#include <ds/ui/sprite/sprite_engine.h>
#include <ds/app/engine/engine_events.h>
#include <ds/content/content_events.h>
#include <ds/ui/scroll/scroll_area.h>
#include <ds/ui/scroll/smart_scroll_list.h>

#include "events/app_events.h"


namespace downstream {

ScrollController::ScrollController(ds::ui::SpriteEngine& eng)
	: ds::ui::SmartLayout(eng, "scroll_controller.xml")
{


	auto smartScrollList = getSprite<ds::ui::SmartScrollList>("smart_scroll_list");
	if (smartScrollList) {
		smartScrollList->setContentItemTappedCallback([this](ds::ui::SmartLayout* item, ds::model::ContentModelRef model) {
			std::cout << "Item tapped id=" << model.getId() << " title=" << model.getPropertyString("title") << std::endl;
		});

		smartScrollList->setContentItemUpdatedCallback([this](ds::ui::SmartLayout* item){
			// do anything specific with the content here. 
			// For example: add fallback icons, set selected state, conditionally hide parts of the layout
		});

		smartScrollList->setStateChangeCallback([this](ds::ui::Sprite* bs, const bool highlighted) {
			auto item = dynamic_cast<ds::ui::SmartLayout*>(bs);
			if(item){
				auto theImage = item->getSprite("icon");
				auto theTitle = item->getSprite("item_title");
				if(theTitle && theImage){
					if(highlighted){
						theImage->setOpacity(0.5f);
						theTitle->setOpacity(0.5f);
					} else {
						theImage->setOpacity(1.0f);
						theTitle->setOpacity(1.0f);
					}
				}
			}
		});
	}

	listenToEvents<ds::ContentUpdatedEvent>([this](const ds::ContentUpdatedEvent& e) {
		setData();
	});

	setData();
}

void ScrollController::setData() {

	auto galleryScroll = getSprite<ds::ui::ScrollArea>("gallery_scroll");
	auto galleryLayout = getSprite<ds::ui::LayoutSprite>("gallery");

	if(galleryLayout && galleryScroll){
		galleryLayout->runLayout();
		galleryScroll->recalculateSizes();
	}

	auto sampleData = mEngine.mContent.getChildByName("sqlite.sample_data");
	auto smartScrollList = getSprite<ds::ui::SmartScrollList>("smart_scroll_list");
	if(smartScrollList){
		smartScrollList->setContentList(sampleData);
	}
}

} // namespace downstream

