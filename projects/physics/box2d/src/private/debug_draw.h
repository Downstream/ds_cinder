#pragma once
#ifndef DS_PHYSICS_PRIVATE_DEBUGDRAW_H_
#define DS_PHYSICS_PRIVATE_DEBUGDRAW_H_

#include "Box2D/Common/b2Draw.h"
#include <ds/app/auto_draw.h>
class b2World;

namespace ds {
namespace physics {

class World;

/**
 * \class ds::physics::DebugDraw
 */
class DebugDraw : public b2Draw
				, public ds::AutoDraw {
public:
	DebugDraw(ds::ui::SpriteEngine&, b2World&, ds::physics::World &);
	~DebugDraw();

	virtual void		DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	virtual void		DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	virtual void		DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
	virtual void		DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	virtual void		DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	virtual void		DrawTransform(const b2Transform& xf);

protected:
	virtual void		drawClient(const ci::mat4&, const DrawParams&);

private:
	ds::physics::World	&mPhysicsWorld;
	b2World				&mB2World;
};

} // namespace physics
} // namespace ds

#endif
