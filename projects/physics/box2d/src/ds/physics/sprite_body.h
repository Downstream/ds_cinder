#pragma once
#ifndef DS_PHYSICS_SPRITEBODY_H_
#define DS_PHYSICS_SPRITEBODY_H_

#include <vector>
#include <functional>
#include <cinder/Vector.h>

class b2Body;
class b2DistanceJoint;
class b2Contact;
struct b2ContactImpulse;
struct b2Manifold;
class b2PrismaticJoint;
class b2DistancJoint;

namespace ds {
namespace ui {
class Sprite;
class SpriteEngine;
struct TouchInfo;
}

namespace physics {
class BodyBuilder;
class Collision;
class Touch;
class World;

// The collision category for the world.
extern const int		WORLD_CATEGORY_BIT;

/**
 * \class SpriteBody
 * \brief This serves as the sprite entry point to the physics system.
 * Simply including this in a sprite subclass and calling create() is
 * enough to enable physics on a sprite.
 * By default this class will take over touch processing; if you want
 * to mix in your own behavior, take it back.
 */
class SpriteBody {
public:
	// Create a physics body on the sprite using the given world.
	// The default is the global world, which is only constructed
	// if the physics.xml setting "create_world_0" is true. Otherwise,
	// applications need to construct worlds as desired and reference them.
	SpriteBody(ds::ui::Sprite&, const int world_id = 0);
	~SpriteBody();

	bool					empty() const;
	void					create(const BodyBuilder&);

	// Distance joints will move the bodies to keep them at the length specified, but they can be at any angle.
	// damping ratio: 0.0 = no damping (faster), 1.0 = full damping (no movement)
	// frequency: how many times it's applied a second. higher frequency for smoother resolution, lower frequency for better performance (generally)
	b2DistanceJoint*		createDistanceJoint(SpriteBody&, float length, float dampingRatio, float frequencyHz, const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));
	void					resizeDistanceJoint(SpriteBody& body, float length);

	// Weld joints attempt to keep the two bodies at the same relative position. There will be a little bit of elasticness between the two.
	// To make a completely rigid connection, combine two fixtures into the same body (may need to add some API to handle that)
	// By default, the positioning will place the center of one body on the center of the other body, use the offsets to place the bodies somewhere else
	void					createWeldJoint(SpriteBody&, const float damping, const float frequencyHz, const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));

	//Joins two bodies along a single, restricted axis of motion.
	b2PrismaticJoint*		createPrismaticJoint(const SpriteBody& body, ci::vec2 axis, bool enableLimit = false, float lowerTranslation = 0.0f, float upperTranslation = 0.0f,
		bool enableMotor = false, float maxMotorForce = 0.0f, float motorSpeed = 0.0f,
		const ci::vec3 bodyAOffset = ci::vec3(0.0f, 0.0f, 0.0f), const ci::vec3 bodyBOffset = ci::vec3(0.0f, 0.0f, 0.0f));
	// Remove all joints associated with this body. destroy also does this.
	void					releaseJoints();

	// Removes the physics attached with this body, but doesn't remove the sprite.
	// Also releases any joints attached to this body (including joints initiated from a different body)
	void					destroy();
	void					setActive(bool flag);
	// Set this to false to turn the fixture into a sensor, which will cause collisions
	// on this body to stop. NOTE: This state is not tracked. If the body is empty(),
	// this does nothing, and will not effect the body when it's created.
	void					enableCollisions(const bool);


	void					processTouchInfo(ds::ui::Sprite*, const ds::ui::TouchInfo&);
	void					processTouchAdded(const ds::ui::TouchInfo&);
	void					processTouchMoved(const ds::ui::TouchInfo&);
	void					processTouchRemoved(const ds::ui::TouchInfo&);

	// Forces the physics body to this position, may result in non-natural movement. But who cares, right?
	void					setPosition(const ci::vec3&);

	void					clearVelocity();
	void					setLinearVelocity(const float x, const float y);
	ci::vec2				getLinearVelocity();

	void					applyForceToCenter(const float x, const float y);
	void					applyImpulseToCenter(const float x, const float y, ci::vec2 point);

	void					setRotation(const float degree);
	float					getRotation() const;
	// Set a collision callback, called whenever this body
	// collides with another physics object.
	void					setCollisionCallback(const std::function<void(const Collision&)>& fn);

	//  Override the ContactListener 'presolve' method
	//void					setContactPresolveFn(const std::function<void(const b2Contact*, const b2Manifold*)>& fn);
	void					setContactPreSolveFn(const std::function<void( b2Contact*, const b2Manifold*)>& fn);
	void					setContactPostSolveFn(const std::function<void( b2Contact*, const b2ContactImpulse* )>& fn);
	void					setBeginContactFn(const std::function<void(b2Contact*)>& fn);
	void					setEndContactFn(const std::function<void(b2Contact*)>& fn);

	// The sprite owner is responsible for telling me when the
	// center changes. (Only necessary if the fixture is a box).
	void					onCenterChanged();

	// Returns true if the underlying b2World is locked
	// A locked world means that the physics service is
	// in the middle of a time step. it's not safe to
	// call create() or destroy() if world is locked.
	// generally it's not safe at all to manipulate world
	// while it's locked.
	bool					isWorldLocked() const;

	//Get scale of physics world.  Needed for realtime update of joint parameters.  
	//Could wrap the joints, and do this internally, but will be left for a later exercise
	float 					getPhysWorldScale();

	// Runs the world ahead this many iterations (updates)
	void					runWorldAhead(const int iterations);
private:
	friend class BodyBuilder;
	friend class BodyBuilderBox;
	friend class BodyBuilderCircle;
	friend class BodyBuilderPolygon;
	friend class World;
	friend class ds::physics::Touch;

	World&					mWorld;
	ds::ui::Sprite&			mSprite;
	b2Body*					mBody;
};

} // namespace physics
} // namespace ds

#endif // DS_PHYSICS_SPRITEBODY_H_
