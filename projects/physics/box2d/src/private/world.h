#pragma once
#ifndef DS_PHYSICS_PRIVATE_WORLD_H_
#define DS_PHYSICS_PRIVATE_WORLD_H_

#include <unordered_map>
#include <cinder/Vector.h>
#include <ds/app/auto_draw.h>
#include <ds/app/auto_update.h>
#include <ds/app/engine/engine_service.h>
#include <ds/cfg/settings.h>
#include <ds/ui/touch/touch_info.h>
#include "private/contact_listener.h"
#include "private/touch.h"

#include "Box2D/Dynamics/b2World.h"
#include "debug_draw.h"


class b2Body;
class b2DistanceJoint;
class b2MouseJoint;
class b2WeldJoint;
class b2PrismaticJoint;
class b2World;
struct b2Vec2;

namespace ds {
namespace physics {
class DebugDraw;
class SpriteBody;

/**
 * \class ds::physics::World
 */
class World : public ds::EngineService
			, public ds::AutoUpdate {
public:
	World(ds::ui::SpriteEngine&, ds::ui::Sprite&);

	b2DistanceJoint*				createDistanceJoint(const SpriteBody&, const SpriteBody&, float length, float dampingRatio, float frequencyHz,
													const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));
	void							resizeDistanceJoint(const SpriteBody&, const SpriteBody&, float length);
	void							createWeldJoint(const SpriteBody&, const SpriteBody&, const float dampingRation, const float frequencyHz,
													const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));
	b2PrismaticJoint*				createPrismaticJoint(const SpriteBody& body1, const SpriteBody& body2, b2Vec2 axis, bool enableLimit = false, float lowerTranslation = 0.0f, float upperTranslation = 0.0f,
													 bool enableMotor= false, float maxMotorForce=0.0f, float motorSpeed=0.0f,
													 const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));

	void							releaseJoints(const SpriteBody&);

	void							processTouchAdded(const SpriteBody&, const ds::ui::TouchInfo&);
	void							processTouchMoved(const SpriteBody&, const ds::ui::TouchInfo&);
	void							processTouchRemoved(const SpriteBody&, const ds::ui::TouchInfo&);

	float							getCi2BoxScale() const;

	float							getFriction() const;
	float							getLinearDampening() const;
	float							getAngularDampening() const;
	bool							getFixedRotation() const;

	void							setCollisionCallback(const ds::ui::Sprite&, const std::function<void(const Collision&)>& fn);
	// Fill out the collision object, if it applies to me, otherwise return false
	bool							makeCollision(const b2Fixture&, Collision&) const;

	ci::vec3						box2CiTranslation(const b2Vec2&, ds::ui::Sprite*) const;
	b2Vec2							Ci2BoxTranslation(const ci::vec3&, ds::ui::Sprite*) const;

	bool							isLocked() const;

	void							runAhead(const int iterations);

protected:
	virtual void					update(const ds::UpdateParams&);

private:
	void							setBounds(const ci::Rectf&, const float restitution);

	friend class ds::physics::SpriteBody;
	friend class ds::physics::Touch;

	ds::ui::Sprite&					mSprite;
	std::unique_ptr<b2World>		mWorld;
	ds::physics::Touch				mTouch;
	ContactListener					mContactListener;
	// Only register the listener with the world if there's someone listening;
	// otherwise, this can incur a bit of overhead.
	bool							mContactListenerRegistered;
	b2Body*							mGround;
	b2Body*							mBounds;

	bool							mTranslateToLocalSpace;

	const float						mCi2BoxScale;

	ds::cfg::Settings				mSettings;
	// Cached from the settings
	float							mFriction,
									mLinearDampening,
									mAngularDampening;
	bool							mFixedRotation;

	float							mMouseMaxForce,
									mMouseDampening,
									mMouseFrequencyHz;

	int								mVelocityIterations,
									mPositionIterations;
	bool							mFixedStep;
	float							mFixedStepAmount;

	std::vector<b2DistanceJoint*>	mDistanceJoints;
	std::vector<b2WeldJoint*>		mWeldJoints;
	std::vector<b2PrismaticJoint*>	mPrismaticJoints;

	std::unique_ptr<DebugDraw>		mDebugDraw;
};

} // namespace physics
} // namespace ds

#endif // DS_PHYSICS_PRIVATE_WORLD_H_
