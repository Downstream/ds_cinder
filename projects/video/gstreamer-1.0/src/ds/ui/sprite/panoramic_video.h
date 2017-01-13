#ifndef DELOITTE_PRESENTATION_SRC_DRONE_VIDEO_SPRITE_H_
#define DELOITTE_PRESENTATION_SRC_DRONE_VIDEO_SPRITE_H_

#include <ds/ui/sprite/sprite.h>

#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/Camera.h>
#include <cinder/Sphere.h>

#include "ds/ui/sprite/video.h"


namespace ds {
	class Engine;
namespace ui {

class PanoramicVideo : public ds::ui::Sprite
{

public:
	PanoramicVideo(ds::ui::SpriteEngine&);

	void				loadVideo(const std::string& videoPath);
	ds::ui::Video*		getVideo() const;
	void				resetCamera();

	// set the spherical coordinate (in degrees).
	void				setSphericalCoord(float theta, float phi);

	void				lookFront(){	setSphericalCoord(90.0f, 0); }
	void				lookBack(){		setSphericalCoord(-90.0f, 0); }
	void				lookUp(){		setSphericalCoord(0, 0); }
	void				lookDown(){		setSphericalCoord(180.0f, 0); }
	void				lookRight(){	setSphericalCoord(90.0f, -90.0f); }
	void				lookLeft(){		setSphericalCoord(90.0f, 90.0f); }

	float				getTheta() const { return mSphericalAngles.x; }
	float				getPhi() const { return mSphericalAngles.y; }

	// Sets how fast dragging around is. Higher numbers are slower panning, lower numbers are faster
	// Default = 5.0f
	void				setDragParams(const float xSensitivity, const float ySensitivity);
	
	// Sets the drag direction, default x=false, y=true
	void				setDragInvert(const bool xInvert, const bool yInvert);

	void				handleDrag(const ci::vec2& deltaPos);

	void				setFOV(const float fov);

	// Applied to video when video is loaded
	void				setPan(const float audioPan);
	void				setPlayableInstances(const std::vector<std::string> instances);
	void				setAutoSyncronize(const bool doSync); // synchronize across client/server

protected:
	// These are our only chances in client mode to catch the video.
	virtual void		onChildAdded(ds::ui::Sprite& child) override;
	virtual void		onChildRemoved(ds::ui::Sprite& child) override;
	virtual void		onSizeChanged() override;
	virtual void		drawLocalClient() override;
	virtual void		writeAttributesTo(ds::DataBuffer&);
	virtual void		readAttributeFrom(const char attributeId, ds::DataBuffer&);

private:
	ds::ui::Video*		mVideoSprite;
	ci::gl::TextureRef	mVideoTexture;

	ci::gl::GlslProgRef	mShader;
	ci::CameraPersp     mCamera;
	ci::Sphere          mSphere;
	ci::vec2			mSphericalAngles; // x is theta and y is phi

	bool				mInvertX;
	bool				mInvertY;
	float				mXSensitivity;
	float				mYSensitivity;
	float				mFov;

	float				mPanning;
	std::vector<std::string> mPlayableInstances;
	bool				mAutoSync;

public:
	static void			installAsServer(ds::BlobRegistry&);
	static void			installAsClient(ds::BlobRegistry&);
	static void			installSprite(ds::Engine&);

};

}} //!dlpr::view

#endif //!DELOITTE_PRESENTATION_SRC_DRONE_VIDEO_SPRITE_H_
