#pragma once
#include <list>
#include <unordered_map>
#include <memory>

#include <cinder/Camera.h>
#include <cinder/Rect.h>
#include <cinder/Vector.h>
#include <cinder/Xml.h>
#include <cinder/app/Window.h>

#include "ds/app/app_defs.h"
#include "ds/debug/logger.h"
#include "ds/time/time_callback.h"
#include "ds/thread/work_manager.h"
#include "ds/content/content_model.h"
#include "ds/cfg/settings_variables.h"

namespace ds {
class AutoUpdateList;
class ColorList;
class EngineCfg;
class EngineData;
class EngineService;
class EventNotifier;
class FontList;
class PerspCameraParams;
class ResourceList;
class WorkManager;
class ComputerInfo;
class MetricsService;

namespace cfg {
class Settings;
}

class TuioObject;

namespace ui {
class IEntryField;
class LoadImageService;
class PangoFontService;
class Sprite;
class Tweenline;
class TouchEvent;
struct TouchInfo;
struct TextStyle;

/**
 * \class SpriteEngine
 * Interface for the API that is supplied to sprites.
 */
class SpriteEngine {
public:
	/** Access to the app-wide notification service. Use this to send a
	 message to everyone who's registered an EventClient. */
	ds::EventNotifier&				getNotifier();

	/** New-style notifier, access a  named channel. Create the
		 channel if it doesn't exist. */
	virtual ds::EventNotifier&		getChannel(const std::string&) = 0;

	/// General engine services
	virtual ds::WorkManager&		getWorkManager() final { return mWorkManager;	};
	virtual ds::ResourceList&		getResources() = 0;
	virtual const ds::ColorList&	getColors() const = 0;
	virtual const ds::FontList&		getFonts() const = 0;
	virtual ds::AutoUpdateList&		getAutoUpdateList(const int = AutoUpdateType::SERVER) = 0;
	virtual LoadImageService&		getLoadImageService() = 0;
	virtual PangoFontService&		getPangoFontService() = 0;
	virtual Tweenline&				getTweenline() = 0;
	virtual ci::app::WindowRef		getWindow() = 0;

	bool							getMute();
	void							setMute(bool);

	/** Defined by platform:guid. Useful if you need to something specific on a particular client */
	const std::string				getAppInstanceName();

	/** Access a service. Throw if the service doesn't exist.
		Handle casting for you (since the root ds::EngineService class is unuseable). */
	template <typename T>
	T&								getService(const std::string&);
	/// Answers true if the requested service exists and registered with Engine.
	bool							hasService(const std::string&) const;

	/** Access to the current engine configuration info. */
	void							loadSettings(const std::string& name, const std::string& filename);
	
	/// EngineCfg owns all the settings and configs. 
	ds::EngineCfg&					getEngineCfg();
	const ds::EngineCfg&			getEngineCfg() const;

	/// Gets the text style for a particular name (Font name, size, color, leading) from text.xml (deprecated) or styles.xml
	const ds::ui::TextStyle&		getTextStyle(const std::string& textName) const;
	
	/// Returns the settings with this settings name
	ds::cfg::Settings&				getSettings(const std::string& name) const;

	/// Returns the settings for engine.xml (convenience)
	ds::cfg::Settings&				getEngineSettings() const;

	/// Returns the settings for app_settings.xml (convenience)
	ds::cfg::Settings&				getAppSettings() const;

	/// Returns the settings for colors.xml (deprecated)
	ds::cfg::Settings&				getColorSettings() const;

	/// Sprite management
	virtual ds::sprite_id_t			nextSpriteId() = 0;
	virtual void					registerSprite(Sprite&) = 0;
	virtual void					unregisterSprite(Sprite&) = 0;
	virtual Sprite*					findSprite(const ds::sprite_id_t) = 0;
	/// Notification that a sprite has been deleted
	virtual void					spriteDeleted(const ds::sprite_id_t&) = 0;
	virtual ci::Color8u				getUniqueColor() = 0;

	float							getMinTouchDistance() const;
	float							getMinTapDistance() const;
	unsigned						getSwipeQueueSize() const;
	float							getSwipeMinVelocity() const;
	float							getSwipeMaxTime() const;
	float							getDoubleTapTime() const;
	const ci::Rectf&				getSrcRect() const;
	const ci::Rectf&				getDstRect() const;
	float							getWidth() const;
	float							getHeight() const;
	float							getWorldWidth() const;
	float							getWorldHeight() const;
	float							getFrameRate() const;

	/// convenience function for getting and setting the target;

	/** Sets a layout target. */
	void setLayoutTarget(std::string target,int index=0);

	/** Checks if a layout target is set. returns true is set. checking for an empty string always returns false */
	bool hasLayoutTarget(std::string);

	/** Gets a layout target*/
	std::string getLayoutTarget(int index=0);
	
	/// The URL to a content management system, as defined in engine.xml or DS_BASEURL env variable
	const std::string&				getCmsURL() const;

	/// Get the standard animation duration
	const float						getAnimDur() const;
	void							setAnimDur(const float newAnimDur);

	/// Camera control. Will throw if the root at the index is the wrong type.
	/// NOTE: You can't call setPerspectiveCamera() in the app constructor. Call
	/// no earlier than App::setup().
	virtual PerspCameraParams		getPerspectiveCamera(const size_t index) const = 0;
	/// For clients that frequently read the camera params, they can cache a direct reference.
	virtual const ci::CameraPersp&	getPerspectiveCameraRef(const size_t index) const = 0;
	virtual void					setPerspectiveCamera(const size_t index, const PerspCameraParams&) = 0;
	virtual void					setPerspectiveCameraRef(const size_t index, const ci::CameraPersp&) = 0;

	/// Will throw if the root at the index is the wrong type
	virtual float					getOrthoFarPlane(const size_t index) const = 0;
	virtual float					getOrthoNearPlane(const size_t index) const = 0;
	virtual void					setOrthoViewPlanes(const size_t index, const float nearPlane, const float farPlane) = 0;

	void							addToDragDestinationList(Sprite *sprite);
	void							removeFromDragDestinationList(Sprite *sprite);
	Sprite*							getDragDestinationSprite(const ci::vec3 &globalPoint, Sprite *draggingSprite);

	double							getElapsedTimeSeconds() const;

	int								getIdleTimeout() const;
	void							setIdleTimeout(int idleTimeout);
	virtual void					resetIdleTimeout(){};
	virtual void					startIdling(){};
	virtual bool					isIdling() = 0;

	virtual void					clearFingers( const std::vector<int> &fingers );
	virtual void					clearFingersForSprite(ui::Sprite* theSprite) {};
	virtual void					setSpriteForFinger( const int fingerId, ui::Sprite* theSprite ) = 0;
	virtual ui::Sprite*				getSpriteForFinger( const int fingerId ) = 0;

	/// If you want to create touch events from your client app, use these functions.
	/// The touch events will use the same pathways that normal touches would.
	/// This is generally only recommended for debugging stuff (like automators) 
	/// or if you have an unusual input situation (like a kinect or something) and want to use touch
	virtual void					injectTouchesBegin(const ds::ui::TouchEvent&) = 0;
	virtual void					injectTouchesMoved(const ds::ui::TouchEvent&) = 0;
	virtual void					injectTouchesEnded(const ds::ui::TouchEvent&) = 0;

	virtual void					injectObjectsBegin(const ds::TuioObject&) = 0;
	virtual void					injectObjectsMoved(const ds::TuioObject&) = 0;
	virtual void					injectObjectsEnded(const ds::TuioObject&) = 0;

	/// Calls every time any touch anywhere happens, and the touch info is post-translation and filtering
	/// This calls *after* any sprites get the touch. 
	void							setTouchInfoPipeCallback(std::function<void(const ds::ui::TouchInfo&)> func){ mTouchInfoPipe = func; }
	
	/// Get the function for touch info callbacks, for TouchManager to callback on.
	std::function<void(const ds::ui::TouchInfo&)>	getTouchInfoPipeCallback(){ return mTouchInfoPipe; }

	/// Turns on Sprite's setRotateTouches when first created so you can enable rotated touches app-wide by default
	/// Sprites can still turn this off after creation
	virtual bool					getRotateTouchesDefault() = 0;

	/// Get the sprite at the global touch point. NOTE: performance intensive. Use carefully.
	virtual ds::ui::Sprite*			getHit(const ci::vec3& point) = 0;

	virtual	int						getBytesRecieved() = 0;
	virtual int						getBytesSent() = 0;


	static const int				CLIENT_MODE = 0;
	static const int				SERVER_MODE = 1;
	static const int				CLIENTSERVER_MODE = 2;
	static const int				STANDALONE_MODE = 3;
	virtual int						getMode() final {	return mAppMode; };

	ds::ComputerInfo&				getComputerInfo();

	/** Register a function to a sprite type. This allows an xml sprite importer to create sprites it knows nothing about, like Jon Snow. */
	void							registerSpriteImporter(const std::string& spriteType, std::function<ds::ui::Sprite*(ds::ui::SpriteEngine&)> func);
	/** Create a sprite of a type specified by the spriteType name in registerSpriteImporter(). Can return nullptr if there's no sprite registered for that name. */
	ds::ui::Sprite*					createSpriteImporter(const std::string& spriteType);

	/** Register a callback to set the property of a sprite during import by an outside caller (like an xml importer) */
	void							registerSpritePropertySetter(const std::string& propertyName, std::function<void(ds::ui::Sprite& theSprite, const std::string& theValue, const std::string& fileRefferer)> func);

	template<class DERIVED_SPRITE>
	void registerSpritePropertySetter(const std::string& property, std::function<void(DERIVED_SPRITE&, const std::string&, const std::string&)> setter) {
		static_assert(std::is_base_of<ds::ui::Sprite, DERIVED_SPRITE>::value,
			"DERIVED_SPRITE must be derived from ds::ui::Sprite");

		registerSpritePropertySetter(property, [setter, property](ds::ui::Sprite& sp, const std::string& value, const std::string& fileReferrer){
			if (auto derived = dynamic_cast<DERIVED_SPRITE*>(&sp)) {
				setter(*derived, value, fileReferrer);
			}
			else {
				DS_LOG_WARNING("Tried to set the property " << property << " for something other than: " << typeid(DERIVED_SPRITE).name());
			}
		});
	}

	/** Set the property of a sprite by name and value string. File referrer (optional) is the relative file path to look up files. See ds/util/file_meta_data.h for relative path finding */
	bool							setRegisteredSpriteProperty(const std::string& propertyName, ds::ui::Sprite& theSprite, const std::string& theValue, const std::string& fileRefferer = "");

	/// Register an Entry Field so it's able to get normal keyboard input
	void							registerEntryField(IEntryField* entryField);

	/// Returns the Entry Field registered. Returns nullptr if no entry field has been registered
	IEntryField*					getRegisteredEntryField();

	/// Calls the function after the elapsed time once
	/// Save the returned ID if you want to cancel it later
	/// Multiple calls do not cancel previous callbacks, unlike Sprite::delayedCallback()
	size_t							timedCallback(std::function<void()> func, const double timerSeconds);

	/// Calls the function after the elapsed time repeatedly
	/// Save the returned ID if you want to cancel it later
	/// Multiple calls do not cancel previous callbacks, unlike Sprite::delayedCallback()
	size_t							repeatedCallback(std::function<void()> func, const double timerSeconds);

	/// Cancels a timedCallback() or a repeatedCallback() using the return value from above
	void							cancelTimedCallback(size_t callbackId);

	/// Get the service that saves metrics, to easily record multiple types
	MetricsService*					getMetrics() { return mMetricsService; }

	/// Send this metric to telegraf. Requires metrics to be enabled in the engine settings
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const std::string& fieldValue);	
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const int& fieldValue);
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const float& fieldValue);
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const double& fieldValue);
	/// Appends _x and _y to field name to save 2 metrics, one for each part of the vector
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const ci::vec2& fieldValue);
	/// Appends _x, _y and _z to field name to save 3 metrics, one for each part of the vector
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const ci::vec3& fieldValue);
	/// Appends _x, _y, _w, _h to field name to save 4 metrics, one for each part of the rect
	void							recordMetric(const std::string& metricName, const std::string& fieldName, const ci::Rectf& fieldValue);

	/// Combined field and value in the format field0=fieldValue,field1=field1value 
	/// Use this if you're sending multiple fields that should have the same timestamp
	/// You'll need to wrap any string values in quotes
	void							recordMetric(const std::string& metricName, const std::string& fieldNameAndValue);

	/// Wraps the value in quotes
	void							recordMetricString(const std::string& metricName, const std::string& fieldName, const std::string& stringValue);
	void							recordMetricString(const std::string& metricName, const std::string& fieldName, const std::wstring& stringValue);

	/// Saves an input with x, y, fingerid and phase
	void							recordMetricTouch(ds::ui::TouchInfo& ti);

	/// Soft restarts the app on the next update (if this is some kind of server)
	/// This happens outside of the engine update loop to avoid iterator issues
	void							restartAfterNextUpdate();
	/// If this engine has been set to restart soon. Resets the variable to false after calling
	bool							getRestartAfterNextUpdate();

	/// Content delivered by ContentWrangler
	ds::model::ContentModelRef		mContent;

protected:
	/// The data is not copied, so it needs to exist for the life of the SpriteEngine,
	/// which is how things work by default (the data and engine are owned by the App).
	SpriteEngine(ds::EngineData&, const int appMode);
	virtual ~SpriteEngine();

	ds::EngineData&					mData;
	std::list<Sprite *>				mDragDestinationSprites;
	ds::ComputerInfo*				mComputerInfo;
	IEntryField*					mRegisteredEntryField;
	const int						mAppMode;
	WorkManager						mWorkManager;

	ds::MetricsService*				mMetricsService;

	bool							mRestartAfterUpdate;

	std::unordered_map<std::string, std::function<ds::ui::Sprite*(ds::ui::SpriteEngine&)>> mImporterMap;
	std::unordered_map<std::string, std::function<void(ds::ui::Sprite& theSprite, const std::string& theValue, const std::string& fileRefferer)>> mPropertyMap;

	friend class ds::time::Callback;
	std::vector<ds::time::Callback*>	mTimedCallbacks;
	size_t							mCallbackId; // for tracking the above

private:
	ds::EngineService&				private_getService(const std::string&);

	std::function<void(const ds::ui::TouchInfo& ti)>	mTouchInfoPipe;
};

template <typename T>
T& SpriteEngine::getService(const std::string& str) {
	return dynamic_cast<T&>(private_getService(str));
}

} // namespace ui

} // namespace ds

