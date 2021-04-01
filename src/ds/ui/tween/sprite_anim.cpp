#include "stdafx.h"

#include "ds/ui/tween/sprite_anim.h"

#include "ds/ui/sprite/sprite.h"
#include "ds/ui/sprite/sprite_engine.h"
#include "ds/ui/tween/tweenline.h"

#include "ds/util/string_util.h"

namespace ds {
namespace ui {

/**
 * \class SpriteAnimatable
 */
SpriteAnimatable::SpriteAnimatable(Sprite& s, SpriteEngine& e)
	: mOwner(s)
	, mEngine(e)
	, mAnimateOnTargetsSet(false)
	, mAnimateOnScaleTarget(1.0f, 1.0f, 1.0f)
	, mAnimateOnPositionTarget(0.0f, 0.0f, 0.0f)
	, mAnimateOnOpacityTarget(1.0f)
	, mAnimateOnScript("")
	, mAnimateOffScript("")
	, mNormalizedTweenValue(0.0f)
	, mInternalColorCinderTweenRef(nullptr)
	, mInternalScaleCinderTweenRef(nullptr)
	, mInternalRotationCinderTweenRef(nullptr)
	, mInternalPositionCinderTweenRef(nullptr)
	, mInternalSizeCinderTweenRef(nullptr)
	, mInternalOpacityCinderTweenRef(nullptr)
	, mInternalNormalizedCinderTweenRef(nullptr)
	, mAnimScriptCueRef(nullptr)
{}

SpriteAnimatable::~SpriteAnimatable() {
	mInternalColorCinderTweenRef = nullptr;
	mInternalScaleCinderTweenRef = nullptr;
	mInternalRotationCinderTweenRef = nullptr;
	mInternalPositionCinderTweenRef = nullptr;
	mInternalSizeCinderTweenRef = nullptr;
	mInternalOpacityCinderTweenRef = nullptr;
	mInternalNormalizedCinderTweenRef = nullptr;

	// This get's deleted in the Sprite destructor for some reason
	mDelayedCallCueRef = nullptr;

	// Need to explicitly remove to avoid crashes on app-refresh
	if(mAnimScriptCueRef) mAnimScriptCueRef->removeSelf();
	mAnimScriptCueRef = nullptr;

	mMultiDelayedCallCueRefs.clear();
}

const SpriteAnim<ci::Color>& SpriteAnimatable::ANIM_COLOR() {
	static ds::ui::SpriteAnim<ci::Color>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<ci::Color>& { return s.mAnimColor; },
		[](ds::ui::Sprite& s)->ci::Color { return s.getColor(); },
		[](const ci::Color& v, ds::ui::Sprite& s) { s.setColor(v); });
	return ANIM;
}

const SpriteAnim<float>& SpriteAnimatable::ANIM_OPACITY() {
	static ds::ui::SpriteAnim<float>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<float>& { return s.mAnimOpacity; },
		[](ds::ui::Sprite& s)->float { return s.getOpacity(); },
		[](const float& v, ds::ui::Sprite& s) { s.setOpacity(v); });
	return ANIM;
}

const SpriteAnim<ci::vec3>& SpriteAnimatable::ANIM_POSITION() {
	static ds::ui::SpriteAnim<ci::vec3>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<ci::vec3>& { return s.mAnimPosition; },
		[](ds::ui::Sprite& s)->ci::vec3 { return s.getPosition(); },
		[](const ci::vec3& v, ds::ui::Sprite& s) { s.setPosition(v); });
	return ANIM;
}

const SpriteAnim<ci::vec3>& SpriteAnimatable::ANIM_SCALE() {
	static ds::ui::SpriteAnim<ci::vec3>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<ci::vec3>& { return s.mAnimScale; },
		[](ds::ui::Sprite& s)->ci::vec3 { return s.getScale(); },
		[](const ci::vec3& v, ds::ui::Sprite& s) { s.setScale(v); });
	return ANIM;
}

const SpriteAnim<ci::vec3>& SpriteAnimatable::ANIM_SIZE() {
	static ds::ui::SpriteAnim<ci::vec3>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<ci::vec3>& { return s.mAnimSize; },
		[](ds::ui::Sprite& s)->ci::vec3 { return ci::vec3(s.getWidth(), s.getHeight(), s.getDepth()); },
		[](const ci::vec3& v, ds::ui::Sprite& s) { s.setSizeAll(v.x, v.y, v.z); });
	return ANIM;
}

const SpriteAnim<ci::vec3>& SpriteAnimatable::ANIM_ROTATION() {
	static ds::ui::SpriteAnim<ci::vec3>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<ci::vec3>& { return s.mAnimRotation; },
		[](ds::ui::Sprite& s)->ci::vec3 { return s.getRotation(); },
		[](const ci::vec3& v, ds::ui::Sprite& s) { s.setRotation(v); });
	return ANIM;
}

const SpriteAnim<float>& SpriteAnimatable::ANIM_NORMALIZED() {
	static ds::ui::SpriteAnim<float>  ANIM(
		[](ds::ui::Sprite& s)->ci::Anim<float>& { s.mNormalizedTweenValue = 0.0f; return s.mAnimNormalized; },
		[](ds::ui::Sprite& s)->float { return s.mNormalizedTweenValue; },
		[](const float& v, ds::ui::Sprite& s) { s.mNormalizedTweenValue = v; });
	return ANIM;
}

void SpriteAnimatable::tweenColor(const ci::Color& c, const float duration, const float delay,
								  const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animColorStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_COLOR(), c, duration, ease, finishFn, delay, updateFn);
	mInternalColorCinderTweenRef = options.operator ci::TweenRef<ci::Color>();
}

void SpriteAnimatable::tweenOpacity(const float opacity, const float duration, const float delay,
									const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animOpacityStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_OPACITY(), opacity, duration, ease, finishFn, delay, updateFn);
	mInternalOpacityCinderTweenRef = options.operator ci::TweenRef<float>();
}

void SpriteAnimatable::tweenPosition(const ci::vec3& pos, const float duration, const float delay,
									 const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animPositionStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_POSITION(), pos, duration, ease, finishFn, delay, updateFn);
	mInternalPositionCinderTweenRef = options.operator ci::TweenRef<ci::vec3>();
}

void SpriteAnimatable::tweenRotation(const ci::vec3& rot, const float duration, const float delay,
									 const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animRotationStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_ROTATION(), rot, duration, ease, finishFn, delay, updateFn);
	mInternalRotationCinderTweenRef = options.operator ci::TweenRef<ci::vec3>();
}

void SpriteAnimatable::tweenScale(const ci::vec3& scale, const float duration, const float delay,
								  const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animScaleStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_SCALE(), scale, duration, ease, finishFn, delay, updateFn);
	mInternalScaleCinderTweenRef = options.operator ci::TweenRef<ci::vec3>();
}

void SpriteAnimatable::tweenSize(const ci::vec3& size, const float duration, const float delay,
								 const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animSizeStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_SIZE(), size, duration, ease, finishFn, delay, updateFn);
	mInternalSizeCinderTweenRef = options.operator ci::TweenRef<ci::vec3>();
}

void SpriteAnimatable::tweenNormalized(const float duration, const float delay,
									const ci::EaseFn& ease, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {
	animNormalizedStop();
	auto options = mEngine.getTweenline().apply(mOwner, ANIM_NORMALIZED(), 1.0f, duration, ease, finishFn, delay, updateFn);
	mInternalNormalizedCinderTweenRef = options.operator ci::TweenRef<float>();
}


void SpriteAnimatable::completeTweenColor(const bool callFinishFunction){
	if(mInternalColorCinderTweenRef){
		if(getColorTweenIsRunning()){
			mAnimColor.stop();
			mOwner.setColor(mInternalColorCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalColorCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenOpacity(const bool callFinishFunction){
	if(mInternalOpacityCinderTweenRef){
		if(getOpacityTweenIsRunning()){
			mAnimOpacity.stop();
			mOwner.setOpacity(mInternalOpacityCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalOpacityCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenPosition(const bool callFinishFunction){
	if(mInternalPositionCinderTweenRef){
		if(getPositionTweenIsRunning()){
			mAnimPosition.stop();
			mOwner.setPosition(mInternalPositionCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalPositionCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenRotation(const bool callFinishFunction){
	if(mInternalRotationCinderTweenRef){
		if(getRotationTweenIsRunning()){
			mAnimRotation.stop();
			mOwner.setRotation(mInternalRotationCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalRotationCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenScale(const bool callFinishFunction){
	if(mInternalScaleCinderTweenRef){
		if(getScaleTweenIsRunning()){
			mAnimScale.stop();
			mOwner.setScale(mInternalScaleCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalScaleCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenSize(const bool callFinishFunction){
	if(mInternalSizeCinderTweenRef){
		if(getSizeTweenIsRunning()){
			mAnimSize.stop();
			mOwner.setSizeAll(mInternalSizeCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalSizeCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}

void SpriteAnimatable::completeTweenNormalized(const bool callFinishFunction){
	if(mInternalNormalizedCinderTweenRef){
		if(getSizeTweenIsRunning()){
			mAnimSize.stop();
			mNormalizedTweenValue = (mInternalNormalizedCinderTweenRef->getEndValue());
			if(callFinishFunction){
				auto finishFunc = mInternalNormalizedCinderTweenRef->getFinishFn();
				if(finishFunc) finishFunc();
			}
		}
	}
}
const bool SpriteAnimatable::animationRunning(){
	return getOpacityTweenIsRunning() 
		|| getPositionTweenIsRunning() 
		|| getScaleTweenIsRunning() 
		|| getSizeTweenIsRunning() 
		|| getRotationTweenIsRunning() 
		|| getColorTweenIsRunning() 
		|| getNormalizeTweenIsRunning();
}

const bool SpriteAnimatable::getPositionTweenIsRunning(){
	return (mInternalPositionCinderTweenRef && !mAnimPosition.isComplete());
}
const bool SpriteAnimatable::getRotationTweenIsRunning(){
	return (mInternalRotationCinderTweenRef && !mAnimRotation.isComplete());
}
const bool SpriteAnimatable::getSizeTweenIsRunning(){
	return (mInternalSizeCinderTweenRef && !mAnimSize.isComplete());
}
const bool SpriteAnimatable::getScaleTweenIsRunning(){
	return (mInternalScaleCinderTweenRef && !mAnimScale.isComplete());
}
const bool SpriteAnimatable::getOpacityTweenIsRunning(){
	return (mInternalOpacityCinderTweenRef && !mAnimOpacity.isComplete());
}
const bool SpriteAnimatable::getColorTweenIsRunning(){
	return (mInternalColorCinderTweenRef && !mAnimColor.isComplete());
}
const bool SpriteAnimatable::getNormalizeTweenIsRunning(){
	return (mInternalNormalizedCinderTweenRef && !mAnimNormalized.isComplete());
}

void SpriteAnimatable::animStop() {
	animPositionStop();
	animRotationStop();
	animScaleStop();
	animSizeStop();
	animOpacityStop();
	animColorStop();
	animNormalizedStop();

	if (!mMultiDelayedCallCueRefs.empty()) {
		for (auto cue : mMultiDelayedCallCueRefs) {
			if (!cue) continue;
			cue->removeSelf();
			cue = nullptr;
		}
		mMultiDelayedCallCueRefs.clear();
	}
}

void SpriteAnimatable::animPositionStop(){
	mAnimPosition.stop();
	mInternalPositionCinderTweenRef = nullptr;
}

void SpriteAnimatable::animRotationStop(){
	mAnimRotation.stop();
	mInternalRotationCinderTweenRef = nullptr;
}

void SpriteAnimatable::animScaleStop(){
	mAnimScale.stop();
	mInternalScaleCinderTweenRef = nullptr;
}

void SpriteAnimatable::animSizeStop(){
	mAnimSize.stop();
	mInternalSizeCinderTweenRef = nullptr;
}

void SpriteAnimatable::animOpacityStop(){
	mAnimOpacity.stop();
	mInternalOpacityCinderTweenRef = nullptr;
}

void SpriteAnimatable::animColorStop(){
	mAnimColor.stop();
	mInternalColorCinderTweenRef = nullptr;
}

void SpriteAnimatable::animNormalizedStop(){
	mAnimNormalized.stop();
	mInternalNormalizedCinderTweenRef = nullptr;
}

void SpriteAnimatable::completeAllTweens(const bool callFinishFunctions, const bool recursive){
	// Complete all my tweens
	completeTweenColor(callFinishFunctions);
	completeTweenOpacity(callFinishFunctions);
	completeTweenPosition(callFinishFunctions);
	completeTweenRotation(callFinishFunctions);
	completeTweenScale(callFinishFunctions);
	completeTweenSize(callFinishFunctions);
	completeTweenNormalized(callFinishFunctions);

	// Complete my babies tweenies
	if(recursive){
		for(auto child : mOwner.mChildren) {
			if(child) {
				child->completeAllTweens(callFinishFunctions, true);
			}
		}
	}

	// If we have an animateOn/Off callback, clear it or call it
	if(mAnimScriptCueRef){
		if(callFinishFunctions) { mAnimScriptCueRef->getFn()(); }

		mAnimScriptCueRef->removeSelf();
	}
}

float SpriteAnimatable::tweenAnimateOn(const bool recursive, const float delay, const float deltaDelay, const std::function<void(void)>& finishFn){
	if(mAnimScriptCueRef) mAnimScriptCueRef->removeSelf();

	float thisDelay = delay;
	float total = delay;
	total = std::max(total, runAnimationScript(mAnimateOnScript, thisDelay));
	if(recursive){
		for(auto it = begin(mOwner.mChildren); it != end(mOwner.mChildren); ++it) {
			auto child = *it;
			if(child) {
				thisDelay += deltaDelay;
				total = std::max(total, child->tweenAnimateOn(true, thisDelay, deltaDelay));
			}
		}
	}
	if(finishFn){
		auto& timeline = mEngine.getTweenline().getTimeline();
		mAnimScriptCueRef = timeline.add(finishFn, timeline.getCurrentTime()+total);
	}

	return total;
}

void SpriteAnimatable::setAnimateOnScript(const std::string& animateOnScript){
	mAnimateOnScript = animateOnScript;
}

void SpriteAnimatable::setAnimateOnTargets(){
	mAnimateOnTargetsSet = true;
	mAnimateOnScaleTarget = mOwner.getScale();
	mAnimateOnPositionTarget = mOwner.getPosition();
	mAnimateOnOpacityTarget = mOwner.getOpacity();
}

void SpriteAnimatable::setAnimateOnTargetsIfNeeded(){
	if(!mAnimateOnTargetsSet){
		setAnimateOnTargets();
	}
}

void SpriteAnimatable::clearAnimateOnTargets(const bool recursive){
	mAnimateOnTargetsSet = false;
	if(recursive){
		const auto chillins = mOwner.getChildren();
		for(auto child : mOwner.mChildren) {
			if(child) {
				child->clearAnimateOnTargets(recursive);
			}
		}
	}
}

float SpriteAnimatable::tweenAnimateOff(const bool recursive, const float delay, const float deltaDelay, const std::function<void(void)>& finishFn){
	if(mAnimScriptCueRef) mAnimScriptCueRef->removeSelf();

	float thisDelay = delay;
	float total = delay;
	if(recursive){
		for(auto rit = rbegin(mOwner.mChildren); rit != rend(mOwner.mChildren); ++rit) {
			auto child = *rit;
			if(child) {
				total = std::max(total, child->tweenAnimateOff(true, thisDelay, deltaDelay));
				thisDelay += deltaDelay;
			}
		}
	}
	total = std::max(total, runAnimationOffScript(mAnimateOffScript, thisDelay+deltaDelay));

	if(finishFn){
		auto& timeline = mEngine.getTweenline().getTimeline();
		mAnimScriptCueRef = timeline.add(finishFn, timeline.getCurrentTime()+total+0.005f);
	}

	return total;
}

void SpriteAnimatable::setAnimateOffScript(const std::string& animateOffScript){
	mAnimateOffScript = animateOffScript;
}

float SpriteAnimatable::runAnimationScript(const std::string& animScript, const float addedDelay){
	return runReversibleAnimationScript(animScript, addedDelay, false);
}

float SpriteAnimatable::runAnimationOffScript(const std::string& animScript, const float addedDelay){
	return runReversibleAnimationScript(animScript, addedDelay, true);
}

float SpriteAnimatable::runReversibleAnimationScript(const std::string& animScript, const float addedDelay, const bool isReverse){
	if (animScript.empty()) return 0.f;

	// find all the commands in the string
	std::vector<std::string> commands = ds::split(animScript, "; ", true);

	if (commands.empty()) return 0.f;

	// set default parameters, if they're not supplied by the string
	ci::EaseFn easing = ci::EaseInOutCubic();
	float dur = mEngine.getAnimDur();
	float delayey = addedDelay;

	// I'm 98% sure this can never return true.
	// if (!&mOwner)
	// 	return 0.f;
	ci::vec3 currentPos = mOwner.getPosition();

	// This maps tracks all the types (scale, position, etc) and their destinations (as 3d vectors)
	std::map<std::string, ci::vec3> animationCommands;
	for (auto it = commands.begin(); it < commands.end(); ++it){

		// Split commands between the type and the destination
		std::vector<std::string> commandProperties = ds::split((*it), ":", true);
		if (commandProperties.empty() || commandProperties.size() < 1) continue;

		// Parse out the special commands
		std::string keyey = commandProperties.front();
		if (commandProperties.size() > 1) {
			if (keyey == "ease") {
				std::string easeString = commandProperties[1];
				easing = getEasingByString(easeString);
				continue;
			} else if (keyey == "duration") {
				ds::string_to_value<float>(commandProperties[1], dur);
				continue;
			} else if (keyey == "delay") {
				float rootDelay = 0.0f;
				ds::string_to_value<float>(commandProperties[1], rootDelay);
				delayey += rootDelay;
				continue;
			}
		}

		// parse the destination vectors to floats
		ci::vec3 destination = ci::vec3();

		if (commandProperties.size() > 1){
			std::vector<std::string> destinationTokens = ds::split(commandProperties[1], ", ", true);

			ds::string_to_value<float>(destinationTokens[0], destination.x);
			if (destinationTokens.size() > 1){
				ds::string_to_value<float>(destinationTokens[1], destination.y);
			}
			if (destinationTokens.size() > 2){
				ds::string_to_value<float>(destinationTokens[2], destination.z);
			}
		}

		animationCommands[keyey] = destination;
	}

	//apply center first 
	for (auto it = animationCommands.begin(); it != animationCommands.end(); ++it)
	{
		std::string animType = it->first;
		ci::vec3 dest = it->second;
		if (animType == "center"){
			auto currentCenter = mOwner.getCenter();
			if (ci::vec2(currentCenter) == ci::vec2(dest))
				break;

			mOwner.setCenter(dest.x, dest.y);
			mOwner.setPosition((dest.x - currentCenter.x) * mOwner.getScaleWidth() + currentPos.x, (dest.y - currentCenter.y) * mOwner.getScaleHeight() + currentPos.y);
			currentPos = mOwner.getPosition();
		}
	}

	// now that we have all the commands, apply them
	for (auto it = animationCommands.begin(); it != animationCommands.end(); ++it){
		std::string animType = it->first;
		ci::vec3 dest = it->second;
		if (animType == "scale"){
			tweenScale(dest, dur, delayey, easing);
		}
		else if (animType == "opacity"){
			tweenOpacity(dest.x, dur, delayey, easing);
		}
		else if (animType == "position"){
			tweenPosition(dest, dur, delayey, easing);
		}
		else if (animType == "rotation"){
			tweenRotation(dest, dur, delayey, easing);
		}
		else if (animType == "size"){
			tweenSize(dest, dur, delayey, easing);
		}
		else if (animType == "color"){
			tweenColor(ci::Color(dest.x, dest.y, dest.z), dur, delayey, easing);
		}
		else if (animType == "shift"){
			tweenPosition(currentPos + dest, dur, delayey, easing);
		}

		if(!isReverse){
			if (animType == "slide") {
				setAnimateOnTargetsIfNeeded();
				mOwner.setPosition(mAnimateOnPositionTarget + dest);
				tweenPosition(mAnimateOnPositionTarget, dur, delayey, easing);

			}
			else if (animType == "fade") {
				setAnimateOnTargetsIfNeeded();
				if (dest.x == 0.0f) {
					mOwner.setOpacity(0.0f);
				}
				else {
					mOwner.setOpacity(mAnimateOnOpacityTarget + dest.x);
				}
				tweenOpacity(mAnimateOnOpacityTarget, dur, delayey, easing);

			}
			else if (animType == "grow") {
				setAnimateOnTargetsIfNeeded();
				if (dest.x == 0.0f && dest.y == 0.0f) {
					mOwner.setScale(0.0f);
				}
				else {
					mOwner.setScale(mAnimateOnScaleTarget + dest);
				}
				tweenScale(mAnimateOnScaleTarget, dur, delayey, easing);
			}
		}else{
			if (animType == "slide"){
				//setAnimateOnTargetsIfNeeded();
				mOwner.setPosition(mAnimateOnPositionTarget);
				tweenPosition(mAnimateOnPositionTarget + dest, dur, delayey, easing);

			}
			else if (animType == "fade"){
				//setAnimateOnTargetsIfNeeded();
				mOwner.setOpacity(mAnimateOnOpacityTarget);
				if (dest.x == 0.0f){
					tweenOpacity(0.f, dur, delayey, easing);
				}
				else {
					tweenOpacity(mAnimateOnOpacityTarget + dest.x, dur, delayey, easing);
				}

			}
			else if (animType == "grow"){
				//setAnimateOnTargetsIfNeeded();
				mOwner.setScale(mAnimateOnScaleTarget);
				if (dest.x == 0.0f && dest.y == 0.0f){
					tweenScale(ci::vec3( 0.f ), dur, delayey, easing);
				}
				else {
					tweenScale(mAnimateOnScaleTarget + dest, dur, delayey, easing);
				}
			}
		}
	}
	
	return dur+delayey;
}

void SpriteAnimatable::runMultiAnimationScripts(const std::vector<std::string> animScripts, const float gapTime, const float addedDelay /*= 0.0f*/)
{
	if (animScripts.empty()) return;
	
	std::vector<float> durationList, delayList;
	parseMultiScripts(animScripts, durationList, delayList);

	float delay = 0.0f, gap = 0.0f;
	delay =addedDelay;
	gap = gapTime;

	if (!mMultiDelayedCallCueRefs.empty()) {
		for (auto cue : mMultiDelayedCallCueRefs) {
			if (!cue) continue;
			cue->removeSelf();
			cue = nullptr;
		}
		mMultiDelayedCallCueRefs.clear();
	}

	ci::Timeline&		t = mEngine.getTweenline().getTimeline();

	for (size_t i = 0; i < animScripts.size(); i++)
	{
		mMultiDelayedCallCueRefs.push_back(t.add([this, delay, animScripts, i]() {runAnimationScript(animScripts[i]); }, t.getCurrentTime() + delay));
		delay += (durationList[i] + delayList[i] + gap);
	}
}

void SpriteAnimatable::parseMultiScripts(const std::vector<std::string> animScripts, std::vector<float>& durations, std::vector<float>& delays)
{
	if (animScripts.empty()) return;


	for (auto i = 0; i < animScripts.size(); i++)
	{
		// find all the commands in the string
		std::vector<std::string> commands = ds::split(animScripts[i], "; ", true);

		if (commands.empty()) return;

		// set default parameters, if they're not supplied by the string
		ci::EaseFn easing = ci::EaseInOutCubic();
		float dur = 0.35f;
		float delayey = 0.0f;

		// This maps tracks all the types (scale, position, etc) and their destinations (as 3d vectors)
		std::map<std::string, ci::vec3> animationCommands;
		for (auto it = commands.begin(); it < commands.end(); ++it){

			// Split commands between the type and the destination
			std::vector<std::string> commandProperties = ds::split((*it), ":", true);
			if (commandProperties.empty() || commandProperties.size() < 1) continue;

			// Parse out the special commands
			std::string keyey = commandProperties.front();
			if (keyey == "duration"){
				ds::string_to_value<float>(commandProperties[1], dur);
				continue;
			}
			else if (keyey == "delay"){
				ds::string_to_value<float>(commandProperties[1], delayey);
				continue;
			}
		}
		durations.push_back(dur);
		delays.push_back(delayey);
	}
}

ci::EaseFn SpriteAnimatable::getEasingByString(const std::string& inString){
	if(inString == "none") { return ci::easeNone; }
	if(inString == "inQuad") { return ci::easeInQuad; }
	if(inString == "outQuad") { return ci::easeOutQuad; }
	if(inString == "inOutQuad") { return ci::easeInOutQuad; }
	if(inString == "inCubic") { return ci::easeInCubic; }
	if(inString == "outCubic") { return ci::easeOutCubic; }
	if(inString == "inOutCubic") { return ci::easeInOutCubic; }
	if(inString == "inQuart") { return ci::easeInQuart; }
	if(inString == "outQuart") { return ci::easeOutQuart; }
	if(inString == "inOutQuart") { return ci::easeInOutQuart; }
	if(inString == "inQuint") { return ci::easeInQuint; }
	if(inString == "outQuint") { return ci::easeOutQuint; }
	if(inString == "inOutQuint") { return ci::easeInOutQuint; }
	if(inString == "inSine") { return ci::easeInSine; }
	if(inString == "outSine") { return ci::easeOutSine; }
	if(inString == "inOutSine") { return ci::easeInOutSine; }
	if(inString == "inExpo") { return ci::easeInExpo; }
	if(inString == "outExpo") { return ci::easeOutExpo; }
	if(inString == "inOutExpo") { return ci::easeInOutExpo; }
	if(inString == "inCirc") { return ci::easeInCirc; }
	if(inString == "outCirc") { return ci::easeOutCirc; }
	if(inString == "inOutCirc") { return ci::easeInOutCirc; }
	if(inString == "inBounce") { return ci::EaseInBounce(); }
	if(inString == "outBounce") { return ci::EaseOutBounce(); }
	if(inString == "inOutBounce") { return ci::EaseInOutBounce(); }
	if(inString == "inBack") { return ci::EaseInBack(); }
	if(inString == "outBack") { return ci::EaseOutBack(); }
	if(inString == "inOutBack") { return ci::EaseInOutBack(); }
	if(inString == "inAtan") { return ci::EaseInAtan(); }
	if(inString == "outAtan") { return ci::EaseOutAtan(); }
	if(inString == "inOutAtan") { return ci::EaseInOutAtan(); }
	else { return ci::easeNone; }
}

} // namespace ui

} // namespace ds
