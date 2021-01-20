#include "stdafx.h"

#include "sprite_engine.h"
#include "sprite.h"
#include <cinder/app/App.h>
#include "ds/app/engine/engine_data.h"
#include "ds/app/engine/engine_events.h"
#include "ds/app/engine/engine_service.h"
#include "ds/debug/debug_defines.h"
#include "ds/debug/computer_info.h"
#include "ds/ui/soft_keyboard/entry_field.h"
#include "ds/metrics/metrics_service.h"

namespace ds {
namespace ui {

SpriteEngine::SpriteEngine(ds::EngineData& ed, const int appMode)
	: mData(ed)
	, mAppMode(appMode)
	, mRegisteredEntryField(nullptr)
	, mCallbackId(0)
	, mMetricsService(nullptr)
	, mRestartAfterUpdate(false)
{
	mComputerInfo = new ds::ComputerInfo();
}

SpriteEngine::~SpriteEngine(){
	mData.clearServices();
}

ds::EventNotifier& SpriteEngine::getNotifier(){
	return mData.mNotifier;
}

/** \cond Doxygen is having trouble deducing this function so ignore it. */
void SpriteEngine::loadSettings(const std::string& name, const std::string& filename) {
	mData.mEngineCfg.loadSettings(name, filename);
}
/** \endcond */

ds::EngineCfg& SpriteEngine::getEngineCfg() {
	return mData.mEngineCfg;
}

const ds::EngineCfg& SpriteEngine::getEngineCfg() const {
	return mData.mEngineCfg;
}

const ds::ui::TextStyle& SpriteEngine::getTextStyle(const std::string& textName) const {
	return mData.mEngineCfg.getTextStyle(textName);
}

ds::cfg::Settings& SpriteEngine::getSettings(const std::string& name) const {
	return mData.mEngineCfg.getSettings(name);
}

ds::cfg::Settings& SpriteEngine::getEngineSettings() const {
	return mData.mEngineCfg.getSettings("engine");
}

ds::cfg::Settings& SpriteEngine::getAppSettings() const {
	return mData.mEngineCfg.getSettings("app_settings");
}

ds::cfg::Settings& SpriteEngine::getColorSettings() const {
	return mData.mEngineCfg.getSettings("colors");
}

float SpriteEngine::getMinTouchDistance() const {
	return mData.mMinTouchDistance;
}

float SpriteEngine::getMinTapDistance() const {
	return mData.mMinTapDistance;
}

unsigned SpriteEngine::getSwipeQueueSize() const {
	return mData.mSwipeQueueSize;
}

float SpriteEngine::getSwipeMinVelocity() const {
	return mData.mSwipeMinVelocity;
}

float SpriteEngine::getSwipeMaxTime() const {
	return mData.mSwipeMaxTime;
}

float SpriteEngine::getDoubleTapTime() const {
	return mData.mDoubleTapTime;
}

const ci::Rectf& SpriteEngine::getSrcRect() const {
	return mData.mSrcRect;
}

const ci::Rectf& SpriteEngine::getDstRect() const {
	return mData.mDstRect;
}

float SpriteEngine::getWidth() const {
	return mData.mDstRect.getWidth();
}

float SpriteEngine::getHeight() const {
	return mData.mDstRect.getHeight();
}

float SpriteEngine::getWorldWidth() const {
	return mData.mWorldSize.x;
}

float SpriteEngine::getWorldHeight() const {
	return mData.mWorldSize.y;
}

void SpriteEngine::addToDragDestinationList(Sprite *sprite){
	if(!sprite)
		return;

	removeFromDragDestinationList(sprite);

	mDragDestinationSprites.push_back(sprite);
}

void SpriteEngine::removeFromDragDestinationList(Sprite *sprite){
	if(!sprite)
		return;

	auto found = std::find(mDragDestinationSprites.begin(), mDragDestinationSprites.end(), sprite);
	if(found != mDragDestinationSprites.end())
		mDragDestinationSprites.erase(found);
}

Sprite *SpriteEngine::getDragDestinationSprite(const ci::vec3 &globalPoint, Sprite *draggingSprite){
	for(auto it = mDragDestinationSprites.begin(), it2 = mDragDestinationSprites.end(); it != it2; ++it) {
		Sprite *sprite = *it;
		if(sprite == draggingSprite)
			continue;
		if(sprite->contains(globalPoint))
			return sprite;
	}

	return nullptr;
}

float SpriteEngine::getFrameRate() const {
	return mData.mFrameRate;
}

void SpriteEngine::setLayoutTarget(std::string target,int index)
{

	ds::cfg::Settings::Setting& setting = getEngineSettings().getSetting("xml_importer:target", index);
	setting.mRawValue = target;

}
bool SpriteEngine::hasLayoutTarget(std::string target)
{
	if (target.empty()) return false;
	std::regex regex{ "(\\s*,\\s*)" };

	auto end = std::sregex_token_iterator();
	auto target_itr = std::sregex_token_iterator(target.begin(), target.end(), regex, -1);

	while (target_itr != end) {
		auto target_count = getEngineSettings().countSetting("xml_importer:target");
		for (int i = 0; i < target_count; i++)
		{

			auto set_target = getEngineSettings().getString("xml_importer:target", i);
			auto set_itr = std::sregex_token_iterator(set_target.begin(), set_target.end(), regex, -1);
			while (set_itr != end) {
				auto set_value = set_itr->str();
				auto target_value = target_itr->str();
				if (set_value == target_value)
				{
					return true;
				}
				++set_itr;
			}
		}
		target_itr++;
	}
	return false;
}

std::string SpriteEngine::getLayoutTarget(int index)
{
	return getEngineSettings().getString("xml_importer:target", index);
}


const std::string& SpriteEngine::getCmsURL() const {
	return mData.mCmsURL;
}

double SpriteEngine::getElapsedTimeSeconds() const {
	return ci::app::getElapsedSeconds();
}

int SpriteEngine::getIdleTimeout() const {
	return mData.mIdleTimeout;
}

void SpriteEngine::setIdleTimeout(int idleTimeout) {
	mData.mIdleTimeout = idleTimeout;
}

void SpriteEngine::clearFingers(const std::vector<int> &fingers) {
}


ds::ComputerInfo& SpriteEngine::getComputerInfo(){
	return *mComputerInfo;
}


bool SpriteEngine::getMute(){
	return mData.mMute;
}


void SpriteEngine::setMute(bool mute){
	mData.mMute = mute;
}

const std::string SpriteEngine::getAppInstanceName(){
	return mData.mAppInstanceName;
}

bool SpriteEngine::hasService(const std::string& key) const {
	return mData.mServices.find(key) != mData.mServices.cend();
}

ds::EngineService& SpriteEngine::private_getService(const std::string& str) {
	ds::EngineService*	s = mData.mServices[str];
	if(!s) {
		const std::string	msg = "Service (" + str + ") does not exist";
		DS_LOG_FATAL(msg);
	}
	return *s;
}

void SpriteEngine::registerSpriteImporter(const std::string& spriteType, std::function<ds::ui::Sprite*(ds::ui::SpriteEngine&)> func) {
	auto finder = mImporterMap.find(spriteType);
	if(finder != mImporterMap.end()){
		DS_LOG_WARNING("Duplicate sprite importer being added for sprite type: " << spriteType);
	}

	mImporterMap[spriteType] = func;
}

ds::ui::Sprite* SpriteEngine::createSpriteImporter(const std::string& spriteType) {
	auto finder = mImporterMap.find(spriteType);
	if(finder == mImporterMap.end()){
		// Not really an error, since the sprite could be created in another manner
		//DS_LOG_WARNING("No importer found for sprite type " << spriteType);
		return nullptr;
	}

	return finder->second(*this);
}

void SpriteEngine::registerSpritePropertySetter(const std::string& propertyName, std::function<void(ds::ui::Sprite& theSprite, const std::string& theValue, const std::string& fileRefferer)> func){
	auto finder = mPropertyMap.find(propertyName);
	if(finder != mPropertyMap.end()){
		DS_LOG_WARNING("Duplicate sprite property setters registered for property name: " << propertyName);
	}

	mPropertyMap[propertyName] = func;
}


bool SpriteEngine::setRegisteredSpriteProperty(const std::string& propertyName, ds::ui::Sprite& theSprite, const std::string& theValue, const std::string& fileRefferer){
	auto finder = mPropertyMap.find(propertyName);
	if(finder == mPropertyMap.end()){
		return false;
	}

	finder->second(theSprite, theValue, fileRefferer);
	return true;
}

void SpriteEngine::registerEntryField(IEntryField* entryField){	
	mRegisteredEntryField = entryField;
	getNotifier().notify(ds::app::EntryFieldRegisteredEvent());
}

ds::ui::IEntryField* SpriteEngine::getRegisteredEntryField(){
	return mRegisteredEntryField;
}


size_t SpriteEngine::timedCallback(std::function<void()> func, const double timerSeconds) {
	auto theCallback = new ds::time::Callback(*this);
	if(!theCallback) {
		DS_LOG_WARNING("Couldn't create a timed callback! That's a big deal!");
		return 0;
	}
	mTimedCallbacks.emplace_back(theCallback);
	auto wrappedCallback = [this, func, theCallback] {
		func();
		for(auto it = mTimedCallbacks.begin(); it < mTimedCallbacks.end(); it++) {
			if((*it) == theCallback) {
				mTimedCallbacks.erase(it);
				break;
			}
		}
	};

	theCallback->timedCallback(wrappedCallback, timerSeconds);

	return theCallback->getId();
}

size_t SpriteEngine::repeatedCallback(std::function<void()> func, const double timerSeconds) {
	auto theCallback = new ds::time::Callback(*this);
	if(!theCallback) {
		DS_LOG_WARNING("Couldn't create a repeated callback! That's a big deal!");
		return 0;
	}
	mTimedCallbacks.emplace_back(theCallback);
	return theCallback->repeatedCallback(func, timerSeconds);
}

void SpriteEngine::cancelTimedCallback(size_t callbackId) {
	for(auto it = mTimedCallbacks.begin(); it < mTimedCallbacks.end(); it++) {
		if((*it)->getId() == callbackId) {
			(*it)->cancel();
			mTimedCallbacks.erase(it);
			break;
		}
	}
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const std::string& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const int& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const float& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const double& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const ci::vec2& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const ci::vec3& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldName, const ci::Rectf& fieldValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldName, fieldValue);
}

void SpriteEngine::recordMetric(const std::string& metricName, const std::string& fieldNameAndValue) {
	if(mMetricsService) mMetricsService->recordMetric(metricName, fieldNameAndValue);
}

void SpriteEngine::recordMetricString(const std::string& metricName, const std::string& fieldName, const std::string& stringValue) {
	if(mMetricsService) mMetricsService->recordMetricString(metricName, fieldName, stringValue);
}

void SpriteEngine::recordMetricString(const std::string& metricName, const std::string& fieldName, const std::wstring& stringValue) {
	if(mMetricsService) mMetricsService->recordMetricString(metricName, fieldName, stringValue);
}

void SpriteEngine::recordMetricTouch(ds::ui::TouchInfo& ti) {
	if(mMetricsService) mMetricsService->recordMetricTouch(ti);
}

void SpriteEngine::restartAfterNextUpdate() {
	mRestartAfterUpdate = true;
}

bool SpriteEngine::getRestartAfterNextUpdate() {
	bool doRestart = mRestartAfterUpdate;
	mRestartAfterUpdate = false;
	return doRestart;
}

const float SpriteEngine::getAnimDur() const {
	return mData.mAnimDur;
}

void SpriteEngine::setAnimDur(const float newAnimDur) {
	mData.mAnimDur = newAnimDur;
}

} // namespace ui
} // namespace ds
