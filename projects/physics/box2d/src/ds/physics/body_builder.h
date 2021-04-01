#pragma once

#include <vector>
#include <cinder/Vector.h>

namespace ds {
namespace physics {
class SpriteBody;

/**
 * \class BodyBuilder
 * \brief Used to supply parameters when creating a body.
 */
class BodyBuilder {
public:
	virtual ~BodyBuilder();

	virtual void		createFixture(SpriteBody&) const = 0;

	float				mDensity,
						mFriction,
						mRestitution,
						mLinearDampening,
						mAngularDampening;
	bool				mFixedRotation;
	bool				mIsStaticBody;
	bool				mIsKinematicBody;
	bool				mAllowSleep;
	int					mCategoryBits;
	int					mMaskBits;
	int					mGroupIndex;

protected:
	BodyBuilder(const SpriteBody&);
};

/**
 * \class BodyBuilderBox
 * \brief Create a box shape.
 */
class BodyBuilderBox : public BodyBuilder {
public:
	BodyBuilderBox(const SpriteBody&);

	virtual void		createFixture(SpriteBody&) const;

	float				mWidth,
						mHeight;
};

/**
 * \class BodyBuilderCircle
 * \brief Create a circle shape.
 */
class BodyBuilderCircle : public BodyBuilder {
public:
	BodyBuilderCircle(const SpriteBody&);

	virtual void		createFixture(SpriteBody&) const;

	float				mRadius;
};


/**
 * \class BodyBuilderPolygon
 * \brief Create an arbitrary polygon. See the box2d manual for specifics on polygon shapes
 */
class BodyBuilderPolygon : public BodyBuilder {
public:
	BodyBuilderPolygon(const SpriteBody&);

	virtual void		createFixture(SpriteBody&) const;

	 std::vector<ci::vec2> mPoints;
};

} // namespace physics
} // namespace ds

