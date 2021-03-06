#include "scrollexample_app.h"

#include <Poco/String.h>
#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/app/engine/engine.h>
#include <ds/ui/sprite/text.h>
#include <ds/ui/sprite/shader/sprite_shader.h>

#include "app/app_defs.h"
#include "app/globals.h"
#include "events/app_events.h"

#include <ds/ui/scroll/scroll_area.h>
#include <ds/ui/scroll/scroll_list.h>
#include <ds/ui/scroll/infinity_scroll_list.h>

#include "model/generated/story_model.h"
#include "ui/info_list/info_list.h"
#include "ui/info_list/info_list_item.h"

namespace example {

ScrollExample::ScrollExample()
	: inherited(ds::RootList()
							//	.persp() // sample ortho view
							  .ortho()
							//	.pickColor()

								.persp() // sample perp view
								.perspFov(60.0f)
								.perspPosition(ci::vec3(0.0, 0.0f, 10.0f))
								.perspTarget(ci::vec3(0.0f, 0.0f, 0.0f))
								.perspNear(0.0002f)
								.perspFar(20.0f)
								.ortho()
								.ortho() ) // ortho view on top
	, mGlobals(mEngine , mAllData )
	, mQueryHandler(mEngine, mAllData)
	, mIdling( false )
{


	/*fonts in use */
	//mEngine.editFonts().install(ds::Environment::getAppFile("data/fonts/NotoSans-Regular.ttf"), "Noto Sans");
}

void ScrollExample::setupServer(){

	mEngine.loadSettings("FONTS", "fonts.xml");
	mEngine.editFonts().clear();
	mEngine.getSettings("FONTS").forEachSetting([this](ds::cfg::Settings::Setting& setting){
		mEngine.editFonts().registerFont(ds::Environment::expand(setting.mRawValue), setting.mName);
	});

	/* Settings */
	mEngine.loadSettings(SETTINGS_LAYOUT, "layout.xml");
	mEngine.loadTextCfg("text.xml");

	const int numRoots = mEngine.getRootCount();
	for(int i = 0; i < numRoots - 1; i++){
		// don't clear the last root, which is the debug draw
		if(mEngine.getRootBuilder(i).mDebugDraw) continue;

		ds::ui::Sprite& rooty = mEngine.getRootSprite(i);
		if(rooty.getPerspective()){
			const float clippFar = mGlobals.getSettingsLayout().getFloat("trends:sphere:clipping_far", 0, mEngine.getWorldWidth());
			const float fov = mGlobals.getSettingsLayout().getFloat("trends:sphere:fov", 0, 60.0f);
			ds::PerspCameraParams p = mEngine.getPerspectiveCamera(i);
			p.mTarget = ci::vec3(mEngine.getWorldWidth() / 2.0f, mEngine.getWorldHeight() / 2.0f, 0.0f);
			p.mFarPlane = clippFar;
			p.mFov = fov;
			p.mPosition = ci::vec3(mEngine.getWorldWidth() / 2.0f, mEngine.getWorldHeight() / 2.0f, mEngine.getWorldWidth() / 2.0f);
			mEngine.setPerspectiveCamera(i, p);
		} else {
			mEngine.setOrthoViewPlanes(i, -10000.0f, 10000.0f);
		}

		rooty.clearChildren();
	}

	// Change the root sprite id to be ortho (0) or persp (1) to test the different kinds
	ds::ui::Sprite &rootSprite = mEngine.getRootSprite(0);
	//ds::ui::Sprite &rootSprite = mEngine.getRootSprite(1);
	rootSprite.setTransparent(false);
	rootSprite.setColor(ci::Color(0.1f, 0.1f, 0.1f));



	ds::ui::Text* outputTexter = new ds::ui::Text(mEngine);
	outputTexter->setFont("Noto Sans", 18.0f);
	outputTexter->setColor(ci::Color::white());
	outputTexter->setPosition(100.0f, 800.0f);
	outputTexter->setText("Above on the left is a simple scroll area. The right side is a scroll list.");
	rootSprite.addChildPtr(outputTexter);

	// add sprites

	// ----------- A basic scroll area with a couple colors ------------------------------- //
	ds::ui::ScrollArea* sa = new ds::ui::ScrollArea(mEngine, 400.0f, 700.0f);
	sa->setFadeColors(ci::ColorA(0.0f, 0.0f, 0.0f, 1.0f), ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f));
	sa->setFadeHeight(50.0f);
	sa->setUseFades(true);
	sa->setPosition(100.0f, 100.0f);
	rootSprite.addChildPtr(sa);

	ds::ui::Sprite* testerA = new ds::ui::Sprite(mEngine, 400.0f, 100.0f);
	testerA->setTransparent(false);
	testerA->setColor(ci::Color(0.5f, 0.1f, 0.3f));
	testerA->enable(false);
	sa->addSpriteToScroll(testerA);

	ds::ui::Sprite* testerB = new ds::ui::Sprite(mEngine, 400.0f, 300.0f);
	testerB->setTransparent(false);
	testerB->setColor(ci::Color(0.1f, 0.5f, 0.3f));
	testerB->enable(false);
	testerB->setPosition(0.0f, 120.0f);
	sa->addSpriteToScroll(testerB);
	sa->resetScrollerPosition();


	// ------------ A scroll list to display some info with custom graphics ---------------//
	// ------------ Info list extends ScrollList  ----------------------------------------//
	mInfoList = new InfoList(mGlobals);
	rootSprite.addChildPtr(mInfoList);
	mInfoList->setPosition(600.0f, 100.0f);

	// In a real app, you would also call this from an event dispatched when the query completed
	mInfoList->setInfo(mAllData.mAllStories.mStories);

	mInfoList->setInfoItemCallback([this, outputTexter](ds::model::StoryRef infoThing, ci::vec3 possy){
		outputTexter->setText(infoThing.getName());
		//mInfoList->nextPage();
	});



	// ----------- A scroll list just like InfoList, but implemented here instead of a separate class ----------//
	//ds::ui::ScrollList* instanceList = new ds::ui::ScrollList(mEngine, false);
	const float itemSize = mGlobals.getSettingsLayout().getFloat("info_list:item:height", 0, 100.0f);
	ds::ui::infinityList* instanceList = new ds::ui::infinityList(mEngine,600,itemSize, false);
	instanceList->setSize(600.0f, itemSize);
	instanceList->enable(false);


	instanceList->setItemTappedCallback([this, outputTexter](ds::ui::Sprite* bs, const ci::vec3& cent){
		InfoListItem* rpi = dynamic_cast<InfoListItem*>(bs);
		if(rpi){
			outputTexter->setText(rpi->getInfo().getName());
		}
	});

	instanceList->setCreateItemCallback([this, instanceList]()->ds::ui::Sprite* {
		const float itemSize = mGlobals.getSettingsLayout().getFloat("info_list:item:height", 0, 100.0f);
		return new InfoListItem(mGlobals, instanceList->getWidth(), itemSize);
	});

	instanceList->setDataCallback([this](ds::ui::Sprite* bs, int dbId){
		InfoListItem* rpi = dynamic_cast<InfoListItem*>(bs);
		if(rpi){
			rpi->setInfo(mInfoMap[dbId]);
		}
	});

	instanceList->setAnimateOnCallback([this](ds::ui::Sprite* bs, const float delay){
		InfoListItem* rpi = dynamic_cast<InfoListItem*>(bs);
		if(rpi){
			rpi->animateOn(delay);
		}
	});

	instanceList->setStateChangeCallback([this](ds::ui::Sprite* bs, const bool highlighted){
		InfoListItem* rpi = dynamic_cast<InfoListItem*>(bs);
		if(rpi){
			rpi->setState(highlighted);
		}
	});


	const float padding = mGlobals.getSettingsLayout().getFloat("info_list:item:pad", 0, 20.0f);
	instanceList->setLayoutParams(20.0f, 0.0f, instanceList->getWidth() + padding);

	mInfoMap.clear();

	// The vector of ints is how ScrollList keeps track of items.
	// Db Id's are unique.
	// We keep a map of the ids in this class so we can match up the UI with the content when new items scroll into view
	std::vector<int> productIds;
	for(auto it = mAllData.mAllStories.mStories.begin(); it < mAllData.mAllStories.mStories.end(); ++it){
		productIds.push_back((*it).getId());
		mInfoMap[(*it).getId()] = (*it);
	}

	instanceList->setPosition(1300.0f, 100.0f);
	instanceList->setContent(productIds);
	rootSprite.addChildPtr(instanceList);
}

void ScrollExample::update() {
	inherited::update();

	if( mEngine.isIdling() && !mIdling ){
		//Start idling
		mIdling = true;
		mEngine.getNotifier().notify( IdleStartedEvent() );
	} else if ( !mEngine.isIdling() && mIdling ){
		//Stop idling
		mIdling = false;
		mEngine.getNotifier().notify( IdleEndedEvent() );
	}

}

void ScrollExample::onKeyDown(ci::app::KeyEvent event){
	using ci::app::KeyEvent;

	if(event.getChar() == KeyEvent::KEY_r){ // R = reload all configs and start over without quitting app
		ds::ui::SpriteShader::clearShaderCache();
		setupServer();
	} else if(event.getCode() == KeyEvent::KEY_d){
		moveCamera(ci::vec3(1.0f, 0.0f, 0.0f));
	} else if(event.getCode() == KeyEvent::KEY_a){
		moveCamera(ci::vec3(-1.0f, 0.0f, 0.0f));
	} else if(event.getCode() == KeyEvent::KEY_w){
		moveCamera(ci::vec3(0.0f, -1.0f, 0.0f));
	} else if(event.getCode() == KeyEvent::KEY_s){
		moveCamera(ci::vec3(0.0f, 1.0f, 0.0f));
	} else if(event.getCode() == KeyEvent::KEY_RIGHTBRACKET){
		moveCamera(ci::vec3(0.0f, 0.0f, 1.0f));
	} else if(event.getCode() == KeyEvent::KEY_LEFTBRACKET){
		moveCamera(ci::vec3(0.0f, 0.0f, -1.0f));
	} else if(event.getCode() == KeyEvent::KEY_EQUALS){
		ds::PerspCameraParams p = mEngine.getPerspectiveCamera(1);
		p.mFarPlane += 1.0f;
		std::cout << "Clip Far camera: " << p.mFarPlane << std::endl;
		mEngine.setPerspectiveCamera(1, p);
	} else if(event.getCode() == KeyEvent::KEY_MINUS){
		ds::PerspCameraParams p = mEngine.getPerspectiveCamera(1);
		p.mFarPlane -= 1.0f;
		std::cout << "Clip Far camera: " << p.mFarPlane << std::endl;
		mEngine.setPerspectiveCamera(1, p);
	}
}

void ScrollExample::moveCamera(const ci::vec3& deltaMove){
	ds::PerspCameraParams p = mEngine.getPerspectiveCamera(1);
	p.mPosition += deltaMove;
	std::cout << "Moving camera: " << p.mPosition.x << " " << p.mPosition.y << " " << p.mPosition.z << std::endl;
	mEngine.setPerspectiveCamera(1, p);
}

} // namespace example

// This line tells Cinder to actually create the application
CINDER_APP(example::ScrollExample, ci::app::RendererGl(ci::app::RendererGl::Options().msaa(4)),
		   [&](ci::app::App::Settings* settings) { settings->setBorderless(true); })

