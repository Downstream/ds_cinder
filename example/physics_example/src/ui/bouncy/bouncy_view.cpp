#include "stdafx.h"

#include "bouncy_view.h"

#include <Poco/LocalDateTime.h>

#include <ds/app/environment.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/sprite/circle.h>
#include <ds/debug/logger.h>
#include <ds/physics/sprite_body.h>
#include <ds/physics/body_builder.h>


namespace physics {

BouncyView::BouncyView(ds::ui::SpriteEngine& g)
	: ds::ui::Sprite(g)
	, mEventClient(g)
{
	mEventClient.listenToEvents<ds::cfg::Settings::SettingsEditedEvent>([this](auto& e) {
		if (e.mSettingsType == "app_settings") {
			rebuildBouncies();
		}
	});

	rebuildBouncies();
}

BouncyView::~BouncyView() {
	clearBouncies();
}

void BouncyView::clearBouncies() {
	for(auto it = mCircles.begin(); it < mCircles.end(); ++it) {
		(*it)->release();
	}

	for(auto it = mPhysicsBodies.begin(); it < mPhysicsBodies.end(); ++it) {
		delete (*it);
	}
	mPhysicsBodies.clear();
	mCircles.clear();
}

void BouncyView::rebuildBouncies() {
	clearBouncies();


	const int   numBouncies = mEngine.getAppSettings().getFloat("circle_physics:number", 0, 100);
	const float startVelocity = mEngine.getAppSettings().getFloat("circle_physics:start_velocity", 0, 0.5f);
	const float restitution = mEngine.getAppSettings().getFloat("circle_physics:restitution", 0, 0.5f);
	const float density = mEngine.getAppSettings().getFloat("circle_physics:density", 0, 1.0f);
	const float radiusyN = mEngine.getAppSettings().getFloat("circle_physics:radius_min", 0, 1.0f);
	const float radiusyX = mEngine.getAppSettings().getFloat("circle_physics:radius_max", 0, 1.0f);
	const float friction = mEngine.getAppSettings().getFloat("circle_physics:friction", 0, 0.0f);
	const float damping = mEngine.getAppSettings().getFloat("circle_physics:damping", 0, 0.0f);
	const bool  fixedRot = mEngine.getAppSettings().getBool("circle_physics:fixed_rotation", 0, true);

	for(int i = 0; i < numBouncies; i++) {

		ds::ui::Circle* ccv = new ds::ui::Circle(mEngine, true, ci::randFloat(radiusyN, radiusyX));
		addChildPtr(ccv);
		ccv->enable(true);
		ccv->setCenter(0.5f, 0.5f);

		ds::ui::Sprite* marker = new ds::ui::Sprite(mEngine, 1.0f, ccv->getRadius());
		marker->setTransparent(false);
		marker->setColor(ci::Color(0.5f, 0.1f, 0.1f));
		marker->setPosition(ccv->getRadius(), 0.0f);
		ccv->addChildPtr(marker);


		ds::physics::SpriteBody* sb = new ds::physics::SpriteBody(*ccv);
		ds::physics::BodyBuilderCircle builder(*sb);

		builder.mDensity = density;
		builder.mLinearDampening = damping;
		builder.mFriction = friction;
		builder.mRestitution = restitution;
		builder.mRadius = ccv->getRadius();
		builder.mFixedRotation = fixedRot;


		sb->create(builder);
		sb->enableCollisions(true);
		sb->setPosition(ci::vec3(ci::randFloat(0.0f, mEngine.getWorldWidth()), ci::randFloat(0.0f, mEngine.getWorldHeight()), 0.0f));
		sb->setLinearVelocity(ci::randFloat(-startVelocity, startVelocity), ci::randFloat(-startVelocity, startVelocity));
		mPhysicsBodies.push_back(sb);
		mCircles.push_back(ccv);
	}
}

void BouncyView::animateOn() {
	show();
	tweenOpacity(1.0f, mEngine.getAnimDur());

	// Recursively animate on any children, including the primary layout
	tweenAnimateOn(true, 0.0f, 0.05f);
}

void BouncyView::animateOff() {
	tweenOpacity(0.0f, mEngine.getAnimDur(), 0.0f, ci::EaseNone(), [this] {hide(); });
}


} // namespace physics

