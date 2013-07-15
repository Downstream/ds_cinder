#include "ds/physics/sprite_body.h"

#include <ds/app/app.h>
#include "ds/physics/body_builder.h"
#include "private/world.h"
#include <ds/ui/sprite/sprite_engine.h>
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Dynamics/b2World.h"

namespace ds {
namespace physics {

namespace {

// Statically initialize the world class. Done here because the Body is
// guaranteed to be referenced by the final application.
class Init {
public:
	Init() {
		ds::App::AddStartup([](ds::Engine& e) {
			ds::physics::World*		w = new ds::physics::World(e);
			if (!w) throw std::runtime_error("Can't create ds::physics::World");
			e.addService("physics", *w);
		});

	}

	void			doNothing() { }
};
Init				INIT;

}

/**
 * \class ds::physics::SpriteBody
 */
SpriteBody::SpriteBody(ds::ui::Sprite& s)
	: mWorld(s.getEngine().getService<ds::physics::World>("physics"))
	, mSprite(s)
	, mBody(nullptr)
{
	// This shouldn't be necessary, but I just want to make sure the static is referenced.
	INIT.doNothing();

	s.enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	s.setProcessTouchCallback([this](ds::ui::Sprite* s, const ds::ui::TouchInfo& ti) { this->processTouchInfo(s, ti); });
}

SpriteBody::~SpriteBody()
{
	destroy();
}

void SpriteBody::create(const BodyBuilder& b)
{
	destroy();

	b2BodyDef			def;
	def.type = b2_dynamicBody;
	def.userData = &mSprite;
	def.position = mWorld.Ci2BoxTranslation(mSprite.getPosition());
	def.linearDamping = b.mLinearDampening;
	def.angularDamping = b.mAngularDampening;
	def.fixedRotation = b.mFixedRotation;

	mBody = mWorld.mWorld->CreateBody(&def);
	if (!mBody) return;

	b.createFixture(*this);
}

void SpriteBody::destroy()
{
	if (mBody != nullptr) {
		mWorld.mWorld->DestroyBody(mBody);
		mBody = nullptr;
	}
}

void SpriteBody::setLinearVelocity(const float x, const float y)
{
	if (mBody != nullptr) {
		mBody->SetLinearVelocity(b2Vec2(x, y));
	}
}

void SpriteBody::processTouchInfo(ds::ui::Sprite*, const ds::ui::TouchInfo& ti)
{
	mWorld.processTouchInfo(*this, ti);
}

void SpriteBody::onCenterChanged()
{
	if (mBody == nullptr) return;

	// Currently there should only be 1 fixture.
	b2Fixture*				fix = mBody->GetFixtureList();
	while (fix) {
		b2PolygonShape*		poly = dynamic_cast<b2PolygonShape*>(fix->GetShape());
		if (poly) {
//			void SetAsBox(float32 hx, float32 hy, const b2Vec2& center, float32 angle);
			const float32	w = mSprite.getWidth() / 2.0f * mWorld.getCi2BoxScale(),
							h = mSprite.getHeight() / 2.0f * mWorld.getCi2BoxScale();
			// Convert the sprite center into a box2d center.
			const ci::Vec2f	cen((mSprite.getCenter().x * 2) - 1.0f,
								(mSprite.getCenter().y * 2) - 1.0f);
			b2Vec2			box_cen;
			box_cen.x = cen.x * w;
			box_cen.y = cen.y * h;
			poly->SetAsBox(w, h, box_cen, 0.0f);
	std::cout << "POLY w=" << w << " h=" << h << " cen=" << box_cen.x << "," << box_cen.y << std::endl;

			return;
		}
		fix = fix->GetNext();
	}
}

} // namespace physics
} // namespace ds
