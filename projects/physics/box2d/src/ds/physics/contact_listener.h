#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "Box2D/Dynamics/b2WorldCallbacks.h"
#include "contact_key.h"

namespace ds {
namespace ui {
class Sprite;
}

namespace physics {
class Collision;
class World;

/**
 * \class ds::physics::ContactListener
 * \brief Report collisions between bodies.
 */
class ContactListener : public b2ContactListener
{
public:
	ContactListener(World&);

	virtual void		PostSolve(b2Contact*, const b2ContactImpulse*);

	//Define local presolve behavior
	virtual void		PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
	/// Called when two fixtures begin to touch.
	virtual void BeginContact(b2Contact* contact);

	/// Called when two fixtures cease to touch.
	virtual void EndContact(b2Contact* contact);



	void				setPreSolveFunction(const std::function<void(b2Contact* contact, const b2Manifold* oldManifold)>& fn);
	void				setPostSolveFunction(const std::function<void(b2Contact* contact, const b2ContactImpulse* impulse)>& fn);
	void				setBeginContactFunction(const std::function<void(b2Contact* contact)>& fn);
	void				setEndContactFunction(const std::function<void(b2Contact* contact)>& fn);

	// Either add or remove, depending on if the function is valid.
	void				setCollisionCallback(const ds::ui::Sprite&, const std::function<void(const Collision&)>& fn);

	// In each update, first clear me, then report after running
	// the physics simulation.
	void				clear();
	void				report();

private:
	void				collide(const b2Fixture* a, const b2Fixture* b, const b2ContactImpulse&, const b2Vec2 pointOne, const b2Vec2 pointTwo, const b2Vec2 normal);
	void				makeCollision(const ContactKey&, Collision&) const;

	World&				mWorld;

	// The collection of all sprites registered with me.
	std::unordered_map<const ds::ui::Sprite*, std::function<void(const Collision&)>>
						mRegistered;
	// The list of sprites to report on in the current update.
	std::unordered_set<ContactKey>
						mReport;

	//preSolve function call
	std::function<void(b2Contact*, const b2Manifold*)>	mPreSolveFn;
	std::function<void(b2Contact*, const b2ContactImpulse*)> mPostSolveFn;
	std::function<void(b2Contact*)> mBeginContactFn;
	std::function<void(b2Contact*)> mEndContactFn;
};

} // namespace physics
} // namespace ds

