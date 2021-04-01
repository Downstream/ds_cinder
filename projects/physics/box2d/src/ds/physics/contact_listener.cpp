#include "contact_listener.h"

#include <ds/ui/sprite/sprite.h>
#include "ds/physics/collision.h"
#include "world.h"
#include "Box2D/Dynamics/Contacts/b2Contact.h"

namespace ds {
namespace physics {

/**
 * \class ds::physics::ContactListener
 */
ContactListener::ContactListener(World& w)
	: mWorld(w)
	, mPostSolveFn(nullptr)
	, mPreSolveFn(nullptr)
{
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
	if (mPostSolveFn)
		mPostSolveFn(contact, impulse);

	if (!contact || !impulse || !contact->IsEnabled()) return;

	try {
		b2WorldManifold manifold;
		contact->GetWorldManifold(&manifold);

		collide(contact->GetFixtureA(), contact->GetFixtureB(), *impulse, manifold.points[0], manifold.points[1], manifold.normal);
		collide(contact->GetFixtureB(), contact->GetFixtureA(), *impulse, manifold.points[0], manifold.points[1], manifold.normal);
	} catch (std::exception const&) {
	}
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
	if (mPreSolveFn)
		mPreSolveFn(contact, oldManifold);
}

void ContactListener::BeginContact(b2Contact* contact){
	if (mBeginContactFn)
		mBeginContactFn(contact);
}

void ContactListener::EndContact(b2Contact* contact){
	if (mEndContactFn)
		mEndContactFn(contact);
}



void ContactListener::setPreSolveFunction(const std::function<void(b2Contact* contact, const b2Manifold* oldManifold)>& fn) {
	mPreSolveFn = fn;
}

void ContactListener::setPostSolveFunction(const std::function<void(b2Contact* contact, const b2ContactImpulse* impulse)>& fn) {
	mPostSolveFn = fn;
}

void ContactListener::setBeginContactFunction(const std::function<void(b2Contact* contact)>& fn) {
	mBeginContactFn = fn;
}

void ContactListener::setEndContactFunction(const std::function<void(b2Contact* contact)>& fn) {
	mEndContactFn = fn;
}

void ContactListener::setCollisionCallback(const ds::ui::Sprite& s, const std::function<void(const Collision&)>& fn)
{
	// Store a list of all sprites that need to be reported.
	// The alternative is to directly report to the sprite here,
	// but it's possible for the physics to collide the same
	// object many times in one update, so weed that out.
	try {
		if (!fn) {
			auto	found = mRegistered.find(&s);
			if (found != mRegistered.end()) mRegistered.erase(found);
		} else {
			mRegistered[&s] = fn;
		}
	} catch (std::exception const&) {
	}
}

void ContactListener::clear()
{
	mReport.clear();
}

void ContactListener::report()
{
	if (mReport.empty() || mRegistered.empty()) {
		mReport.clear();
		return;
	}

	for (auto it=mReport.begin(), end=mReport.end(); it!=end; ++it) {
		auto found = mRegistered.find(it->mSprite);
		if (found != mRegistered.end() && found->second) {
			Collision		c;
			c.mForce = it->mForce;
			makeCollision(*it, c);
			found->second(c);
		}
	}
	mReport.clear();
}

void ContactListener::collide(const b2Fixture* a, const b2Fixture* b, const b2ContactImpulse& impulse, const b2Vec2 pointOne, const b2Vec2 pointTwo, const b2Vec2 normal)
{
	if (!a || !b) return;

	// A is considered to be my sprite object, b is what I'm colliding with.
	const b2Body*			body = a->GetBody();
	const ds::ui::Sprite*	sprite = reinterpret_cast<ds::ui::Sprite*>(body ? body->GetUserData() : nullptr);
	if (!sprite) return;

	mReport.insert(ContactKey(sprite, b, impulse.normalImpulses[0], pointOne, pointTwo, normal));
}

void ContactListener::makeCollision(const ContactKey& key, Collision& collision) const
{
	collision.mContactOne = mWorld.box2CiTranslation(key.mContactPointOne, nullptr); // nullptr for sprite will make contacts in world space
	collision.mContactTwo = mWorld.box2CiTranslation(key.mContactPointTwo, nullptr); // so assume that all contacts are world space position
	collision.mNormal = ci::vec2(key.mNormal.x, key.mNormal.y);

	// I *think* the sprite in the key can be ignored, because technically it should
	// always be the object receiving the callback (although I bet I have some details
	// to work out when it comes to sprites colliding with each other).
	// That means it's the fixture I care about.
	if (!key.mFixture) return;
	if (mWorld.makeCollision(*key.mFixture, collision)) return;

	const b2Body*			b = key.mFixture->GetBody();
	ds::ui::Sprite*			sprite = reinterpret_cast<ds::ui::Sprite*>(b ? b->GetUserData() : nullptr);
	if (sprite) collision.setToSprite(sprite->getId());
}

} // namespace physics
} // namespace ds
