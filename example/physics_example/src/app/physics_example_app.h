#pragma once

#include <cinder/app/App.h>
#include <ds/app/app.h>
#include <ds/app/event_client.h>


namespace physics {
class AllData;

class physics_example_app : public ds::App {
public:
	physics_example_app();

	void				setupServer();

};

} 
