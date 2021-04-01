#include "ds/physics/sprite_body.h"

#include <ds/app/app.h>
#include <ds/app/engine/engine.h>
#include <ds/math/math_defs.h>

#include "ds/physics/body_builder.h"
#include "service.h"
#include "world.h"

#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Dynamics/b2World.h"
#include "Box2D/Dynamics/Joints/b2Joint.h"
#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "Box2D/Dynamics/Contacts/b2Contact.h"
#include "Box2D/Collision/b2Collision.h"
#include "Box2D/Dynamics/b2WorldCallbacks.h"


namespace ds {
namespace physics {

const int		WORLD_CATEGORY_BIT = 1;

namespace {

// Statically initialize the service class. Done here because the Body is
// guaranteed to be referenced by the final application.
class Init {
public:
	Init() {
		ds::App::AddStartup([](ds::Engine& e) {
			ds::physics::Service*		s = new ds::physics::Service(e);
			if (!s) throw std::runtime_error("Can't create ds::physics::Service");
			e.addService(SERVICE_NAME, *s);
		});

	}

	void			doNothing() { }
};
Init				INIT;

}

/**
 * \class SpriteBody
 */
SpriteBody::SpriteBody(ds::ui::Sprite& s, const int world_id)
		: mWorld(s.getEngine().getService<ds::physics::Service>(SERVICE_NAME).getWorld(world_id))
		, mSprite(s)
		, mBody(nullptr) {
	// This shouldn't be necessary, but I just want to make sure the static is referenced.
	INIT.doNothing();

	s.enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	s.setProcessTouchCallback([this](ds::ui::Sprite* s, const ds::ui::TouchInfo& ti) { this->processTouchInfo(s, ti); });
}

SpriteBody::~SpriteBody() {
	mWorld.setCollisionCallback(mSprite,  nullptr);
	destroy();
}

bool SpriteBody::empty() const {
	return !mBody;
}

void SpriteBody::create(const BodyBuilder& b) {
	destroy();

	b2BodyDef			def;
	
	if (b.mIsStaticBody) {
		def.type = b2_staticBody;
	} else if (b.mIsKinematicBody) {
		def.type = b2_kinematicBody;
	} else {
		def.type = b2_dynamicBody;
	}

	def.allowSleep = b.mAllowSleep;
	def.userData = &mSprite;
	def.position = mWorld.Ci2BoxTranslation(mSprite.getPosition(), &mSprite);
	def.linearDamping = b.mLinearDampening;
	def.angularDamping = b.mAngularDampening;
	def.fixedRotation = b.mFixedRotation;

	mBody = mWorld.mWorld->CreateBody(&def);
	if (!mBody) return;

	b.createFixture(*this);
}

b2DistanceJoint* SpriteBody::createDistanceJoint(SpriteBody& body, float length, float dampingRatio, float frequencyHz, const ci::vec3 bodyAOffset, const ci::vec3 bodyBOffset) {
	return mWorld.createDistanceJoint(*this, body, length, dampingRatio, frequencyHz, bodyAOffset, bodyBOffset);
}

void SpriteBody::createWeldJoint(SpriteBody& body,const float damping, const float frequency, const ci::vec3 bodyAOffset, const ci::vec3 bodyBOffset) {
	mWorld.createWeldJoint(*this, body, damping, frequency, bodyAOffset, bodyBOffset);
}

b2PrismaticJoint* SpriteBody::createPrismaticJoint(const SpriteBody& body, ci::vec2 axis, bool enableLimit, float lowerTranslation, float upperTranslation,
	bool enableMotor , float maxMotorForce , float motorSpeed,
	const ci::vec3 bodyAOffset , const ci::vec3 bodyBOffset ) {
	b2Vec2 bAxis(axis.x, axis.y);
	return mWorld.createPrismaticJoint(*this, body, bAxis,  enableLimit, lowerTranslation, upperTranslation, enableMotor, maxMotorForce, motorSpeed, bodyAOffset, bodyBOffset);
}

void SpriteBody::destroy() {
	if (!mBody) return;

	
	// Destroying a body also destroys all joints associated with that body.
	releaseJoints();
	mWorld.mWorld->DestroyBody(mBody);
	mBody = nullptr;
}

void SpriteBody::releaseJoints() {
	mWorld.releaseJoints(*this);
}

void SpriteBody::setActive(bool flag) {
	if (!mBody) return;
	// Setting a body as inactive also sets all associated joints as inactive, but does not delete them from the world.
	mBody->SetActive(flag);
}

void SpriteBody::enableCollisions(const bool on) {
	if (!mBody) return;

	const bool		sensor = !on;
	b2Fixture*		fix = mBody->GetFixtureList();
	while (fix) {
		fix->SetSensor(sensor);
		fix = fix->GetNext();
	}
}

void SpriteBody::resizeDistanceJoint(SpriteBody& body, float length) {

	mWorld.resizeDistanceJoint(*this, body, length);
}

void SpriteBody::processTouchInfo(ds::ui::Sprite*, const ds::ui::TouchInfo& ti) {
	if (!mBody) return;

	if (ti.mPhase == ti.Added) mWorld.processTouchAdded(*this, ti);
	else if (ti.mPhase == ti.Moved) mWorld.processTouchMoved(*this, ti);
	else if (ti.mPhase == ti.Removed) mWorld.processTouchRemoved(*this, ti);
}

void SpriteBody::processTouchAdded(const ds::ui::TouchInfo& ti) {
	mWorld.processTouchAdded(*this, ti);
}

void SpriteBody::processTouchMoved(const ds::ui::TouchInfo& ti) {
	mWorld.processTouchMoved(*this, ti);
}

void SpriteBody::processTouchRemoved(const ds::ui::TouchInfo& ti) {
	mWorld.processTouchRemoved(*this, ti);
}

void SpriteBody::setPosition(const ci::vec3& pos) {
	if (!mBody) return;

	const b2Vec2		boxpos = mWorld.Ci2BoxTranslation(pos, &mSprite);
	mBody->SetTransform(boxpos, mBody->GetAngle());
}

void SpriteBody::clearVelocity() {
	if (!mBody) return;

	b2Vec2			zeroVec;
	zeroVec.SetZero();
	mBody->SetLinearVelocity(zeroVec);
	mBody->SetAngularVelocity(0);
}

void SpriteBody::setLinearVelocity(const float x, const float y) {
	if (mBody) {
		mBody->SetLinearVelocity(b2Vec2(x, y));
	}
}


ci::vec2 SpriteBody::getLinearVelocity() {
	b2Vec2 vel = b2Vec2(0.0f, 0.0f);

	if (mBody) {
		vel =  mBody->GetLinearVelocity();
	}
	return ci::vec2(vel.x, vel.y);
}


void SpriteBody::applyForceToCenter(const float x, const float y) {
	if (mBody) {
		mBody->ApplyForceToCenter(b2Vec2(x, y), true);
	}
}
void SpriteBody::applyImpulseToCenter(const float x, const float y, ci::vec2 point) {
	if (mBody) {
		mBody->ApplyLinearImpulse (b2Vec2(x, y), b2Vec2(point.x, point.y), true);
	}
}

void SpriteBody::setRotation(const float degree) {
	if (!mBody) return;

	const float		angle = degree * ds::math::DEGREE2RADIAN;
	mBody->SetTransform(mBody->GetPosition(), angle);
	if (!mBody->IsAwake()) {
		// You'd think setting the transform would wake up the body,
		// but nope.
		mBody->SetAwake(true);
	}
}

float SpriteBody::getRotation() const {
	if (!mBody) return 0.0f;
	return mBody->GetAngle() * ds::math::RADIAN2DEGREE;
}

void SpriteBody::setCollisionCallback(const std::function<void(const Collision&)>& fn) {
	mWorld.setCollisionCallback(mSprite, fn);
}

void SpriteBody::setContactPreSolveFn(const std::function<void( b2Contact* , const b2Manifold* )>& fn) {

	mWorld.mContactListener.setPreSolveFunction(fn);
}

void SpriteBody::setContactPostSolveFn(const std::function<void( b2Contact*, const b2ContactImpulse*)>& fn) {

	mWorld.mContactListener.setPostSolveFunction(fn);
}
void SpriteBody::setBeginContactFn(const std::function<void(b2Contact*)>& fn){
	mWorld.mContactListener.setBeginContactFunction(fn);
}

void SpriteBody::setEndContactFn(const std::function<void(b2Contact*)>& fn){
	mWorld.mContactListener.setEndContactFunction(fn);

}

void SpriteBody::onCenterChanged() {
	if (!mBody) return;

	// Currently there should only be 1 fixture.
	b2Fixture*				fix = mBody->GetFixtureList();
	while (fix) {
		b2PolygonShape*		poly = dynamic_cast<b2PolygonShape*>(fix->GetShape());
		if (poly) {
			const float32	w = mSprite.getWidth() / 2.0f * mWorld.getCi2BoxScale(),
							h = mSprite.getHeight() / 2.0f * mWorld.getCi2BoxScale();
			// Convert the sprite center into a box2d center.
			const ci::vec2	cen((mSprite.getCenter().x * 2) - 1.0f,
								(mSprite.getCenter().y * 2) - 1.0f);
			b2Vec2			box_cen;
			box_cen.x = cen.x * -w;
			box_cen.y = cen.y * -h;
			poly->SetAsBox(w, h, box_cen, 0.0f);

			return;
		}
		fix = fix->GetNext();
	}
}

float	SpriteBody::getPhysWorldScale(){
	return mWorld.getCi2BoxScale();

}


void SpriteBody::runWorldAhead(const int iterations) {
	mWorld.runAhead(iterations);
}

bool SpriteBody::isWorldLocked() const
{
	return mWorld.isLocked();
}

} // namespace physics
} // namespace ds
