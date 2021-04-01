#pragma once

#include <unordered_map>
#include <ds/app/engine/engine_service.h>
#include <ds/cfg/settings.h>
#include <ds/util/bit_mask.h>

namespace ds {
namespace ui {
class Sprite;
class SpriteEngine;
} // namespace ui
namespace physics {
class World;

extern const std::string&		SERVICE_NAME;
extern const ds::BitMask		PHYSICS_LOG;

/**
 * \class ds::physics::Service
 * A service that stores all the physics worlds.
 */
class Service : public ds::EngineService {
public:
	Service(ds::ui::SpriteEngine&);

	virtual void				start();
	virtual void				stop();

	void						createWorld(ds::ui::Sprite&, const int id);
	World&						getWorld(const int id);

private:
	ds::ui::SpriteEngine&		mEngine;
	std::unordered_map<int, std::unique_ptr<World>>	mWorlds;
};

} // namespace physics
} // namespace ds

