#include "service.h"

#include <ds/app/environment.h>
#include <ds/app/engine/engine.h>
#include <ds/debug/logger.h>
#include "world.h"

namespace ds {
namespace physics {

namespace {
const std::string			_SERVICE_NAME("physics");
}

const std::string&			SERVICE_NAME(_SERVICE_NAME);
const ds::BitMask			PHYSICS_LOG = ds::Logger::newModule("physics");

// NOTE: Ideally we'd initialize the Service as an Engine service in a static here.
// However, because I'm in a library statically linked to the main application, that
// initialization will get tossed, so it has to happen from a class that the app
// will reference. Currently it's happening in SpriteBody.

/**
 * \class ds::physics::Service
 */
Service::Service(ds::ui::SpriteEngine& e)
		: mEngine(e) {
}

void Service::start() {
	ds::Engine*						e(dynamic_cast<ds::Engine*>(&mEngine));
	if(!e){
		DS_LOG_WARNING("No app engine trying to start the physicss service!");
		return;
	}

	ds::cfg::Settings settings;
	ds::Environment::loadSettings("physics", "physics.xml", settings);
	if (settings.getBool("create_root_world", 0, true)) {
		createWorld(e->getRootSprite(), 0);		
	}
}

void Service::stop() {
	mWorlds.clear();
}

void Service::createWorld(ds::ui::Sprite& owner, const int id) {
	auto found = mWorlds.find(id);
	if (found != mWorlds.end()) {
		//delete &found;
		//mWorlds.erase(found);
	}

	std::unique_ptr<World>		up(new World(mEngine, owner));
	World*						w(up.get());
	if (!w) {
		DS_LOG_WARNING("PhysicsService: Can't create physics world");
		return;
	}
	mWorlds[id] = std::move(up);
}

World& Service::getWorld(const int id) {
	if (mWorlds.empty()) {
		throw std::runtime_error("No physics worlds");
	}
	auto found = mWorlds.find(id);
	if (found == mWorlds.end()) {
		throw std::runtime_error("Can't find physics world");
	}
	return *(found->second.get());
}

} // namespace physics
} // namespace ds
