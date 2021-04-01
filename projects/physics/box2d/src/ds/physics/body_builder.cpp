#include "ds/physics/body_builder.h"

#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "ds/physics/sprite_body.h"
#include "ds/debug/logger.h"
#include "world.h"

namespace ds {
namespace physics {

/**
 * \class BodyBuilder
 */
BodyBuilder::BodyBuilder(const SpriteBody& b)
	: mDensity(1.0f)
	, mFriction(b.mWorld.getFriction())
	, mRestitution(0.5f)
	, mLinearDampening(b.mWorld.getLinearDampening())
	, mAngularDampening(b.mWorld.getAngularDampening())
	, mFixedRotation(b.mWorld.getFixedRotation())
	, mIsStaticBody(false)
	, mIsKinematicBody(false)
	, mAllowSleep(true)
{
	b2Filter		filter;
	mCategoryBits = filter.categoryBits;
	mMaskBits = filter.maskBits;
	mGroupIndex = filter.groupIndex;
}

BodyBuilder::~BodyBuilder()
{
}

/**
 * \class BodyBuilderBox
 */
BodyBuilderBox::BodyBuilderBox(const SpriteBody& sb)
	: BodyBuilder(sb)
{
}

void BodyBuilderBox::createFixture(SpriteBody& body) const
{
	if (!body.mBody) return;

	// Define the dynamic body fixture.
	b2FixtureDef		fixtureDef;
	fixtureDef.density = mDensity;
	fixtureDef.friction = mFriction;
	fixtureDef.restitution = mRestitution;
	fixtureDef.filter.categoryBits = mCategoryBits;
	fixtureDef.filter.maskBits = mMaskBits;
	fixtureDef.filter.groupIndex = mGroupIndex;

	b2PolygonShape	dynamicBox;
	const float32			w = mWidth / 2.0f * body.mWorld.getCi2BoxScale(),
							h = mHeight / 2.0f * body.mWorld.getCi2BoxScale();
	dynamicBox.SetAsBox(w, h);
	fixtureDef.shape = &dynamicBox;
	body.mBody->CreateFixture(&fixtureDef);

}

/**
 * \class BodyBuilderCircle
 */
BodyBuilderCircle::BodyBuilderCircle(const SpriteBody& sb)
	: BodyBuilder(sb)
{
}

void BodyBuilderCircle::createFixture(SpriteBody& body) const
{
	if (!body.mBody) return;

	// Define the dynamic body fixture.
	b2FixtureDef		fixtureDef;
	fixtureDef.density = mDensity;
	fixtureDef.friction = mFriction;
	fixtureDef.restitution = mRestitution;
	fixtureDef.filter.categoryBits = mCategoryBits;
	fixtureDef.filter.maskBits = mMaskBits;
	fixtureDef.filter.groupIndex = mGroupIndex;

	b2CircleShape	circle;
	circle.m_radius = mRadius * body.mWorld.getCi2BoxScale(); 
	fixtureDef.shape = &circle;
	body.mBody->CreateFixture(&fixtureDef);
}


/**
 * \class BodyBuilderPolygon
 */
BodyBuilderPolygon::BodyBuilderPolygon(const SpriteBody& sb)
	: BodyBuilder(sb)
{
}

void BodyBuilderPolygon::createFixture(SpriteBody& body) const
{
	if (!body.mBody) return;

	if(mPoints.empty() || mPoints.size() < 3){
		DS_LOG_ERROR("Not enough points supplied to polygon body builder!");
		return;
	}

	// Define the dynamic body fixture.
	b2FixtureDef		fixtureDef;
	fixtureDef.density = mDensity;
	fixtureDef.friction = mFriction;
	fixtureDef.restitution = mRestitution;
	fixtureDef.filter.categoryBits = mCategoryBits;
	fixtureDef.filter.maskBits = mMaskBits;
	fixtureDef.filter.groupIndex = mGroupIndex;

 	int32 count = (int32)mPoints.size();
 	b2Vec2 * vertices = new b2Vec2[count];
 	int i = 0;
 	for (auto it = mPoints.begin(); it < mPoints.end(); ++it){
 		vertices[i].Set((*it).x* body.mWorld.getCi2BoxScale(), (*it).y* body.mWorld.getCi2BoxScale());
 	//	vertices[i].Set((*it).x, (*it).y);
		i++;
 	}


// 	b2Vec2 vertices[3];
// 	vertices[0].Set(0.0f, 0.0f);
// 	vertices[1].Set(200.0f * body.mWorld.getCi2BoxScale(), 0.0f);
// 	vertices[2].Set(0.0f, 200.0f * body.mWorld.getCi2BoxScale());
// 	int32 count = 3;

	b2PolygonShape	polygon;
	polygon.Set(vertices, count);
	fixtureDef.shape = &polygon;
	body.mBody->CreateFixture(&fixtureDef);

	delete vertices;
}

} // namespace physics
} // namespace ds
