#include "debug_draw.h"

#include "Box2D/Dynamics/b2World.h"
#include "world.h"
#include <cinder/gl/gl.h>

namespace ds {
namespace physics {

/**
 * \class ds::physics::DebugDraw
 */
DebugDraw::DebugDraw(ds::ui::SpriteEngine& e, b2World& b2w, ds::physics::World &pw)
		: ds::AutoDraw(e)
		, mB2World(b2w)
		, mPhysicsWorld(pw)
{
	mB2World.SetDebugDraw(this);
	SetFlags( b2Draw::e_shapeBit | b2Draw::e_jointBit );
}


DebugDraw::~DebugDraw() {
	mB2World.SetDebugDraw(nullptr);
}

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	ci::gl::color(color.r, color.g, color.b);
	ci::gl::begin(GL_LINE_LOOP);
	for (int32 i = 0; i < vertexCount; ++i)
	{
		ci::gl::vertex(vertices[i].x, vertices[i].y);
	}
	ci::gl::end();
}

void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	DrawPolygon( vertices, vertexCount, color );
}

void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {
	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 theta = 0.0f;
	ci::gl::color(color.r, color.g, color.b);
	ci::gl::begin(GL_LINE_LOOP);
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		ci::gl::vertex(v.x, v.y);
		theta += k_increment;
	}
	ci::gl::end();
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {
	DrawCircle( center, radius, color );
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
	ci::gl::color(color.r, color.g, color.b);
	ci::gl::begin(GL_LINES);
	ci::gl::vertex(p1.x, p1.y);
	ci::gl::vertex(p2.x, p2.y);
	ci::gl::end();
}

void DebugDraw::DrawTransform(const b2Transform& xf) {
	b2Vec2 p1 = xf.p, p2;
	const float32 k_axisScale = 0.4f;
	ci::gl::begin(GL_LINES);

	ci::gl::color(1.0f, 0.0f, 0.0f);
	ci::gl::vertex(p1.x, p1.y);
	p2 = p1 + k_axisScale * xf.q.GetXAxis();
	ci::gl::vertex(p2.x, p2.y);

	ci::gl::color(0.0f, 1.0f, 0.0f);
	ci::gl::vertex(p1.x, p1.y);
	p2 = p1 + k_axisScale * xf.q.GetYAxis();
	ci::gl::vertex(p2.x, p2.y);

	ci::gl::end();
}

void DebugDraw::drawClient(const ci::mat4& t, const DrawParams& p) {
	ci::gl::pushModelView();
	auto trans = t;
	float scale = 1.0f / (mPhysicsWorld.getCi2BoxScale() * 7.07f); // i have no clue why it's 7.07 but that seems to work
	trans = glm::scale(trans, ci::vec3(scale, scale, scale));
	ci::gl::setModelMatrix(trans);
	ci::gl::setViewMatrix(trans);
	mB2World.DrawDebugData();
	ci::gl::popModelView();
}


} // namespace physics
} // namespace ds
