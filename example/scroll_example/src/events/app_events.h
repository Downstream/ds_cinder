#pragma once

#include <ds/app/event.h>

namespace downstream {

class SomethingHappenedEvent : public ds::RegisteredEvent<SomethingHappenedEvent>{};

} // !namespace downstream

