#pragma once

#include <functional>

#include <Box2D/Common/b2Math.h>

class b2Fixture;

namespace ds {
namespace ui {
class Sprite;
}

namespace physics {

/**
 * \class ds::physics::ContactKey
 */
class ContactKey
{
public:
	ContactKey();
	ContactKey(const ds::ui::Sprite*, const b2Fixture*, const float force, const b2Vec2 pointOne, const b2Vec2 pointTwo, const b2Vec2 normal);

	bool					operator==(const ContactKey&) const;
	bool					operator!=(const ContactKey&) const;

	const ds::ui::Sprite*	mSprite;
	const b2Fixture*		mFixture;
	// Note: force and points are ignored in terms equality and hashing.
	const float				mForce;
	const b2Vec2			mContactPointOne;
	const b2Vec2			mContactPointTwo;
	const b2Vec2			mNormal;
};

} // namespace physics
} // namespace ds

// Make the resource ID available for hashing functions
namespace std {
	template<>
	struct hash<ds::physics::ContactKey> : public unary_function<ds::physics::ContactKey, size_t> {
		size_t operator()(const ds::physics::ContactKey& key) const {
			return (size_t)( (size_t)key.mSprite + (size_t)key.mFixture);
		}
	};
}
