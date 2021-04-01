#include "world.h"

#include <cinder/CinderMath.h>
#include <ds/app/auto_update.h>
#include <ds/app/environment.h>
#include <ds/app/engine/engine.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite.h>
#include <ds/physics/collision.h>
#include <ds/physics/sprite_body.h>
#include "debug_draw.h"
#include "service.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/b2World.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "Box2D/Dynamics/Joints/b2WeldJoint.h"
#include "Box2D/Dynamics/Joints/b2PrismaticJoint.h"

namespace ds {
namespace physics {

namespace {
// All this just to keep track of the bound edges
int							BOUNDS_LEFT = 1;
int							BOUNDS_TOP = 2;
int							BOUNDS_RIGHT = 3;
int							BOUNDS_BOTTOM = 4;
void*						BOUNDS_LEFT_PTR = reinterpret_cast<void*>(&BOUNDS_LEFT);
void*						BOUNDS_TOP_PTR = reinterpret_cast<void*>(&BOUNDS_TOP);
void*						BOUNDS_RIGHT_PTR = reinterpret_cast<void*>(&BOUNDS_RIGHT);
void*						BOUNDS_BOTTOM_PTR = reinterpret_cast<void*>(&BOUNDS_BOTTOM);
}

// NOTE: Ideally we'd initialize the World as an Engine service in a static here.
// However, because I'm in a library statically linked to the main application, that
// initialization will get tossed, so it has to happen from a class that the app
// will reference. Currently it's happening in Body.

/**
 * \class ds::physics::World
 */
World::World(ds::ui::SpriteEngine& e, ds::ui::Sprite& spriddy)
		: ds::AutoUpdate(e)
		, mSprite(spriddy)
		, mTouch(*this)
		, mContactListener(*this)
		, mContactListenerRegistered(false)
		, mGround(nullptr)
		, mBounds(nullptr)
		, mCi2BoxScale(0.02f)
		, mFriction(0.5f)
		, mLinearDampening(0.0f)
		, mAngularDampening(0.0f)
		, mFixedRotation(true) 
		, mMouseMaxForce(10000.0f)
		, mMouseDampening(1.0f)
		, mMouseFrequencyHz(25.0f)
		, mTranslateToLocalSpace(false)
		, mSettings()
{
	mWorld = std::move(std::unique_ptr<b2World>(new b2World(b2Vec2(0.0f, 0.0f))));
	if (mWorld.get() == nullptr) {
		DS_LOG_WARNING("ds::physics::World() can't create b2World");
		return;
	}
	b2BodyDef		def;
	mGround = mWorld->CreateBody(&def);
	if (!mGround) {
		DS_LOG_WARNING("ds::physics::World() can't create mGround");
		return;
	}

	ds::Environment::loadSettings("physics", "physics.xml", mSettings);
	mTranslateToLocalSpace = mSettings.getBool("use_local_translation", 0, false);
	mFriction = mSettings.getFloat("friction", 0, mFriction);
	mLinearDampening = mSettings.getFloat("dampening:linear", 0, mLinearDampening);
	mAngularDampening = mSettings.getFloat("dampening:angular", 0, mAngularDampening);
	mFixedRotation = mSettings.getBool("rotation:fixed", 0, mFixedRotation);

	mMouseMaxForce = mSettings.getFloat("mouse:max_force", 0, mMouseMaxForce);
	mMouseDampening = mSettings.getFloat("mouse:dampening", 0, mMouseDampening);
	mMouseFrequencyHz = mSettings.getFloat("mouse:frequency_hz", 0, mMouseFrequencyHz);

	mVelocityIterations = mSettings.getInt("step:velocity_iterations", 0, 6);
	mPositionIterations = mSettings.getInt("step:position_iterations", 0, 2);
	mFixedStep = mSettings.getBool("step:fixed", 0, false);
	mFixedStepAmount = mSettings.getFloat("step:fixed_amount", 0, 1.0f/60.0f);

	// Slightly complicated, but flexible: Bounds can be either fixed or unit,
	// or a combination of both, which applies the fixed as an offset.
	if (mSettings.hasSetting("bounds:fixed") && mSettings.hasSetting("bounds:unit")) {
		const ci::Rectf&		unit = mSettings.getRect("bounds:unit");
		const ci::Rectf&		fixed = mSettings.getRect("bounds:fixed");
		ci::Rectf				r(	(unit.x1 * e.getWorldWidth()) + fixed.x1, (unit.y1 * e.getWorldHeight()) + fixed.y1,
									(unit.x2 * e.getWorldWidth()) + fixed.x2, (unit.y2 * e.getWorldHeight()) + fixed.y2);
		setBounds(r, mSettings.getFloat("bounds:restitution", 0, 1.0f));
	} else if (mSettings.hasSetting("bounds:fixed")) {
		setBounds(	mSettings.getRect("bounds:fixed"),
					mSettings.getFloat("bounds:restitution", 0, 1.0f));
	} else if (mSettings.hasSetting("bounds:unit")) {
		const ci::Rectf&		r = mSettings.getRect("bounds:unit");
		setBounds(	ci::Rectf(r.x1 * e.getWorldWidth(), r.y1 * e.getWorldHeight(), r.x2 * e.getWorldWidth(), r.y2 * e.getWorldHeight()),
					mSettings.getFloat("bounds:restitution", 0, 1.0f));
	}

	if (mSettings.getBool("draw_debug", 0, false)) {
		mDebugDraw.reset(new DebugDraw(e, *(mWorld.get()), *this));
	}
}

b2DistanceJoint* World::createDistanceJoint(const SpriteBody& body1, const SpriteBody& body2, float length, float dampingRatio, float frequencyHz,
	const ci::vec3 bodyAOffset, const ci::vec3 bodyBOffset) {
	
	if (body1.mBody && body2.mBody) {
		b2DistanceJointDef jointDef;
		jointDef.bodyA = body1.mBody;
		jointDef.bodyB = body2.mBody;
		jointDef.localAnchorA = Ci2BoxTranslation(body1.mSprite.getCenter() + bodyAOffset, nullptr);
		jointDef.localAnchorB = Ci2BoxTranslation(body2.mSprite.getCenter() + bodyBOffset, nullptr);
		
		jointDef.dampingRatio = dampingRatio;
		jointDef.frequencyHz = frequencyHz;
		
		jointDef.collideConnected = true;
		jointDef.length = getCi2BoxScale()*(length);
		b2DistanceJoint* joint = (b2DistanceJoint*) mWorld->CreateJoint(&jointDef);

		//DS_LOG_INFO_M("Joint anchors a=(" << joint->GetAnchorA().x << ", " << joint->GetAnchorA().y << ") b=(" << joint->GetAnchorB().x << ", " << joint->GetAnchorB().y << ")", PHYSICS_LOG);

		mDistanceJoints.insert(mDistanceJoints.end(), joint);
		return joint;
	}
	return nullptr;
}

b2PrismaticJoint* World::createPrismaticJoint(const SpriteBody& body1, const SpriteBody& body2, b2Vec2 axis, bool enableLimit, float lowerTranslation, float upperTranslation,
	bool enableMotor, float maxMotorForce, float motorSpeed,
	const ci::vec3 bodyAOffset, const ci::vec3 bodyBOffset) {

	if (body1.mBody && body2.mBody) {
		b2PrismaticJointDef jointDef; 
		jointDef.Initialize(body1.mBody, body2.mBody, b2Vec2(0.0f, 0.0f), axis);
		jointDef.bodyA = body1.mBody;
		jointDef.bodyB = body2.mBody;
		jointDef.lowerTranslation = getCi2BoxScale()*lowerTranslation;
		jointDef.upperTranslation = getCi2BoxScale()*upperTranslation;
		jointDef.enableLimit = enableLimit;
		jointDef.maxMotorForce = maxMotorForce;
		jointDef.motorSpeed = motorSpeed;
		jointDef.enableMotor = enableMotor;

		jointDef.localAnchorA = Ci2BoxTranslation(body1.mSprite.getCenter() + bodyAOffset, nullptr);
		jointDef.localAnchorB = Ci2BoxTranslation(body2.mSprite.getCenter() + bodyBOffset, nullptr);

		//jointDef.dampingRatio = dampingRatio;
		//jointDef.frequencyHz = frequencyHz;

		jointDef.collideConnected = true;
		//jointDef.length = getCi2BoxScale()*(length);
	
		b2PrismaticJoint* joint = (b2PrismaticJoint*)mWorld->CreateJoint(&jointDef);

		//DS_LOG_INFO_M("Joint anchors a=(" << joint->GetAnchorA().x << ", " << joint->GetAnchorA().y << ") b=(" << joint->GetAnchorB().x << ", " << joint->GetAnchorB().y << ")", PHYSICS_LOG);

		mPrismaticJoints.insert(mPrismaticJoints.end(), joint);
		return joint;
	}
	return nullptr;
}


void World::resizeDistanceJoint(const SpriteBody& body1, const SpriteBody& body2, float length) {
	for(auto it  = mDistanceJoints.begin(); it != mDistanceJoints.end(); ++it) {
		b2DistanceJoint* joint  = *it;
		if (joint->GetBodyA() == body1.mBody && joint->GetBodyB() == body2.mBody
			|| joint->GetBodyB() == body1.mBody && joint->GetBodyA() == body2.mBody) {
				joint->SetLength(length*getCi2BoxScale());
		}
	}
}

void World::createWeldJoint(const SpriteBody& body1, const SpriteBody& body2, const float damping, const float frequency, const ci::vec3 bodyAOffset, const ci::vec3 bodyBOffset) {
	if (body1.mBody && body2.mBody) {
		b2WeldJointDef jointDef;
		jointDef.bodyA = body1.mBody;
		jointDef.bodyB = body2.mBody;
		jointDef.localAnchorA = Ci2BoxTranslation(body1.mSprite.getCenter() + bodyAOffset, nullptr);
		jointDef.localAnchorB = Ci2BoxTranslation(body2.mSprite.getCenter() + bodyBOffset, nullptr);
		jointDef.dampingRatio = damping;
		jointDef.frequencyHz = frequency;
		jointDef.referenceAngle = jointDef.bodyA->GetAngle() - jointDef.bodyB->GetAngle();
		jointDef.collideConnected = false;

		b2WeldJoint* joint = (b2WeldJoint*) mWorld->CreateJoint(&jointDef);

		//DS_LOG_INFO_M("Joint anchors a=(" << joint->GetAnchorA().x << ", " << joint->GetAnchorA().y << ") b=(" << joint->GetAnchorB().x << ", " << joint->GetAnchorB().y << ")", PHYSICS_LOG);

		mWeldJoints.insert(mWeldJoints.end(), joint);
	}
}

void World::releaseJoints(const SpriteBody& body) {
	if(!body.mBody) return;
	for (int i = 0; i < mDistanceJoints.size(); i++){
		if(mDistanceJoints[i]->GetBodyA() == body.mBody || mDistanceJoints[i]->GetBodyB() == body.mBody){
			mWorld->DestroyJoint(mDistanceJoints[i]);
			mDistanceJoints.erase(mDistanceJoints.begin() + i);
			i--;
		}
	}

	for (int i = 0; i < mWeldJoints.size(); i++){
		if(mWeldJoints[i]->GetBodyA() == body.mBody || mWeldJoints[i]->GetBodyB() == body.mBody){
			mWorld->DestroyJoint(mWeldJoints[i]);
			mWeldJoints.erase(mWeldJoints.begin() + i);
			i--;
		}
	}
	for (int i = 0; i < mPrismaticJoints.size(); i++){
		if (mPrismaticJoints[i]->GetBodyA() == body.mBody || mPrismaticJoints[i]->GetBodyB() == body.mBody){
			mWorld->DestroyJoint(mPrismaticJoints[i]);
			mPrismaticJoints.erase(mPrismaticJoints.begin() + i);
			i--;
		}
	}
}


void World::processTouchAdded(const SpriteBody& body, const ds::ui::TouchInfo& ti) {	
	mTouch.processTouchAdded(body, ti);
}

void World::processTouchMoved(const SpriteBody& body, const ds::ui::TouchInfo& ti) {
	mTouch.processTouchMoved(body, ti);
}

void World::processTouchRemoved(const SpriteBody& body, const ds::ui::TouchInfo& ti) {
	mTouch.processTouchRemoved(body, ti);
}

float World::getFriction() const
{
	return mFriction;
}

float World::getLinearDampening() const
{
	return mLinearDampening;
}

float World::getAngularDampening() const
{
	return mAngularDampening;
}

bool World::getFixedRotation() const
{
	return mFixedRotation;
}

void World::setCollisionCallback(const ds::ui::Sprite& s, const std::function<void(const Collision&)>& fn)
{
	mContactListener.setCollisionCallback(s, fn);
	if (!mContactListenerRegistered) {
		mContactListenerRegistered = true;
		mWorld->SetContactListener(&mContactListener);
	}
}

bool World::makeCollision(const b2Fixture& fix, Collision& c) const
{
	if (!mBounds) return false;
	if (fix.GetBody() != mBounds) return false;

	if (fix.GetUserData() == BOUNDS_LEFT_PTR) c.setToWorldBounds(Collision::LEFT);
	else if (fix.GetUserData() == BOUNDS_TOP_PTR) c.setToWorldBounds(Collision::TOP);
	else if (fix.GetUserData() == BOUNDS_RIGHT_PTR) c.setToWorldBounds(Collision::RIGHT);
	else if (fix.GetUserData() == BOUNDS_BOTTOM_PTR) c.setToWorldBounds(Collision::BOTTOM);
	else return false;

	return true;
}

void World::runAhead(const int iterations) {
	ds::UpdateParams p = ds::UpdateParams();
	p.setDeltaTime(1.0f / 60.0f);
	for (int i = 0; i < iterations; i++) {
		update(p);
	}
}

void World::update(const ds::UpdateParams& p)
{
	mContactListener.clear();
	
	if(mFixedStep){
		mWorld->Step(mFixedStepAmount, mVelocityIterations, mPositionIterations);
	} else {
		mWorld->Step(p.getDeltaTime(), mVelocityIterations, mPositionIterations);
	}
	
	// Update all objects
	for ( b2Body* b = mWorld->GetBodyList(); b; b = b->GetNext() )
	{
		if ( b->GetType() != b2_dynamicBody )
			continue;
		if ( b->IsAwake() )
		{
			ds::ui::Sprite*	sprite = reinterpret_cast<ds::ui::Sprite*>( b->GetUserData() );
			if (sprite)
			{
				auto pos = box2CiTranslation(b->GetPosition(), sprite);
				sprite->setPosition(pos);
				sprite->setRotation(ci::toDegrees(b->GetAngle()));
			}
		}
	}

	mContactListener.report();
}

float World::getCi2BoxScale() const {
	return mCi2BoxScale;
}

ci::vec3 World::box2CiTranslation(const b2Vec2 &vec, ds::ui::Sprite* sp) const
{
	if(mTranslateToLocalSpace && sp && sp->getParent()){
		return  sp->getParent()->globalToLocal(ci::vec3(vec.x / mCi2BoxScale, vec.y / mCi2BoxScale, 0.0f));
	} else {
		return ci::vec3(vec.x / mCi2BoxScale, vec.y / mCi2BoxScale, 0.0f);
	}
}

b2Vec2 World::Ci2BoxTranslation(const ci::vec3 &vec, ds::ui::Sprite* sp) const
{
	if(mTranslateToLocalSpace && sp && sp->getParent()){
		ci::vec3 globalPoint = sp->getParent()->localToGlobal(vec) * mCi2BoxScale;
		return b2Vec2(globalPoint.x, globalPoint.y);
	} else {
		return b2Vec2(vec.x * mCi2BoxScale, vec.y * mCi2BoxScale);
	}
}

static void set_polygon_shape(	const World& trans,
								const float l, const float t, const float r, const float b,
								b2PolygonShape& out) {
	const b2Vec2	lt = trans.Ci2BoxTranslation(ci::vec3(l, t, 0.0f), nullptr); // using nullptr for the sprite will make the polygon be in world space
	const b2Vec2	rb = trans.Ci2BoxTranslation(ci::vec3(r, b, 0.0f), nullptr);
	b2Vec2			vtx[4];
	vtx[0].Set(lt.x, lt.y);
	vtx[1].Set(rb.x, lt.y);
	vtx[2].Set(rb.x, rb.y);
	vtx[3].Set(lt.x, rb.y);
	out.Set(vtx, 4);
}

void World::setBounds(const ci::Rectf& f, const float restitution) {
	if (f.x2 <= f.x1 || f.y2 <= f.y1) {
		DS_LOG_WARNING_M("World constructed on invalid bounds (" << f << ")", PHYSICS_LOG);
		return;
	}

	b2BodyDef		def;
	def.type = b2_staticBody;
	def.fixedRotation = true;
	mBounds = mWorld->CreateBody(&def);
	if (!mBounds) throw std::runtime_error("ds::physics::World() can't create mBounds");

	b2PolygonShape	shape;
	b2FixtureDef	fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 0.0f;
	fixtureDef.friction = mFriction;
	fixtureDef.restitution = restitution;

	// Make the world walls large enough to prevent anything from flying outside.
	const float		world_w = f.getWidth(),
					world_h = f.getHeight();

	// left
	set_polygon_shape(*this, f.x1-world_w, f.y1-world_h, f.x1, f.y2+world_h, shape);
	fixtureDef.userData = BOUNDS_LEFT_PTR;
	mBounds->CreateFixture(&fixtureDef);
	// top
	set_polygon_shape(*this, f.x1-world_w, f.y1-world_h, f.x2+world_w, f.y1, shape);
	fixtureDef.userData = BOUNDS_TOP_PTR;
	mBounds->CreateFixture(&fixtureDef);
	// right
	set_polygon_shape(*this, f.x2, f.y1-world_h, f.x2+world_w, f.y2+world_h, shape);
	fixtureDef.userData = BOUNDS_RIGHT_PTR;
	mBounds->CreateFixture(&fixtureDef);
	// bottom
	set_polygon_shape(*this, f.x1-world_w, f.y2, f.x2+world_w, f.y2+world_h, shape);
	fixtureDef.userData = BOUNDS_BOTTOM_PTR;
	mBounds->CreateFixture(&fixtureDef);
}

bool World::isLocked() const
{
	return mWorld->IsLocked();
}

} // namespace physics
} // namespace ds
