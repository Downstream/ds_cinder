#include "ds/physics/sprite_world.h"

#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/sprite_engine.h>
#include "service.h"

namespace ds {
namespace physics {

/**
 * \class SpriteWorld
 */
SpriteWorld::SpriteWorld(ds::ui::Sprite& s, const int world_id) {
	s.getEngine().getService<ds::physics::Service>(SERVICE_NAME).createWorld(s, world_id);
}

} // namespace physics
} // namespace ds
