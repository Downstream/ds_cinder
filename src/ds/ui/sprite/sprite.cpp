#include "stdafx.h"

#include "sprite.h"
#include <cinder/Camera.h>
#include <cinder/gl/gl.h>
#include "ds/app/blob_reader.h"
#include "ds/app/blob_registry.h"
#include "ds/app/camera_utils.h"
#include "ds/app/environment.h"
#include "ds/data/data_buffer.h"
#include "ds/debug/logger.h"
#include "ds/debug/debug_defines.h"
#include "ds/math/math_defs.h"
#include "ds/math/math_func.h"
#include "ds/ui/sprite/sprite_engine.h"
#include "ds/ui/tween/tweenline.h"
#include "ds/util/string_util.h"
#include "util/clip_plane.h"
#include "ds/params/draw_params.h"

#include "cinder/ImageIo.h"
#include <cinder/Ray.h>
#include <cinder/Rand.h>

//#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ds/cfg/settings_variables.h>

#pragma warning (disable : 4355)    // disable 'this': used in base member initializer list

namespace ds {
namespace ui {

const char          SPRITE_ID_ATTRIBUTE = 1;

namespace {
char                BLOB_TYPE         = 0;

const DirtyState	ID_DIRTY			= newUniqueDirtyState();
const DirtyState	PARENT_DIRTY		= newUniqueDirtyState();
const DirtyState	CHILD_DIRTY			= newUniqueDirtyState();
const DirtyState	FLAGS_DIRTY 	   	= newUniqueDirtyState();
const DirtyState	SIZE_DIRTY 	    	= newUniqueDirtyState();
const DirtyState	POSITION_DIRTY		= newUniqueDirtyState();
const DirtyState	CHECKBOUNDS_DIRTY	= newUniqueDirtyState();
const DirtyState	CENTER_DIRTY		= newUniqueDirtyState();
const DirtyState	SCALE_DIRTY			= newUniqueDirtyState();
const DirtyState	COLOR_DIRTY			= newUniqueDirtyState();
const DirtyState	OPACITY_DIRTY		= newUniqueDirtyState();
const DirtyState	BLEND_MODE			= newUniqueDirtyState();
const DirtyState	CLIPPING_BOUNDS		= newUniqueDirtyState();
const DirtyState	SORTORDER_DIRTY		= newUniqueDirtyState();
const DirtyState	ROTATION_DIRTY		= newUniqueDirtyState();
const DirtyState	CORNER_DIRTY		= newUniqueDirtyState();

const char			PARENT_ATT			= 2;
const char			SIZE_ATT			= 3;
const char			FLAGS_ATT			= 4;
const char			POSITION_ATT		= 5;
const char			CENTER_ATT			= 6;
const char			SCALE_ATT			= 7;
const char			COLOR_ATT			= 8;
const char			OPACITY_ATT			= 9;
const char			BLEND_ATT			= 10;
const char			CLIP_BOUNDS_ATT		= 11;
const char			SORTORDER_ATT		= 12;
const char			ROTATION_ATT		= 13;
const char			CHECKBOUNDS_ATT		= 14;
const char			CORNERRADIUS_ATT	= 15;

// flags
const int			VISIBLE_F			= (1<<0);
const int			TRANSPARENT_F = (1 << 1);
const int			ENABLED_F = (1 << 2);
const int			DRAW_SORTED_F = (1 << 3);
const int			CLIP_F = (1 << 4);
const int			SHADER_CHILDREN_F = (1 << 5);
const int			NO_REPLICATION_F = (1 << 6);
const int			ROTATE_TOUCHES_F = (1 << 7);
const int			DRAW_DEBUG_F		= (1<<8);

const ds::BitMask	SPRITE_LOG = ds::Logger::newModule("sprite");
}

void Sprite::installAsServer(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r); });
}

void Sprite::installAsClient(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<Sprite>(r); });
}

void Sprite::handleBlobFromClient(ds::BlobReader& r) {
	ds::DataBuffer&		buf(r.mDataBuffer);
	if(buf.read<char>() != SPRITE_ID_ATTRIBUTE) return;
	ds::sprite_id_t		id = buf.read<ds::sprite_id_t>();
	Sprite*				s = r.mSpriteEngine.findSprite(id);
	if(s) s->readFrom(r);
}

Sprite::Sprite(SpriteEngine& engine, float width /*= 0.0f*/, float height /*= 0.0f*/)
	: SpriteAnimatable(*this, engine)
	, mEngine(engine)
	, mId(ds::EMPTY_SPRITE_ID)
	, mParent(nullptr)
	, mWidth(width)
	, mHeight(height)
	, mTouchProcess(engine, *this)
	, mSpriteShader(Environment::getAppFolder("data/shaders"), "base")
	, mIdleTimer(engine)
	, mLastWidth(width)
	, mLastHeight(height)
	, mLastDepth(1.f)
	, mPerspective(false)
	, mUseDepthBuffer(false)
	, mShaderTexture(nullptr)
{
	init(mEngine.nextSpriteId());
	setSize(width, height);
}

Sprite::Sprite(SpriteEngine& engine, const ds::sprite_id_t id, const bool perspective)
	: SpriteAnimatable(*this, engine)
	, mEngine(engine)
	, mId(ds::EMPTY_SPRITE_ID)
	, mParent(nullptr)
	, mTouchProcess(engine, *this)
	, mSpriteShader(Environment::getAppFolder("data/shaders"), "base")
	, mIdleTimer(engine)
	, mLastWidth(0.f)
	, mLastHeight(0.f)
	, mLastDepth(1.f)
	, mPerspective(perspective)
	, mUseDepthBuffer(false)
	, mShaderTexture(nullptr)
{
	init(id);
}

void Sprite::init(const ds::sprite_id_t id) {
	mSpriteFlags = VISIBLE_F | TRANSPARENT_F;
	mWidth = 0.f;
	mHeight = 0.f;
	mDepth = 1.f;
	mCenter = ci::vec3(0.0f, 0.0f, 0.0f);
	mRotation = ci::vec3(0.0f, 0.0f, 0.0f);
	mRotationOrderZYX = false;
	mScale = ci::vec3(1.0f, 1.0f, 1.0f);
	mUpdateTransform = true;
	mParent = nullptr;
	mOpacity = 1.0f;
	mColor = ci::Color(1.0f, 1.0f, 1.0f);
	mMultiTouchEnabled = false;
	mCheckBounds = false;
	mBoundsNeedChecking = true;
	mInBounds = true;
	mDragDestination = nullptr;
	mBlobType = BLOB_TYPE;
	mBlendMode = NORMAL;
	mUseShaderTexture = false;
	mIsInScreenCoordsHack = false;
	mTouchScaleSizeMode = false;
	mCornerRadius = 0.0f;
	mDrawOpacity = 1.0f;
	mDelayedCallCueRef = nullptr;
	mLayoutFixedAspect = false;
	mShaderTexture = nullptr;
	mCachedRender = false;
	mCachedRenderNeedsRefresh = false;
	mNeedsBatchUpdate = false;
	mDoSpecialRotation = false;
	mDegree = 0.0f;

	mLayoutBPad = 0.0f;
	mLayoutTPad = 0.0f;
	mLayoutRPad = 0.0f;
	mLayoutLPad = 0.0f;
	mLayoutFudge = ci::vec3();
	mLayoutSize = ci::vec2();
	mLayoutHAlign = 0;
	mLayoutVAlign = 0;
	mLayoutUserType = 0;

	mExportWithXml = true;

	if(mEngine.getRotateTouchesDefault()){
		setRotateTouches(true);
	}

	setSpriteId(id);

	mServerColor = ci::ColorA(static_cast<float>(ci::randFloat(0.5, 1.0f)),
							  static_cast<float>(ci::randFloat(0.5, 1.0f)),
							  static_cast<float>(ci::randFloat(0.5, 1.0f)),
							  0.4f);
	mClippingBounds.set(0.0f, 0.0f, 0.0f, 0.0f);
	mClippingBoundsDirty = false;
	mOutputFbo = nullptr;
	mIsRenderFinalToTexture = false;

	dimensionalStateChanged();
}

Sprite::~Sprite() {
	animStop();
	cancelDelayedCall();

	mEngine.removeFromDragDestinationList(this);

	// We only want to request a delete for the sprite at the head of a tree,
	const sprite_id_t	id = mId;
	setSpriteId(ds::EMPTY_SPRITE_ID);

	for(auto it = mChildren.begin(), end = mChildren.end(); it != end; ++it) {
		(*it)->mParent = nullptr;
		// Make sure the destructor doesn't ask the engine to delete
		(*it)->setSpriteId(ds::EMPTY_SPRITE_ID);
		delete (*it);
	}
	mChildren.clear();

	if(id != ds::EMPTY_SPRITE_ID) {
		if((mSpriteFlags&NO_REPLICATION_F) == 0) {
			mEngine.spriteDeleted(id);
		}
	}
}

void Sprite::updateClient(const UpdateParams &p) {
	mIdleTimer.update();

	if(mCheckBounds) {
		updateCheckBounds();
	}

	for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
		(*it)->updateClient(p);
	}

	onUpdateClient(p);
}

void Sprite::updateServer(const UpdateParams &p) {
	mTouchProcess.update(p);

	mIdleTimer.update();

	if(mCheckBounds) {
		updateCheckBounds();
	}

	for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
		(*it)->updateServer(p);
	}

	onUpdateServer(p);
}

void Sprite::drawClient(const ci::mat4 &trans, const DrawParams &drawParams) {
	if ((mSpriteFlags&VISIBLE_F) == 0) {
		return;
	}
	DS_REPORT_GL_ERRORS();

	buildTransform();
	ci::mat4 totalTransformation = mTransformation;
	
	// If rendering to an FBO, no need to get the total transformation. Otherwise combine existing
	// transform with the local transform
	if (!mIsRenderFinalToTexture){
		totalTransformation = trans * totalTransformation;
	}

	if (mCachedRender && !mCachedRenderNeedsRefresh && getFinalOutTexture()) {
		ci::gl::pushModelMatrix();
		ci::gl::multModelMatrix(totalTransformation);
		ci::gl::enableAlphaBlending();
		applyBlendingMode(mBlendMode);
		ci::gl::GlslProgRef shaderBase = mSpriteShader.getShader();
		if (shaderBase) {
			DS_REPORT_GL_ERRORS();
			shaderBase->bind();
			DS_REPORT_GL_ERRORS();
			shaderBase->uniform("tex0", 0);
			shaderBase->uniform("useTexture", mUseShaderTexture);
			shaderBase->uniform("preMultiply", premultiplyAlpha(mBlendMode));

			int uniformLoc = 0;
			if (shaderBase->findUniform("extent", &uniformLoc)) {
				shaderBase->uniform("extent", ci::vec2(getWidth(), getHeight()));
			}
			if (shaderBase->findUniform("extra", &uniformLoc)) {
				shaderBase->uniform("extra", mShaderExtraData);
			}

			mUniform.applyTo(shaderBase);
			clip_plane::passClipPlanesToShader(shaderBase);
		}
		ci::gl::draw(getFinalOutTexture());
		ci::gl::popModelMatrix();
		return;
	}

	// Local anonymous lambda for the common aspects of drawing. Called differently below depending
	// on the mIsRenderFinalToTexture state
	auto renderInternal = [&](){
		// ci::gl::ScopedModelMatrix sMm;//{ totalTransformation };
		ci::gl::pushModelMatrix();
		ci::gl::multModelMatrix(totalTransformation);
		mSpriteShader.loadShaders();

		if ((mSpriteFlags&TRANSPARENT_F) == 0) {

			buildRenderBatch();

			DS_REPORT_GL_ERRORS();
			ci::gl::enableAlphaBlending();
			applyBlendingMode(mBlendMode);
			ci::gl::GlslProgRef shaderBase = mSpriteShader.getShader();
			if (shaderBase) {
				DS_REPORT_GL_ERRORS();
				shaderBase->bind();
				DS_REPORT_GL_ERRORS();
				shaderBase->uniform("tex0", 0);

				int uniformLoc = 0;

				if (shaderBase->findUniform("useTexture", &uniformLoc)) {
					shaderBase->uniform("useTexture", mUseShaderTexture);
				}
				if (shaderBase->findUniform("preMultiply", &uniformLoc)) {
					shaderBase->uniform("preMultiply", premultiplyAlpha(mBlendMode));
				}
				if(shaderBase->findUniform("extent", &uniformLoc)){
					shaderBase->uniform("extent", ci::vec2(getWidth(), getHeight()));
				}
				if(shaderBase->findUniform("extra", &uniformLoc)){
					shaderBase->uniform("extra", mShaderExtraData);
				}

				mUniform.applyTo(shaderBase);
				clip_plane::passClipPlanesToShader(shaderBase);
			}

			DS_REPORT_GL_ERRORS();

			mDrawOpacity = mOpacity * drawParams.mParentOpacity;

			ci::gl::color(mColor.r, mColor.g, mColor.b, mDrawOpacity);

			if (mUseDepthBuffer) {
				ci::gl::enableDepthRead();
				ci::gl::enableDepthWrite();
			} else {
				ci::gl::disableDepthRead();
				ci::gl::disableDepthWrite();
			}

			DS_REPORT_GL_ERRORS();
			drawLocalClient();
			DS_REPORT_GL_ERRORS();
		}	


		if (!mIsRenderFinalToTexture && (mSpriteFlags&CLIP_F) != 0){ // Clipping is implicit when rendering to an FBO, only set clipping if we aren't
			const ci::Rectf&	  clippingBounds = getClippingBounds();
			clip_plane::enableClipping(clippingBounds.getX1(), clippingBounds.getY1(), clippingBounds.getX2(), clippingBounds.getY2());
		}

		ci::gl::popModelMatrix();
		DS_REPORT_GL_ERRORS();


		DrawParams dParams = drawParams;
		dParams.mParentOpacity *= mOpacity;

		if((mSpriteFlags&DRAW_SORTED_F) == 0) {
			for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
				(*it)->drawClient(totalTransformation, dParams);
			}
		} else {
			makeSortedChildren();
			for(auto it = mSortedTmp.begin(), it2 = mSortedTmp.end(); it != it2; ++it) {
				(*it)->drawClient(totalTransformation, dParams);
			}
		}
	};

	if(mIsRenderFinalToTexture && mOutputFbo){
		// set the viewport and maticies to match the w/h of this object and fbo
		ci::CameraOrtho camera = ci::CameraOrtho(0.0f, getWidth(), getHeight(), 0.0f, -1000.0f, 1000.0f);
		ci::gl::ScopedMatrices sMat;
		ci::gl::ScopedViewport sVp(ci::ivec2(0), mOutputFbo->getSize());
		ci::gl::ScopedFramebuffer sFb(mOutputFbo);
		ci::gl::setMatrices(camera);

		//Need to manual flip image.  The flip() function of the ci::Texture element doesn't work with shaders.
		//ci::gl::scale(1.0f, -1.0f, 1.0f);							// invert Y axis so increasing Y goes down.
		//ci::gl::translate(0.0f, (float)-getHeight(), 0.0f);			// shift origin up to upper-left corner.
		ci::gl::clear(ci::ColorA(0.f, 0.f, 0.f, 0.f), true);
		renderInternal();

		if(mCachedRender && mCachedRenderNeedsRefresh){
			mCachedRenderNeedsRefresh = false;
		}

	}else{
		renderInternal();
	}

	if (!mIsRenderFinalToTexture && (mSpriteFlags&CLIP_F) != 0){
		clip_plane::disableClipping();
	}
}

void Sprite::drawServer(const ci::mat4 &trans, const DrawParams &drawParams) {
	if((mSpriteFlags&VISIBLE_F) == 0) {
		return;
	}

	buildTransform();
	ci::mat4 totalTransformation = trans*mTransformation;
	ci::gl::pushModelMatrix();

	ci::gl::multModelMatrix(totalTransformation);

	bool debugDraw = getDrawDebug();
	if((mSpriteFlags&TRANSPARENT_F) == 0 && (isEnabled() || debugDraw)) {
		if(debugDraw){
			ci::gl::enableAlphaBlending();
			applyBlendingMode(mBlendMode);

			mDrawOpacity = mOpacity*drawParams.mParentOpacity;
			ci::gl::color(mColor.r, mColor.g, mColor.b, mDrawOpacity);
		} else {
			ci::gl::disableAlphaBlending();
			ci::gl::color(mServerColor);
		}
		if(mUseDepthBuffer) {
			ci::gl::enableDepthRead();
			ci::gl::enableDepthWrite();
		} else {
			ci::gl::disableDepthRead();
			ci::gl::disableDepthWrite();
		}
		drawLocalServer();
	}

	if((mSpriteFlags&CLIP_F) != 0) {
		const ci::Rectf&      clippingBounds = getClippingBounds();
		clip_plane::enableClipping(clippingBounds.getX1(), clippingBounds.getY1(), clippingBounds.getX2(), clippingBounds.getY2());
	}

	ci::gl::popModelMatrix();

	if((mSpriteFlags&DRAW_SORTED_F) == 0) {
		for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
			(*it)->drawServer(totalTransformation, drawParams);
		}
	} else {
		makeSortedChildren();
		for(auto it = mSortedTmp.begin(), it2 = mSortedTmp.end(); it != it2; ++it) {
			(*it)->drawServer(totalTransformation, drawParams);
		}
	}

	if((mSpriteFlags&CLIP_F) != 0) {
		clip_plane::disableClipping();
	}
}

void Sprite::drawLocalClient(){
	if(mRenderBatch){
		mRenderBatch->draw();
	} else if(mCornerRadius > 0.0f){
		ci::gl::drawSolidRoundedRect(ci::Rectf(0.0f, 0.0f, mWidth, mHeight), mCornerRadius);
	} else {
		ci::gl::drawSolidRect(ci::Rectf(0.0f, 0.0f, mWidth, mHeight));
	}
}

void Sprite::drawLocalServer(){
	Sprite::drawLocalClient();
}

void Sprite::buildRenderBatch() {
	if(!mNeedsBatchUpdate) return;
	mNeedsBatchUpdate = false;

	if(getTransparent()){
		mRenderBatch = nullptr;
		return;
	}

	mSpriteShader.loadShaders();

	if(mRenderBatch && mRenderBatch->getGlslProg() != mSpriteShader.getShader()){
		mRenderBatch->replaceGlslProg(mSpriteShader.getShader());
	}

	onBuildRenderBatch();
}

void Sprite::onBuildRenderBatch(){
	auto drawRect =	ci::Rectf(0.0f, 0.0f, getWidth(), getHeight());
	if(mCornerRadius > 0.0f){
		auto theGeom = ci::geom::RoundedRect(drawRect, mCornerRadius);
		if(mRenderBatch){
			mRenderBatch->replaceVboMesh(ci::gl::VboMesh::create(theGeom));
		} else {
			mRenderBatch = ci::gl::Batch::create(theGeom, mSpriteShader.getShader());
		}
	} else {
		auto theGeom = ci::geom::Rect(drawRect);
		if(mRenderBatch){
			mRenderBatch->replaceVboMesh(ci::gl::VboMesh::create(theGeom));
		} else {
			mRenderBatch = ci::gl::Batch::create(theGeom, mSpriteShader.getShader());
		}
	}
}

void Sprite::setPosition( float x, float y, float z ) {
	doSetPosition(ci::vec3(x, y, z));
}

void Sprite::setPosition(const ci::vec2& pos){
	doSetPosition(ci::vec3(pos.x, pos.y, mPosition.z));
}

void Sprite::setPosition(const ci::vec3 &pos) {
	doSetPosition(pos);
}

bool Sprite::getInnerHit(const ci::vec3& p) const {
	if (mInnerHitFunction) {
		return mInnerHitFunction(p);
	}
	return true;
}

void Sprite::doSetPosition(const ci::vec3& pos) {
	if (mPosition == pos) return;

	mPosition = pos;
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	markAsDirty(POSITION_DIRTY);
	dimensionalStateChanged();
	onPositionChanged();
}

void Sprite::doSetScale(const ci::vec3& scale) {
	if(mScale == scale) return;

	mScale = scale;
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	markAsDirty(SCALE_DIRTY);
	dimensionalStateChanged();
	onScaleChanged();
}

const ci::vec3& Sprite::getPosition() const {
	return mPosition;
}

const ci::vec3 Sprite::getGlobalPosition() const{
	if(!getParent()) return ci::vec3();
	return getParent()->localToGlobal(mPosition);
}


const ci::vec3 Sprite::getGlobalCenterPosition() const {
	return getGlobalPosition() + getLocalCenterPosition();
}

ci::vec3 Sprite::getCenterPosition() const {
	return mPosition + getLocalCenterPosition();
}
 
ci::vec3 Sprite::getLocalCenterPosition() const {
	return ci::vec3(floorf(mWidth/2.0f), floorf(mHeight/2.0f), mPosition.z);
}

void Sprite::setScale(float x, float y, float z){
	doSetScale(ci::vec3(x, y, z));
}

void Sprite::setScale(const ci::vec3& scale){
	doSetScale(scale);
}

void Sprite::setScale(float scale){
	setScale(scale, scale, scale);
}

const ci::vec3& Sprite::getScale() const {
	return mScale;
}

void Sprite::setCenter(float x, float y, float z){
	setCenter(ci::vec3(x, y, z));
}

void Sprite::setCenter(const ci::vec3& center){
	if(mCenter == center) return;

	mCenter = center;
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	markAsDirty(CENTER_DIRTY);
	dimensionalStateChanged();
	onCenterChanged();
}

const ci::vec3& Sprite::getCenter() const {
	return mCenter;
}

void Sprite::setRotation(float rotZ) {
	doSetRotation(ci::vec3(mRotation.x, mRotation.y, rotZ) );
}

void Sprite::setRotation(const float xRot, const float yRot, const float zRot) {
	doSetRotation(ci::vec3(xRot, yRot, zRot));
}

void Sprite::setRotation(const ci::vec3& rot) {
	doSetRotation(rot);
}

void Sprite::setRotation(const ci::vec3 &axis, const float angle)
{
	doSetRotation(axis);
	mDoSpecialRotation = true;
	mDegree = angle;
}

void Sprite::doSetRotation(const ci::vec3& rot) {
	if(math::isEqual(mRotation.x, rot.x) && math::isEqual(mRotation.y, rot.y) && math::isEqual(mRotation.z, rot.z) && mDegree ==0.0f)
		return;

	mRotation = rot;
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	markAsDirty(ROTATION_DIRTY);
	dimensionalStateChanged();
	onRotationChanged();
}

ci::vec3 Sprite::getRotation() const
{
	return mRotation;
}

namespace {
	inline float	min(const float a, const float b) { return a <= b ? a : b; }
	inline float	max(const float a, const float b) { return a >= b ? a : b; }
}

ci::Rectf Sprite::getBoundingBox() const {
	const ci::mat4 t = getTransform();

	glm::vec3				ul = glm::vec3(t * glm::vec4(0, 0, 0, 1));
	glm::vec3				ll = glm::vec3(t * glm::vec4(0, getHeight(), 0, 1));
	glm::vec3				lr = glm::vec3(t * glm::vec4(getWidth(), getHeight(), 0, 1));
	glm::vec3				ur = glm::vec3(t * glm::vec4(getWidth(), 0, 0, 1));

	const float				left = min(min(min(ul.x, ll.x), lr.x), ur.x);
	const float				right = max(max(max(ul.x, ll.x), lr.x), ur.x);
	const float				top = min(min(min(ul.y, ll.y), lr.y), ur.y);
	const float				bottom = max(max(max(ul.y, ll.y), lr.y), ur.y);
	return ci::Rectf(left, top, right, bottom);
}

ci::Rectf Sprite::getChildBoundingBox() const {
	if(mChildren.empty()) return getBoundingBox();

	auto it = mChildren.begin();
	// initialize the box to the first child and expand from there
	ci::Rectf result = (*it)->getBoundingBox();
	for(auto end = mChildren.end(); it != end; ++it) {
		ci::Rectf curBounds = (*it)->getBoundingBox();
		result.include(curBounds);
	}
	return result;
}

void Sprite::setDrawSorted(bool drawSorted){
	setFlag(DRAW_SORTED_F, drawSorted, FLAGS_DIRTY, mSpriteFlags);
}

bool Sprite::getDrawSorted() const {
  return getFlag(DRAW_SORTED_F, mSpriteFlags);
}

const ci::mat4 &Sprite::getTransform() const {
	buildTransform();
	return mTransformation;
}

void Sprite::addChild(Sprite &child){
	if(this == &child) {
		DS_LOG_WARNING("Trying to add a Sprite to itself.");
		return;
	}

	if(containsChild(&child))
		return;

	if(getFlag(SHADER_CHILDREN_F, mSpriteFlags)) {
		child.setBaseShader(mSpriteShader.getLocation(), mSpriteShader.getName(), true);
	}

	mChildren.push_back(&child);
	child.setParent(this);
	child.setPerspective(mPerspective);
	child.setDrawSorted(getDrawSorted());
	child.setUseDepthBuffer(mUseDepthBuffer);

	onChildAdded(child);
}

void Sprite::removeChild(Sprite &child){
	if(!containsChild(&child))
		return;

	onChildRemoved(child);

	auto found = std::find(mChildren.begin(), mChildren.end(), &child);
	if(found != mChildren.end()) mChildren.erase(found);
	if(child.getParent() == this) {
		child.setParent(nullptr);
		child.setPerspective(false);
	}
}

void Sprite::setParent(Sprite *parent) {
	if(containsChild(parent)){
		removeChild(*parent);
	}
	removeParent();
	mParent = parent;
	if(mParent)
		mParent->addChild(*this);
	onParentSet();
	markAsDirty(PARENT_DIRTY);
}

void Sprite::removeParent() {
	if (mParent) {
		mParent->removeChild(*this);
		mParent = nullptr;
		markAsDirty(PARENT_DIRTY);
	}
}

void Sprite::remove() {
	clearChildren();
	removeParent();
}

void Sprite::release() {
	removeParent();
	delete this;
}

bool Sprite::containsChild(Sprite *child) const {
	if(mChildren.empty()) return false;
	auto found = std::find(mChildren.begin(), mChildren.end(), child);

	if(found != mChildren.end()) {
		return true;
	}
	return false;
}

void Sprite::clearChildren(){
	if(mChildren.empty()) return;
	auto tempList = mChildren;
	mChildren.clear();

	for(auto it : tempList) {
		it->release();
	}
}

void Sprite::forEachChild(const std::function<void(Sprite&)>& fn, const bool recurse) {
	if(!fn) return;

	for(auto it = mChildren.begin(), end = mChildren.end(); it != end; ++it) {
		Sprite*		s(*it);
		if(s) {
			fn(*s);
			if(recurse) s->forEachChild(fn, recurse);
		}
	}
}

void Sprite::buildTransform() const{
	if(!mUpdateTransform)
		return;

	mUpdateTransform = false;

	mTransformation = glm::mat4();

	mTransformation = glm::translate(mTransformation, glm::vec3(mPosition.x, mPosition.y, mPosition.z));
	if (!mDoSpecialRotation)
	{
		mTransformation = glm::rotate(mTransformation, mRotation.x * math::DEGREE2RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		mTransformation = glm::rotate(mTransformation, mRotation.y * math::DEGREE2RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		mTransformation = glm::rotate(mTransformation, mRotation.z * math::DEGREE2RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		mTransformation = glm::rotate(mTransformation, mDegree * math::DEGREE2RADIAN, mRotation);
	}
	mTransformation = glm::scale(mTransformation, glm::vec3(mScale.x, mScale.y, mScale.z));
	mTransformation = glm::translate(mTransformation, glm::vec3(-mCenter.x*mWidth, -mCenter.y*mHeight, -mCenter.z*mDepth));

	mInverseTransform = glm::inverse(mTransformation);
}

const ci::vec3 Sprite::getSize()const{
	return ci::vec3(mWidth, mHeight, mDepth);
}

void Sprite::setSizeAll(float width, float height, float depth){
	if(mWidth == width && mHeight == height && mDepth == depth) return;

	mWidth = width;
	mHeight = height;
	mDepth = depth;
	mUpdateTransform = true;
	mNeedsBatchUpdate = true;
	markAsDirty(SIZE_DIRTY);
	dimensionalStateChanged();
}

void Sprite::setSizeAll(const ci::vec3& size3d){
	setSizeAll(size3d.x, size3d.y, size3d.z);
}

void Sprite::setSize(const ci::vec2& size2d) {
	setSize(size2d.x, size2d.y);
}

void Sprite::setSize(float width, float height) {
	setSizeAll(width, height, mDepth);
}

void Sprite::sizeToChildBounds(){
	ci::Rectf childBounds = getChildBoundingBox();

	move(childBounds.x1, childBounds.y1);
	setSize(childBounds.getWidth(), childBounds.getHeight());

	// move the children to compensate
	for (auto it = mChildren.begin(), end = mChildren.end(); it != end; ++it){
		(*it)->move(-childBounds.x1, -childBounds.y1);
	}	
}

ci::vec3 Sprite::getPreferredSize() const {
	return ci::vec3(0.0f, 0.0f, 0.0f);
}

void Sprite::setColor(const ci::Color &color){
	if(mColor == color) return;

	mColor = color;
	markAsDirty(COLOR_DIRTY);
}

void Sprite::setColor(float r, float g, float b){
	setColor(ci::Color(r, g, b));
}

void Sprite::setColorA(const ci::ColorA& color){
	setColor(ci::Color(color.r, color.g, color.b));
	setOpacity(color.a);
}

ci::Color Sprite::getColor() const {
	return mColor;
}

ci::ColorA Sprite::getColorA() const {
	return ci::ColorA(mColor.r, mColor.g, mColor.b, mOpacity);
}

void Sprite::setOpacity(float opacity){
	if(mOpacity == opacity) return;

	mOpacity = opacity;
	markAsDirty(OPACITY_DIRTY);
}

float Sprite::getOpacity() const {
	return mOpacity;
}

float Sprite::getDrawOpacity() const {
	return mDrawOpacity;
}


void Sprite::setTransparent(bool transparent){
	mNeedsBatchUpdate = true;
	setFlag(TRANSPARENT_F, transparent, FLAGS_DIRTY, mSpriteFlags);
}

bool Sprite::getTransparent() const {
	return getFlag(TRANSPARENT_F, mSpriteFlags);
}

void Sprite::show(){
	const auto before = visible();
	setFlag(VISIBLE_F, true, FLAGS_DIRTY, mSpriteFlags);
	const auto now = visible();
	if(before != now)
	{
		onAppearanceChanged(now);
	}
	doPropagateVisibilityChange(before, now);
}

void Sprite::hide(){
	const auto before = visible();
	setFlag(VISIBLE_F, false, FLAGS_DIRTY, mSpriteFlags);
	const auto now = visible();
	if(before != now)
	{
		onAppearanceChanged(now);
	}
	doPropagateVisibilityChange(before, now);
}

bool Sprite::visible() const {
	return getFlag(VISIBLE_F, mSpriteFlags);
}

float Sprite::getWidth() const {
	return mWidth;
}

float Sprite::getHeight() const {
	return mHeight;
}

void Sprite::enable(bool flag) {
	mTouchProcess.clearTouches();
	setFlag(ENABLED_F, flag, FLAGS_DIRTY, mSpriteFlags);
}

bool Sprite::isEnabled() const {
	return getFlag(ENABLED_F, mSpriteFlags);
}

void Sprite::buildGlobalTransform() const {
	buildTransform();

	mGlobalTransform = mTransformation;

	for(Sprite *parent = mParent; parent; parent = parent->getParent())	{
		parent->buildTransform();
		mGlobalTransform = parent->mTransformation * mGlobalTransform;
	}

	mInverseGlobalTransform = glm::inverse(mGlobalTransform);
}

void Sprite::parentEventReceived(const ds::Event &e) {
	Sprite*		p = mParent;
	while (p) {
		p->eventReceived(e);
		p = p->mParent;
	}
}

Sprite *Sprite::getParent() const {
	return mParent;
}

const ci::mat4 &Sprite::getGlobalTransform() const {
	buildGlobalTransform();

	return mGlobalTransform;
}

ci::vec3 Sprite::globalToLocal(const ci::vec3 &globalPoint){
	buildGlobalTransform();

	ci::vec4 point = mInverseGlobalTransform * ci::vec4(globalPoint.x, globalPoint.y, globalPoint.z, 1.0f);
	return ci::vec3(point.x, point.y, point.z);
}

ci::vec3 Sprite::localToGlobal(const ci::vec3 &localPoint){
	buildGlobalTransform();
	ci::vec4 point = mGlobalTransform * ci::vec4(localPoint.x, localPoint.y, localPoint.z, 1.0f);
	return ci::vec3(point.x, point.y, point.z);
}

bool Sprite::contains(const ci::vec3& point, const float pad) const {
	// If I don't check this, then sprites with no size are always picked.
	// Someone who knows the math can probably address the root issue.
	if(mWidth < 0.001f || mHeight < 0.001f) return false;
	// Same deal as above.
	//if (mScale.x <= 0.0f || mScale.y <= 0.0f) return false;
	//May have negative scaling
	if (mScale.x == 0.0f || mScale.y == 0.0f) return false;

	buildGlobalTransform();

	glm::vec4 pR = glm::vec4(point.x, point.y, point.z, 1.0f);

	glm::vec4 cA = mGlobalTransform * glm::vec4(-pad, -pad, 0.0f, 1.0f);
	glm::vec4 cB = mGlobalTransform * glm::vec4(mWidth + pad, -pad, 0.0f, 1.0f);
	glm::vec4 cC = mGlobalTransform * glm::vec4(mWidth + pad, mHeight + pad, 0.0f, 1.0f);

	glm::vec4 v1 = cA - cB;
	glm::vec4 v2 = cC - cB;
	glm::vec4 v = pR - cB;

	float dot1 = glm::dot(v, v1);
	float dot2 = glm::dot(v, v2);
	float dot3 = glm::dot(v1, v1);
	float dot4 = glm::dot(v2, v2);

	return (
		dot1 >= 0 &&
		dot2 >= 0 &&
		dot1 <= dot3 &&
		dot2 <= dot4
		);
}

Sprite* Sprite::getHit(const ci::vec3 &point) {
	// EH:  Not sure what bigworld was doing, but I don't see why we'd want to
	// select children of an invisible sprite.
	if(!visible()) {
		return nullptr;
	}
	// EH: Fix a bug where scales of 0,0,0 result in the sprite ALWAYS getting picked
//	if (mScale.x <= 0.0f || mScale.y <= 0.0f || mScale.z <= 0.0f) {
	//Scale could be negative for quickly flipping an image/texture.
		if (mScale.x == 0.0f || mScale.y == 0.0f /*|| mScale.z <= 0.0f*/) {
			return nullptr;
	}
	if(getClipping()) {
		if(!contains(point))
			return nullptr;
	}

	if(!getFlag(DRAW_SORTED_F, mSpriteFlags))
	{
		for(auto it = mChildren.rbegin(), it2 = mChildren.rend(); it != it2; ++it)
		{
			Sprite *child = *it;
			Sprite *hitChild = child->getHit(point);
			if(hitChild)
				return hitChild;
			if(child->visible() && child->isEnabled() && child->contains(point) && child->getInnerHit(point))
				return child;
		}
	} else {
		makeSortedChildren();
		// picks happen in reverse sorted order (front to back)
		for(auto it = mSortedTmp.rbegin(), it2 = mSortedTmp.rend(); it != it2; ++it)
		{
			Sprite *child = *it;
			
			if(child->visible() && child->isEnabled() && child->contains(point) && child->getInnerHit(point))
				return child;
			Sprite *hitChild = child->getHit(point);
			if(hitChild)
				return hitChild;
		}
	}

	if(isEnabled() && contains(point) && getInnerHit(point))
		return this;

	return nullptr;
}

Sprite* Sprite::getPerspectiveHit(CameraPick& pick) {
	if(!visible())
		return nullptr;

	ci::vec3 hitWorldPos;
	static float hitZ;

	// Collect hit candidates from children first, before considering this sprite
	makeSortedChildren();
	typedef std::pair<ds::ui::Sprite*, float> HitCandidate;
	std::vector<HitCandidate> candidates;
	for (auto it = mSortedTmp.rbegin(), it2 = mSortedTmp.rend(); it != it2; ++it) {
		Sprite* hit = (*it)->getPerspectiveHit(pick);
		if (hit) {
			candidates.push_back(std::make_pair(hit, hitZ));
				//std::cout << "Found pick candidate: " << hit->getParent()->localToGlobal(hit->getPosition()).z << " " << hit->getId() << std::endl;
				//return hit;
		}
	}

	// Return the child hit candidate with the nearest hitZ (closest to camera)
	auto nearestCandidate = std::min_element(candidates.begin(), candidates.end(),
		[](const HitCandidate &lhs, const HitCandidate& rhs) {
		return lhs.second < rhs.second;
	});
	if (nearestCandidate != candidates.end()) {
		hitZ = nearestCandidate->second;
		return nearestCandidate->first;
	}

	if (pick.testHitSprite(this, hitWorldPos)) {
		hitZ = pick.calcHitDepth(hitWorldPos);
		return this;
	}

	return nullptr;
}

void Sprite::setProcessTouchCallback(const std::function<void(Sprite *, const TouchInfo &)> &func){
	mProcessTouchInfoCallback = func;
}

void Sprite::processTouchInfo(const TouchInfo &touchInfo) {
	mTouchProcess.processTouchInfo(touchInfo);
}

void Sprite::move(const ci::vec3 &delta) {
	mPosition += delta;
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	// XXX This REALLY should be going through doSetPosition().
	// Don't know what the original thought was, but now I'm
	// nrevous to hook that up.
	markAsDirty(POSITION_DIRTY);
	dimensionalStateChanged();
}

void Sprite::move( float deltaX, float deltaY, float deltaZ ) {
	mPosition += ci::vec3(deltaX, deltaY, deltaZ);
	mUpdateTransform = true;
	mBoundsNeedChecking = true;
	// XXX This REALLY should be going through doSetPosition().
	// Don't know what the original thought was, but now I'm
	// nrevous to hook that up.
	markAsDirty(POSITION_DIRTY);
	dimensionalStateChanged();
}

bool Sprite::multiTouchEnabled() const {
	return mMultiTouchEnabled;
}

const ci::mat4& Sprite::getInverseGlobalTransform() const {
	return mInverseGlobalTransform;
}

const ci::mat4& Sprite::getInverseTransform() const {
	buildTransform();
	return mInverseTransform;
}

ds::UserData& Sprite::getUserData() {
	return mUserData;
}

const ds::UserData& Sprite::getUserData() const {
	return mUserData;
}

bool Sprite::hasMultiTouchConstraint( const BitMask &constraint ) const {
	return mMultiTouchConstraints & constraint;
}

void Sprite::swipe(const ci::vec3 &swipeVector){
	if(mSwipeCallback)
		mSwipeCallback(this, swipeVector);
}

bool Sprite::hasDoubleTap() const{
	if(mDoubleTapCallback){
		return true;
	}
	return false;
}

bool Sprite::tapInfo(const TapInfo& ti){
	if(mTapInfoCallback){
		return mTapInfoCallback(this, ti);
	}
	return false;
}

void Sprite::tap(const ci::vec3 &tapPos){
	if(mTapCallback){
		mTapCallback(this, tapPos);
	}
}

void Sprite::doubleTap(const ci::vec3 &tapPos){
	if(mDoubleTapCallback)
		mDoubleTapCallback(this, tapPos);
}

bool Sprite::hasTap() const {
	if(mTapCallback){
		return true;
	}
	return false;
}

bool Sprite::hasTapInfo() const {
	return mTapInfoCallback != nullptr;
}

void Sprite::processTouchInfoCallback(const TouchInfo &touchInfo){
	if(mProcessTouchInfoCallback)
		mProcessTouchInfoCallback(this, touchInfo);
}

void Sprite::setTapInfoCallback(const std::function<bool(Sprite *, const TapInfo &)> &func){
	mTapInfoCallback = func;
}

void Sprite::setTapCallback(const std::function<void(Sprite *, const ci::vec3 &)> &func){
	mTapCallback = func;
}

void Sprite::setDoubleTapCallback(const std::function<void(Sprite *, const ci::vec3 &)> &func){
	mDoubleTapCallback = func;
}

void Sprite::enableMultiTouch(const BitMask &constraints){
	mMultiTouchEnabled = true;
	mMultiTouchConstraints.clear();
	mMultiTouchConstraints |= constraints;
}

void Sprite::disableMultiTouch() {
	mMultiTouchEnabled = false;
	mMultiTouchConstraints.clear();
}

void Sprite::callAfterDelay(const std::function<void(void)>& fn, const float delay_in_seconds) {
	if (!fn) return;
	cancelDelayedCall();
	ci::Timeline&		t = mEngine.getTweenline().getTimeline();
	mDelayedCallCueRef = t.add(fn, t.getCurrentTime() + delay_in_seconds);
}

void Sprite::cancelDelayedCall(){
	if(mDelayedCallCueRef){
		mDelayedCallCueRef->removeSelf();
		mDelayedCallCueRef = nullptr;
	}
}

bool Sprite::checkBounds() const {
	if(!mCheckBounds)
		return true;

	mBoundsNeedChecking = false;
	mInBounds = false;

	const ci::Rectf&		screenRect(mEngine.getSrcRect());

	float screenMinX = screenRect.getX1();
	float screenMaxX = screenRect.getX2();
	float screenMinY = screenRect.getY1();
	float screenMaxY = screenRect.getY2();

	float spriteMinX = 0.0f;
	float spriteMinY = 0.0f;
	float spriteMaxX = mWidth - 1.0f;
	float spriteMaxY = mHeight - 1.0f;

	ci::vec3 positions[4];

	buildGlobalTransform();

	positions[0] = glm::vec3(mGlobalTransform * glm::vec4(spriteMinX, spriteMinY, 0.0f, 1.0f));
	positions[1] = glm::vec3(mGlobalTransform * glm::vec4(spriteMaxX, spriteMinY, 0.0f, 1.0f));
	positions[2] = glm::vec3(mGlobalTransform * glm::vec4(spriteMinX, spriteMaxY, 0.0f, 1.0f));
	positions[3] = glm::vec3(mGlobalTransform * glm::vec4(spriteMaxX, spriteMaxY, 0.0f, 1.0f));


	spriteMinX = spriteMaxX = positions[0].x;
	spriteMinY = spriteMaxY = positions[0].y;

	for(int i = 1; i < 4; ++i) {
		if(positions[i].x < spriteMinX)
			spriteMinX = positions[i].x;
		if(positions[i].y < spriteMinY)
			spriteMinY = positions[i].y;
		if(positions[i].x > spriteMaxX)
			spriteMaxX = positions[i].x;
		if(positions[i].y > spriteMaxY)
			spriteMaxY = positions[i].y;
	}

	if(spriteMinX == spriteMaxX || spriteMinY == spriteMaxY) {
		return false;
	}

	if(spriteMinX > screenMaxX)
		return false;
	if(spriteMaxX < screenMinX)
		return false;
	if(spriteMinY > screenMaxY)
		return false;
	if(spriteMaxY < screenMinY)
		return false;

	for(int i = 0; i < 4; ++i) {
		if(positions[i].x >= screenMinX && positions[i].x <= screenMaxX && positions[i].y >= screenMinY && positions[i].y <= screenMaxY) {
			mInBounds = true;
			return true;
		}
	}

	ci::vec3 screenpos[4];

	screenpos[0] = ci::vec3(screenMinX, screenMinY, 0.0f);
	screenpos[1] = ci::vec3(screenMaxX, screenMinY, 0.0f);
	screenpos[2] = ci::vec3(screenMinX, screenMaxY, 0.0f);
	screenpos[3] = ci::vec3(screenMaxX, screenMaxY, 0.0f);

	for(int i = 0; i < 4; ++i) {
		if(screenpos[i].x >= spriteMinX && screenpos[i].x <= spriteMaxX && screenpos[i].y >= spriteMinY && screenpos[i].y <= spriteMaxY) {
			mInBounds = true;
			return true;
		}
	}


	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			if(math::intersect2D(screenpos[i % 4], screenpos[(i + 1) % 4], positions[i % 4], positions[(i + 1) % 4])) {
				mInBounds = true;
				return true;
			}
		}
	}

	mInBounds = true;
	return true;
}

void Sprite::setCheckBounds(bool checkBounds) {
	mCheckBounds = checkBounds;
	mInBounds = !mCheckBounds;
	mBoundsNeedChecking = checkBounds;
	markAsDirty(CHECKBOUNDS_DIRTY);
}

bool Sprite::getCheckBounds() const {
	return mCheckBounds;
}

void Sprite::updateCheckBounds() const
{
	if(mBoundsNeedChecking)
		checkBounds();
}

bool Sprite::inBounds() const {
	updateCheckBounds();
	return mInBounds;
}

bool Sprite::isLoaded() const {
	return true;
}

float Sprite::getDepth() const {
	return mDepth;
}

void Sprite::setDragDestination(Sprite *dragDestination){
	mDragDestination = dragDestination;
}

Sprite *Sprite::getDragDestination() const{
	return mDragDestination;
}

void Sprite::setDragDestinationCallback(const std::function<void(Sprite *, const DragDestinationInfo &)> &func){
	mDragDestinationCallback = func;
}

void Sprite::dragDestination(Sprite *sprite, const DragDestinationInfo &dragInfo) {
	if(mDragDestinationCallback)
		mDragDestinationCallback(sprite, dragInfo);
}

bool Sprite::isDirty() const {
	return !mDirty.isEmpty();
}

void Sprite::writeTo(ds::DataBuffer& buf) {
	if ((mSpriteFlags&NO_REPLICATION_F) != 0) return;
	if (mDirty.isEmpty()) return;
	if (mId == ds::EMPTY_SPRITE_ID) {
		// This shouldn't be possible
		DS_LOG_WARNING_M("Sprite::writeTo() on empty sprite ID", SPRITE_LOG);
		return;
	}

	buf.add(mBlobType);
	buf.add(SPRITE_ID_ATTRIBUTE);
	buf.add(mId);

	writeAttributesTo(buf);
	// Terminate the sprite and attribute list
	buf.add(ds::TERMINATOR_CHAR);
	// If I wrote any attributes then make sure to terminate the block
	mDirty.clear();

	for (auto it=mChildren.begin(), end=mChildren.end(); it != end; ++it) {
		(*it)->writeTo(buf);
	}
}

void Sprite::writeClientTo(ds::DataBuffer &buf) {
	writeClientAttributesTo(buf);
	for (auto it=mChildren.begin(), end=mChildren.end(); it != end; ++it) {
		(*it)->writeClientTo(buf);
	}
}

void Sprite::writeAttributesTo(ds::DataBuffer &buf) {
	if(mDirty.has(PARENT_DIRTY)) {
		if(mParent){
			buf.add(PARENT_ATT);
			buf.add(mParent->getId());
		} 

		// Why would you send an empty sprite id?
		//buf.add(ds::EMPTY_SPRITE_ID);
	}
	if (mDirty.has(SIZE_DIRTY)) {
		buf.add(SIZE_ATT);
		buf.add(mWidth);
		buf.add(mHeight);
		buf.add(mDepth);
	}
	if (mDirty.has(FLAGS_DIRTY)) {
		buf.add(FLAGS_ATT);
		buf.add(mSpriteFlags);
		// This is being sent here because I do not want to introduce a
		// new dirty state and the previous code already sets flag to false.
		buf.add(mSpriteShader.getLocation());
		buf.add(mSpriteShader.getName());
	}
	if (mDirty.has(POSITION_DIRTY)) {
		buf.add(POSITION_ATT);
		buf.add(mPosition.x);
		buf.add(mPosition.y);
		buf.add(mPosition.z);
	}
	if (mDirty.has(CHECKBOUNDS_DIRTY)) {
		buf.add(CHECKBOUNDS_ATT);
		buf.add(mCheckBounds);
	}
	if (mDirty.has(CENTER_DIRTY)) {
		buf.add(CENTER_ATT);
		buf.add(mCenter.x);
		buf.add(mCenter.y);
		buf.add(mCenter.z);
	}
	if (mDirty.has(ROTATION_DIRTY)) {
		buf.add(ROTATION_ATT);
		buf.add(mRotation.x);
		buf.add(mRotation.y);
		buf.add(mRotation.z);
	}
	if (mDirty.has(SCALE_DIRTY)) {
		buf.add(SCALE_ATT);
		buf.add(mScale.x);
		buf.add(mScale.y);
		buf.add(mScale.z);
	}
	if (mDirty.has(COLOR_DIRTY)) {
		buf.add(COLOR_ATT);
		buf.add(mColor.r);
		buf.add(mColor.g);
		buf.add(mColor.b);
	}
	if (mDirty.has(OPACITY_DIRTY)) {
		buf.add(OPACITY_ATT);
		buf.add(mOpacity);
	}
	if (mDirty.has(BLEND_MODE)) {
		buf.add(BLEND_ATT);
		buf.add(mBlendMode);
	}
	if (mDirty.has(CLIPPING_BOUNDS)) {
		buf.add(CLIP_BOUNDS_ATT);
		buf.add(mClippingBounds.getX1());
		buf.add(mClippingBounds.getY1());
		buf.add(mClippingBounds.getX2());
		buf.add(mClippingBounds.getY2());
	}
	if (mDirty.has(CORNER_DIRTY)){
		buf.add(CORNERRADIUS_ATT);
		buf.add(mCornerRadius);
	}
	if (mDirty.has(SORTORDER_DIRTY)) {
		// A flat list of ints, the first value is the number of ints
		buf.add(SORTORDER_ATT);
		buf.add<int32_t>(static_cast<int32_t>(mChildren.size()));
		for (auto it=mChildren.begin(), end=mChildren.end(); it != end; ++it) {
			buf.add<sprite_id_t>((*it) ? (*it)->getId() : 0);
		}
	}
}

void Sprite::readFrom(ds::BlobReader& blob) {
	ds::DataBuffer&       buf(blob.mDataBuffer);
	readAttributesFrom(buf);
}

void Sprite::readAttributesFrom(ds::DataBuffer& buf) {
	char          id;
	bool          transformChanged = false;
	while (buf.canRead<char>() && (id=buf.read<char>()) != ds::TERMINATOR_CHAR) {
		if (id == PARENT_ATT) {
			const sprite_id_t     parentId = buf.read<sprite_id_t>();
			Sprite*               parent = mEngine.findSprite(parentId);
			if (parent) parent->addChild(*this);
		} else if (id == SIZE_ATT) {
			mWidth = buf.read<float>();
			mHeight = buf.read<float>();
			mDepth = buf.read<float>();
			transformChanged = true;
		} else if (id == ROTATION_ATT) {
			mRotation.x = buf.read<float>();
			mRotation.y = buf.read<float>();
			mRotation.z = buf.read<float>();
			transformChanged = true;
		} else if (id == FLAGS_ATT) {
			mSpriteFlags = buf.read<int>();
			// This is being read here because I do not want to introduce a
			// new dirty state and the previous code already sets flag to false.
			// This is a no-op if it's the same shader.
			// NOTE: in a __thiscall function, usually order of argument execution
			// is from right to left. but I am just gonna play safe and copy the
			// strings once.
			auto loc = buf.read<std::string>();
			auto name = buf.read<std::string>();
			mSpriteShader.setShaders(loc, name);
		} else if (id == POSITION_ATT) {
			mPosition.x = buf.read<float>();
			mPosition.y = buf.read<float>();
			mPosition.z = buf.read<float>();
			transformChanged = true;
		} else if (id == CHECKBOUNDS_ATT) {
			bool checkBounds = buf.read<bool>();
			setCheckBounds(checkBounds);
		} else if (id == CENTER_ATT) {
			mCenter.x = buf.read<float>();
			mCenter.y = buf.read<float>();
			mCenter.z = buf.read<float>();
			transformChanged = true;
		} else if (id == SCALE_ATT) {
			mScale.x = buf.read<float>();
			mScale.y = buf.read<float>();
			mScale.z = buf.read<float>();
			transformChanged = true;
		} else if (id == COLOR_ATT) {
			mColor.r = buf.read<float>();
			mColor.g = buf.read<float>();
			mColor.b = buf.read<float>();
		} else if (id == OPACITY_ATT) {
			mOpacity = buf.read<float>();
		} else if (id == BLEND_ATT) {
			mBlendMode = buf.read<BlendMode>();
		} else if(id == CLIP_BOUNDS_ATT) {
			float x1 = buf.read<float>();
			float y1 = buf.read<float>();
			float x2 = buf.read<float>();
			float y2 = buf.read<float>();
			mClippingBounds.set(x1, y1, x2, y2);
			markClippingDirty();
		} else if(id == CORNERRADIUS_ATT){ 
			float cornerRad = buf.read<float>();
			mCornerRadius = cornerRad;
		} else if (id == SORTORDER_ATT) {
			int32_t						size = buf.read<int32_t>();
			// I'll assume anything beyond a certain size is a broken packet.
			if (size > 0 && size < 100000) {
				try {
					std::vector<sprite_id_t>	order;
					for (int32_t k=0; k<size; ++k) {
						order.push_back(buf.read<sprite_id_t>());
					}
					setSpriteOrder(order);
				} catch (std::exception const&) {
				}
			} 
		} else {
			readAttributeFrom(id, buf);
		}
	}

	if (transformChanged) {
		mUpdateTransform = true;
		mBoundsNeedChecking = true;
		dimensionalStateChanged();
	}
}

void Sprite::setSpriteId(const ds::sprite_id_t& id) {
	if (mId == id) return;

	if (mId != ds::EMPTY_SPRITE_ID) mEngine.unregisterSprite(*this);
	mId = id;
	if (mId != ds::EMPTY_SPRITE_ID) mEngine.registerSprite(*this);
	markAsDirty(ID_DIRTY);
}

void Sprite::setFlag(const int newBit, const bool on, const DirtyState& dirty, int& oldFlags) {
	int             newFlags = oldFlags;
	if(on) newFlags |= newBit;
	else newFlags &= ~newBit;
	if(newFlags == oldFlags) return;

	oldFlags = newFlags;
	markAsDirty(dirty);
}

bool Sprite::getFlag(const int bit, const int flags) const {
	return (flags&bit) != 0;
}

void Sprite::markAsDirty(const DirtyState& dirty){
	mDirty |= dirty;
	Sprite*		      p = mParent;
	while (p) {
		if ((p->mDirty&CHILD_DIRTY) == true) break;

		p->mDirty |= CHILD_DIRTY;
		p = p->mParent;
	}

	// Opacity is a special case, since it's composed of myself and all parent values.
	// So make sure any children are notified that my opacity changed.
// This was true in BigWorld, probably still but not sure yet.
//	if (dirty.has(ds::OPACITY_DIRTY)) {
//		markChildrenAsDirty(ds::OPACITY_DIRTY);
//	}
}

void Sprite::markChildrenAsDirty(const DirtyState& dirty){
	mDirty |= dirty;
	for (auto it=mChildren.begin(), end=mChildren.end(); it != end; ++it) {
		(*it)->markChildrenAsDirty(dirty);
	}
}

void Sprite::setBlendMode(const BlendMode &blendMode){
	if(mBlendMode == blendMode)
		return;

	mBlendMode = blendMode;
	markAsDirty(BLEND_MODE);
}

BlendMode Sprite::getBlendMode() const {
	return mBlendMode;
}

void Sprite::setBaseShader(const std::string &location, const std::string &shadername, bool applyToChildren){
	mNeedsBatchUpdate = true;
	mSpriteShader.setShaders(location, shadername);
	setFlag(SHADER_CHILDREN_F, applyToChildren, FLAGS_DIRTY, mSpriteFlags);

	if(applyToChildren) {
		for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
			(*it)->setBaseShader(location, shadername, applyToChildren);
		}
	}
}

void Sprite::setBaseShader(const std::string &vertShader, const std::string& fragShader, const std::string &shadername, bool applyToChildren){
	mNeedsBatchUpdate = true;
	mSpriteShader.setShaders(vertShader, fragShader, shadername);
	setFlag(SHADER_CHILDREN_F, applyToChildren, FLAGS_DIRTY, mSpriteFlags);

	if(applyToChildren) {
		for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
			(*it)->setBaseShader(vertShader, fragShader, shadername, applyToChildren);
		}
	}
}

void Sprite::setResource(const ds::Resource&) {
	DS_LOG_WARNING("Set resource hasn't been implemented for this sprite type. Sprite name=" << ds::utf8_from_wstr(getSpriteName(true)));
}

void Sprite::setupFinalRenderBuffer(){
	if(mOutputFbo){
		mOutputFbo = nullptr;
	}

	if(mIsRenderFinalToTexture &&
	   getWidth() > 1.0f &&
	   getHeight() > 1.0f
	   ){
		mOutputFbo = ci::gl::Fbo::create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), mFboFormat);
	} else {
		mOutputFbo = nullptr;
	}
}

void Sprite::setCacheTextureRender(bool cache_texture) {
	mCachedRender = cache_texture;
	setFinalRenderToTexture(mCachedRender);
	mCachedRenderNeedsRefresh = true;
}

void Sprite::redrawCachedTexture() {
	mCachedRenderNeedsRefresh = true;
}

ds::gl::Uniform& Sprite::getUniform(){
	return mUniform;
}

void Sprite::setShaderExtraData(const ci::vec4& data){
	mShaderExtraData = data;
}

void Sprite::setFinalRenderToTexture(bool render_to_texture, ci::gl::Fbo::Format format){
	if (render_to_texture == mIsRenderFinalToTexture) return;
	mIsRenderFinalToTexture = render_to_texture;

	mFboFormat = format;
	setupFinalRenderBuffer();

}

bool Sprite::isFinalRenderToTexture(){
	return mIsRenderFinalToTexture;
}

ci::gl::TextureRef Sprite::getFinalOutTexture(){
	if (mOutputFbo){
		return mOutputFbo->getColorTexture();
	}
	else return nullptr;
}

std::string Sprite::getBaseShaderName() const {
	return mSpriteShader.getName();
}

bool Sprite::getUseShaderTexture() const {
	return mUseShaderTexture;
}

void Sprite::setUseShaderTexture( bool flag ){
	mUseShaderTexture = flag;
}

void Sprite::setClipping( bool flag ) {
	setFlag(CLIP_F, flag, FLAGS_DIRTY, mSpriteFlags);
	markClippingDirty();
}

bool Sprite::getClipping() const {
	return getFlag(CLIP_F, mSpriteFlags);
}

const ci::Rectf& Sprite::getClippingBounds(){
	if(mClippingBoundsDirty) {
		mClippingBoundsDirty = false;
		computeClippingBounds();
	}
	return mClippingBounds;
}

void Sprite::computeClippingBounds(){
	if(getClipping()) {
		ci::vec3 tl = localToGlobal(ci::vec3(0.0f, 0.0f, 0.0f));
		ci::vec3 br = localToGlobal(ci::vec3(getWidth(), getHeight(), 0.0f));
		ci::Rectf clippingRect(tl.x, tl.y, br.x, br.y);

		// first find the outermost clipped window and use it as our reference
		Sprite *outerClippedSprite = nullptr;
		Sprite *curSprite = this;
		while(curSprite) {
			if(curSprite->getClipping())
				outerClippedSprite = curSprite;
			curSprite = curSprite->mParent;
		}

		if(outerClippedSprite) {
			curSprite = mParent;
			while(curSprite) {
				if(curSprite->getClipping()) {
					float ww = curSprite->getWidth();
					float wh = curSprite->getHeight();
					ci::vec3 tl = curSprite->localToGlobal(ci::vec3(0.0f, 0.0f, 0.0f));
					ci::vec3 br = curSprite->localToGlobal(ci::vec3(ww, wh, 0.0f));
					ci::Rectf outerRect(tl.x, tl.y, br.x, br.y);
					clippingRect.clipBy(outerRect);
				}
				curSprite = curSprite->mParent;
			}
		}

		if (clippingRect.x1 == clippingRect.x2) clippingRect.x2 += 1.0f;
		if (clippingRect.y1 == clippingRect.y2) clippingRect.y2 += 1.0f;

		if(    !math::isEqual(clippingRect.x1, mClippingBounds.x1)
			|| !math::isEqual(clippingRect.x2, mClippingBounds.x2)
			|| !math::isEqual(clippingRect.y1, mClippingBounds.y1)
			|| !math::isEqual(clippingRect.y2, mClippingBounds.y2) )
		{
			mClippingBounds = clippingRect;
			markAsDirty(CLIPPING_BOUNDS);
		}
	}
}

void Sprite::dimensionalStateChanged(){
	markClippingDirty();
	if (mLastWidth != mWidth || mLastHeight != mHeight || mLastDepth != mDepth) {
		mLastWidth = mWidth;
		mLastHeight = mHeight;
		mLastDepth = mDepth;
		onSizeChanged();
	}

	setupFinalRenderBuffer();
}

void Sprite::markClippingDirty(){
	mClippingBoundsDirty = true;
	for(auto it = mChildren.begin(), end = mChildren.end(); it != end; ++it) {
		Sprite*     s = *it;
		if(s) s->markClippingDirty();
	}
}

void Sprite::makeSortedChildren() {
	mSortedTmp = mChildren;
	std::sort( mSortedTmp.begin(), mSortedTmp.end(), [](Sprite *i, Sprite *j) {
		return i->getPosition().z < j->getPosition().z;
	});
}

void Sprite::setSecondBeforeIdle( const double idleTime ) {
	mIdleTimer.setSecondBeforeIdle(idleTime);
}

double Sprite::secondsToIdle() const {
	return mIdleTimer.secondsToIdle();
}

bool Sprite::isIdling() const {
	return mIdleTimer.isIdling();
}

void Sprite::startIdling() {
	mIdleTimer.startIdling();
}

void Sprite::resetIdleTimer() {
	mIdleTimer.resetIdleTimer();
}

void Sprite::clearIdleTimer() {
	mIdleTimer.clear();
}

void Sprite::setNoReplicationOptimization(const bool on) {
	// This doesn't need to be replicated. Obviously.
	if (on) mSpriteFlags |= NO_REPLICATION_F;
	else mSpriteFlags &= ~NO_REPLICATION_F;
}

void Sprite::markTreeAsDirty() {
	markAsDirty(ds::BitMask::newFilled());
	markChildrenAsDirty(ds::BitMask::newFilled());
}

void Sprite::setRotateTouches(const bool on) {
	// This doesn't need to be replicated. Obviously.
	if (on) mSpriteFlags |= ROTATE_TOUCHES_F;
	else mSpriteFlags &= ~ROTATE_TOUCHES_F;
}

bool Sprite::isRotateTouches() const {
	return ((mSpriteFlags&ROTATE_TOUCHES_F) != 0);
}

void Sprite::userInputReceived() {
	if (mParent) {
		mParent->userInputReceived();
	}
	resetIdleTimer();
}

void Sprite::sendSpriteToFront(Sprite &sprite) {
	auto found = std::find(mChildren.begin(), mChildren.end(), &sprite);
	if (found == mChildren.end()) return;
	if (*found == mChildren.back()) return;

	mChildren.erase(found);
	mChildren.push_back(&sprite);

	markAsDirty(SORTORDER_DIRTY);
}

void Sprite::sendSpriteToBack(Sprite &sprite) {
	auto found = std::find(mChildren.begin(), mChildren.end(), &sprite);
	if (found == mChildren.end()) return;
	if (*found == mChildren.front()) return;

	mChildren.erase(found);
	mChildren.insert(mChildren.begin(), &sprite);

	markAsDirty(SORTORDER_DIRTY);
}

Sprite* Sprite::getFirstDescendantWithName(const std::wstring& name) {
	Sprite* output = nullptr;
	for(auto it = mChildren.begin(); it != mChildren.end(); it++) {
		if((*it)->getSpriteName() == name) {
			output = (*it);
			break;
		} else {
			output = (*it)->getFirstDescendantWithName(name);
			if(output != nullptr) {
				break;
			}
		}
	}
	return output;
}

void Sprite::sendToFront() {
	if (mParent) {
		mParent->sendSpriteToFront(*this);
	}
}

void Sprite::sendToBack() {
	if (mParent) {
		mParent->sendSpriteToBack(*this);
	}
}

void Sprite::setSpriteOrder(const std::vector<sprite_id_t> &order) {
	for (auto it=order.begin(), end=order.end(); it!=end; ++it) {
		const sprite_id_t	id(*it);
		auto found = std::find_if(mChildren.begin(), mChildren.end(), [id](Sprite *s)->bool { return s && s->getId() == id; });
		if (found != mChildren.end()) {
			Sprite*			s(*found);
			mChildren.erase(found);
			mChildren.push_back(s);
		}
	}
}

ds::ui::SpriteShader &Sprite::getBaseShader() {
	return mSpriteShader;
}

float Sprite::getScaleWidth() const {
	return mScale.x * getWidth();
}

float Sprite::getScaleHeight() const {
	return mScale.y * getHeight();
}

float Sprite::getScaleDepth() const {
	return mScale.z * mDepth;
}

void Sprite::setSwipeCallback( const std::function<void (Sprite *, const ci::vec3 &)> &func ) {
	mSwipeCallback = func;
}

bool Sprite::hasTouches() const {
	return mTouchProcess.hasTouches();
}

void Sprite::passTouchToSprite( Sprite *destinationSprite, const TouchInfo &touchInfo ) {
	if (!destinationSprite || this == destinationSprite) return;

	// tell our current sprite we're through.
	TouchInfo newTouchInfo = touchInfo;
	newTouchInfo.mCurrentGlobalPoint = localToGlobal(ci::vec3(-10000.0f,-10000.0f, 0.0f));	// make sure we touch up outside the sprite area, so buttons don't think they're hit
	newTouchInfo.mPhase = TouchInfo::Removed;
	newTouchInfo.mPassedTouch = true;

	processTouchInfo(newTouchInfo);
	// switch to the new sprite
	mEngine.setSpriteForFinger(touchInfo.mFingerId, destinationSprite);
	newTouchInfo = touchInfo;
	newTouchInfo.mPhase = TouchInfo::Added; 
	destinationSprite->processTouchInfo(newTouchInfo);
}

void Sprite::postAppSetup() {
	mBlobType = BLOB_TYPE;
}

bool Sprite::getPerspective() const{
  return mPerspective;
}

void Sprite::setPerspective( const bool perspective ){
  mPerspective = perspective;

  for (auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
	(*it)->setPerspective(perspective);
  }
}

ds::cfg::Settings* Sprite::getLayoutSettings()
{
	if (mParent != nullptr && mSettings == nullptr) {
		return mParent->getLayoutSettings();
	}
	return mSettings;
}

void Sprite::setLayoutSettings(ds::cfg::Settings& settings) {
	if (mSettings != nullptr) {
		delete mSettings;
		mSettings = nullptr;
	}

	mSettings = new ds::cfg::Settings(settings);
}

void Sprite::setIsInScreenCoordsHack(const bool b){
	mIsInScreenCoordsHack = b;
}

void Sprite::setUseDepthBuffer(bool useDepth){
	mUseDepthBuffer = useDepth;

	for(auto it = mChildren.begin(), it2 = mChildren.end(); it != it2; ++it) {
		(*it)->setUseDepthBuffer(mUseDepthBuffer);
	}
}

bool Sprite::getUseDepthBuffer() const {
	return mUseDepthBuffer;
}

void Sprite::setCornerRadius( const float newRadius ){
	mCornerRadius = newRadius;
	mNeedsBatchUpdate = true;
	markAsDirty(CORNER_DIRTY);
}

float Sprite::getCornerRadius() const{
	return mCornerRadius;
}

void Sprite::setTouchScaleMode(bool doSizeScale){
	mTouchScaleSizeMode = doSizeScale;
}

void Sprite::setInnerHitFunction(std::function<const bool(const ci::vec3&)> func) {
	mInnerHitFunction = func;
}

void Sprite::readClientFrom(ds::DataBuffer& buf){
	while (buf.canRead<char>()) {
		char cmd = buf.read<char>();
		if (cmd != TERMINATOR_CHAR) {
			readClientAttributeFrom(cmd, buf);
		} else {
			return;
		}
	}
}
#ifdef _DEBUG
void Sprite::write(std::ostream &s, const size_t tab) const {
	writeState(s, tab);
	for (auto it=mChildren.begin(), end=mChildren.end(); it!=end; ++it) {
		Sprite*		child(*it);
		if (child) child->write(s, tab + 1);
	}
}

void Sprite::writeState(std::ostream &s, const size_t tab) const {
	for (size_t k=0; k<tab; ++k) s << "\t";
	s << "CLASS NAME=" << typeid(this).name() << "ID=" << mId << " flags=" << mSpriteFlags << " pos=" << mPosition << " size=[" << mWidth << "x" << mHeight << "x" << mDepth << "] scale=" << mScale << " cen=" << mCenter << " rot=" << mRotation << " clip=" << mClippingBounds << std::endl;
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE opacity=" << mOpacity << " use_shader=" << mUseShaderTexture << " use_depthbuffer=" << mUseDepthBuffer << " last_w=" << mLastWidth << " last_h=" << mLastHeight << std::endl;
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE need_bounds_check=" << mBoundsNeedChecking << " in_bounds=" << mInBounds << " check_bounds=" << mCheckBounds << " clip_dirty=" << mClippingBoundsDirty << " update_transform=" << mUpdateTransform << std::endl;
	// Transform
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE transform=" << mTransformation;
	s << std::endl;
	// Inv transform
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE inv_tform=" << mInverseTransform;
	s << std::endl;
	// Global transform
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE global_tx=" << mGlobalTransform;
	s << std::endl;
	// Global inverse transform
	for (size_t k=0; k<tab+2; ++k) s << "\t";
	s << "STATE gl_inv_tx=" << mInverseGlobalTransform;
	s << std::endl;
}

#endif

void Sprite::doPropagateVisibilityChange(bool before, bool after){
	if (before == after) return;

	for (auto it = mChildren.cbegin(), it2 = mChildren.cend(); it != it2; ++it)
	{
		if ((*it)->visible()) {
			// if we were visible, and now we're hidden and our child is visible...
			// our child will vanish.
			if (before && !after) {
				(*it)->onAppearanceChanged(false);
			}

			// if we were hidden, and now we're visible and our child is visible...
			// our child will show up.
			else if (!before && after) {
				(*it)->onAppearanceChanged(true);
			}

			// continue propagating
			(*it)->doPropagateVisibilityChange(before, after);
		}

		// DO NOT propagate this change to hidden children!
		// visibility change of a parent has no effect on hidden children.
	}
}

void Sprite::setDrawDebug(const bool doDebug){
	if(doDebug) mSpriteFlags |= DRAW_DEBUG_F;
	else mSpriteFlags &= ~DRAW_DEBUG_F;
}

bool Sprite::getDrawDebug(){
	return getFlag(DRAW_DEBUG_F, mSpriteFlags);
}


void Sprite::setSpriteName(const std::wstring& name){
	mSpriteName = name;
}

const std::wstring Sprite::getSpriteName(const bool useDefault) const {
	if(mSpriteName.empty() && useDefault){
		std::wstringstream wss;
		wss << getId();
		auto spriteName = wss.str();
		return spriteName;
	} else {
		return mSpriteName;
	}
}

} // namespace ui
} // namespace ds
