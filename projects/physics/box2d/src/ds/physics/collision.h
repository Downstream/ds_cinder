#pragma once

#include <ds/app/app_defs.h>

namespace ds {
namespace physics {

/**
 * \class Collision
 * \brief Information about a collision.
 */
class Collision {
public:
	// Information about what I collided with.
	enum Type { EMPTY, WORLD_BOUNDS, SPRITE };
	enum Edge { LEFT, TOP, RIGHT, BOTTOM };

public:
	Collision();

	void			setToWorldBounds(const Edge);
	void			setToSprite(const ds::sprite_id_t);

	// Amount of force at this collision
	float			mForce;

	Type			mType;
	// If type is WORLD_BOUNDS
	Edge			mEdge;
	// If type is SPRITE
	ds::sprite_id_t	mSpriteId;

	// Contact positions are in world space
	ci::vec3		mContactOne;
	ci::vec3		mContactTwo;
	ci::vec2		mNormal;
};

} // namespace physics
} // namespace ds

