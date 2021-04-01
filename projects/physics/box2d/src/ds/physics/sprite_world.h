#pragma once

namespace ds {
namespace ui {
class Sprite;
}
namespace physics {

/**
 * \class SpriteWorld
 * \brief Construct a physics world attached to a sprite.
 * NOTE: Right now the sprite does not own the world, so if
 * the sprite gets deleted, CRASH.
 */
class SpriteWorld {
public:
	SpriteWorld(ds::ui::Sprite&, const int world_id = 0);

private:
};

} // namespace physics
} // namespace ds

