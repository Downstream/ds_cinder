#include "contact_key.h"

namespace ds {
namespace physics {

/**
 * \class ds::physics::ContactListener
 */
ContactKey::ContactKey()
	: mSprite(nullptr)
	, mFixture(nullptr)
	, mForce(0.0f)
{
}

ContactKey::ContactKey(const ds::ui::Sprite* s, const b2Fixture* f, const float force, const b2Vec2 pointUno, const b2Vec2 pointNino, const b2Vec2 normal)
	: mSprite(s)
	, mFixture(f)
	, mForce(force)
	, mContactPointOne(pointUno)
	, mContactPointTwo(pointNino)
	, mNormal(normal)
{
}

bool ContactKey::operator==(const ContactKey& o) const
{
	return mSprite == o.mSprite && mFixture == o.mFixture;
}

bool ContactKey::operator!=(const ContactKey& o) const
{
	return mSprite != o.mSprite || mFixture != o.mFixture;
}

} // namespace physics
} // namespace ds
