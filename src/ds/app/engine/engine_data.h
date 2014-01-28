#pragma once
#ifndef DS_APP_ENGINE_ENGINEDATA_H_
#define DS_APP_ENGINE_ENGINEDATA_H_

#include <unordered_map>
#include <cinder/Rect.h>
#include "ds/app/event_notifier.h"
#include "ds/app/engine/engine_cfg.h"

namespace ds
{
class EngineService;

/**
 * \class ds::EngineData
 * \brief Store all the data for an engine. Primarily a
 * programmer convenience, to hide and group info.
 */
class EngineData
{
public:
	EngineData();

	EventNotifier			mNotifier;
	std::unordered_map<std::string, ds::EngineService*>
							mServices;

	// Will stop and delete them.
	void					clearServices();

	ds::EngineCfg			mEngineCfg;

	float					mMinTouchDistance;
	float					mMinTapDistance;
	int						mSwipeQueueSize;
	float					mDoubleTapTime;
	ci::Rectf				mScreenRect;
	ci::Vec2f				mWorldSize;
	float					mFrameRate;
	// Used by the app to tell the engine the camera has changed
	bool					mCameraDirty;

private:
	EngineData(const EngineData&);
	EngineData&				operator=(const EngineData&);
};

} // namespace ds

#endif // DS_APP_ENGINE_ENGINEDATA_H_
