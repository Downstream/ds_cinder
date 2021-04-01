#pragma once

#include <unordered_map>
#include <ds/ui/touch/touch_info.h>
class b2MouseJoint;

namespace ds {
namespace physics {
class SpriteBody;
class World;

/**
 * \class ds::physics::Touch
 * Handle touch processing for a physics world.
 */
class Touch {
public:
	Touch(ds::physics::World&);

	void							processTouchAdded(const SpriteBody&, const ds::ui::TouchInfo&);
	void							processTouchMoved(const SpriteBody&, const ds::ui::TouchInfo&);
	void							processTouchRemoved(const SpriteBody&, const ds::ui::TouchInfo&);

private:
	// Answer a joint for a finger ID
	void							eraseTouch(const int fingerId);
	b2MouseJoint*					getTouchJoint(const int fingerId);
	b2MouseJoint*					getTouchJointFromPtr(const void*);

	ds::physics::World&				mWorld;
	std::unordered_map<int, void*>	mTouchJoints;
};

} // namespace physics
} // namespace ds

