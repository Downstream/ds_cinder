#include "stdafx.h"

#include "physics_example_app.h"

#include <Poco/String.h>
#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/app/engine/engine.h>


#include <cinder/Rand.h> 
#include <cinder/app/RendererGl.h>


#include "ui/bouncy/bouncy_view.h"

namespace physics {

physics_example_app::physics_example_app()
	: ds::App()
{

}

void physics_example_app::setupServer(){


	ds::ui::Sprite &rootSprite = mEngine.getRootSprite();
	//rootSprite.setTransparent(false);
	//rootSprite.setColor(ci::Color(0.1f, 0.1f, 0.1f));
	
	// add sprites
	rootSprite.addChildPtr(new BouncyView(mEngine));
}


} // namespace physics

// This line tells Cinder to actually create the application
CINDER_APP(physics::physics_example_app, ci::app::RendererGl(ci::app::RendererGl::Options().msaa(4)),
		   [&](ci::app::App::Settings* settings){ settings->setBorderless(true); })
