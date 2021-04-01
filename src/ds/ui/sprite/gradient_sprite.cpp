#include "stdafx.h"

#include "gradient_sprite.h"

#include <ds/app/blob_reader.h>
#include <ds/app/blob_registry.h>
#include <ds/data/data_buffer.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite_engine.h>

namespace ds {
namespace ui {

namespace {
char				BLOB_TYPE			= 0;

const DirtyState&	TL_DIRTY			= INTERNAL_A_DIRTY;
const DirtyState&	TR_DIRTY			= INTERNAL_B_DIRTY;
const DirtyState&	BL_DIRTY			= INTERNAL_C_DIRTY;
const DirtyState&	BR_DIRTY			= INTERNAL_D_DIRTY;

const char			TL_ATT				= 80;
const char			TR_ATT				= 81;
const char			BL_ATT				= 82;
const char			BR_ATT				= 83;
}


void Gradient::installAsServer(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r);});
}

void Gradient::installAsClient(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<Gradient>(r);});
}

Gradient& Gradient::makeH(SpriteEngine& e, const ci::ColorA& x1, const ci::ColorA& x2, Sprite* parent) {
	return makeAlloc<ds::ui::Gradient>([&e, &x1, &x2]()->ds::ui::Gradient*{
		Gradient* s = new ds::ui::Gradient(e);
		if (s) s->setColorsH(x1, x2);
		return s;
	}, parent);
}

Gradient& Gradient::makeV(SpriteEngine& e, const ci::ColorA& y1, const ci::ColorA& y2, Sprite* parent) {
	return makeAlloc<ds::ui::Gradient>([&e, &y1, &y2]()->ds::ui::Gradient*{
		Gradient* s = new ds::ui::Gradient(e);
		if (s) s->setColorsV(y1, y2);
		return s;
	}, parent);
}

Gradient::Gradient(	ds::ui::SpriteEngine& e,
					const ci::ColorA& tlColor, const ci::ColorA& trColor,
					const ci::ColorA& brColor, const ci::ColorA& blColor)
		: ds::ui::Sprite(e)
{
	mBlobType = BLOB_TYPE;

	setTransparent(false);
	setColorsAll(tlColor, trColor, brColor, blColor);
}

void Gradient::setColorsH(const ci::ColorA& leftColor, const ci::ColorA& rightColor) {
	setGradientColor(TL_DIRTY, leftColor, mTLColor);
	setGradientColor(TR_DIRTY, rightColor, mTRColor);
	setGradientColor(BL_DIRTY, leftColor, mBLColor);
	setGradientColor(BR_DIRTY, rightColor, mBRColor);
}

void Gradient::setColorsV(const ci::ColorA& topColor, const ci::ColorA& botColor ){
	setGradientColor(TL_DIRTY, topColor, mTLColor);
	setGradientColor(TR_DIRTY, topColor, mTRColor);
	setGradientColor(BL_DIRTY, botColor, mBLColor);
	setGradientColor(BR_DIRTY, botColor, mBRColor);
}

void Gradient::setColorsAll(const ci::ColorA& tlColor, const ci::ColorA& trColor,
							const ci::ColorA& brColor, const ci::ColorA& blColor) {
	setGradientColor(TL_DIRTY, tlColor, mTLColor);
	setGradientColor(TR_DIRTY, trColor, mTRColor);
	setGradientColor(BL_DIRTY, blColor, mBLColor);
	setGradientColor(BR_DIRTY, brColor, mBRColor);
}

void Gradient::drawLocalClient() {
	// the magic!
	const float drawOpacity = getDrawOpacity();

	if(mRenderBatch){
		ci::gl::color(ci::ColorA(1.0f, 1.0f, 1.0f, drawOpacity));
		mRenderBatch->draw();
	} else {

		if(mCornerRadius > 0.0f){
			auto rect = ci::geom::RoundedRect();
			rect.colors(ci::ColorA(mTRColor, mTRColor.a * drawOpacity),
						ci::ColorA(mTLColor, mTLColor.a * drawOpacity),
						ci::ColorA(mBRColor, mBRColor.a * drawOpacity),
						ci::ColorA(mBLColor, mBLColor.a * drawOpacity));
			rect.rect(ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()));
			rect.cornerRadius(mCornerRadius);
			rect.cornerSubdivisions(6);
			ci::gl::draw(rect);
		} else {
			auto rect = ci::geom::Rect();
			rect.colors(ci::ColorA(mTLColor, mTLColor.a * drawOpacity),
						ci::ColorA(mTRColor, mTRColor.a * drawOpacity),
						ci::ColorA(mBRColor, mBRColor.a * drawOpacity),
						ci::ColorA(mBLColor, mBLColor.a * drawOpacity));
			rect.rect(ci::Rectf(0.0f, 0.0f, getWidth(), getHeight()));
			ci::gl::draw(rect);
		}
	}
}

void Gradient::onBuildRenderBatch() {
	// Batch renders don't take the currently set color or opacity when drawing
	// So disabled for now.
	// TODO update render batch on opacity change
	return;

	auto drawRect = ci::Rectf(0.0f, 0.0f, getWidth(), getHeight());
	if(mCornerRadius > 0.0f){
		auto theGeom = ci::geom::RoundedRect(drawRect, mCornerRadius);
		theGeom.cornerSubdivisions(6);
		theGeom.colors(mTLColor, mTRColor, mBRColor, mBLColor);
		if(mRenderBatch) mRenderBatch->replaceVboMesh(ci::gl::VboMesh::create(theGeom));
		else mRenderBatch = ci::gl::Batch::create(theGeom, mSpriteShader.getShader());

	} else {
		auto theGeom = ci::geom::Rect(drawRect);
		theGeom.colors(mTLColor, mTRColor, mBRColor, mBLColor);
		if(mRenderBatch) mRenderBatch->replaceVboMesh(ci::gl::VboMesh::create(theGeom));
		else mRenderBatch = ci::gl::Batch::create(theGeom, mSpriteShader.getShader());
	}
}

void Gradient::writeAttributesTo(ds::DataBuffer& buf) {
	ds::ui::Sprite::writeAttributesTo(buf);

	writeGradientColor(TL_DIRTY, mTLColor, TL_ATT, buf);
	writeGradientColor(TR_DIRTY, mTRColor, TR_ATT, buf);
	writeGradientColor(BL_DIRTY, mBLColor, BL_ATT, buf);
	writeGradientColor(BR_DIRTY, mBRColor, BR_ATT, buf);
}

void Gradient::readAttributeFrom(const char attributeId, ds::DataBuffer& buf) {
	if (attributeId == TL_ATT) {
		mTLColor = buf.read<ci::ColorA>();
	} else if (attributeId == TR_ATT) {
		mTRColor = buf.read<ci::ColorA>();
	} else if (attributeId == BL_ATT) {
		mBLColor = buf.read<ci::ColorA>();
	} else if (attributeId == BR_ATT) {
		mBRColor = buf.read<ci::ColorA>();
	} else {
		ds::ui::Sprite::readAttributeFrom(attributeId, buf);
	}
}

void Gradient::setGradientColor(const DirtyState& state, const ci::ColorA& src, ci::ColorA& dst) {
	if (src == dst) return;

	mNeedsBatchUpdate = true;

	dst = src;
	markAsDirty(state);
}

void Gradient::writeGradientColor(const DirtyState& dirty, const ci::ColorA& src, const char att, ds::DataBuffer& buf) const {
	if (mDirty.has(dirty)) {
		buf.add(att);
		buf.add(src);
	}
}

ci::ColorA& Gradient::getColorTL(){
	return mTLColor;
}

ci::ColorA& Gradient::getColorTR(){
	return mTRColor;
}

ci::ColorA& Gradient::getColorBL(){
	return mBLColor;
}

ci::ColorA& Gradient::getColorBR(){
	return mBRColor;
}

void Gradient::tweenColorsAll(const ci::ColorA& tlColor, const ci::ColorA& trColor, const ci::ColorA& brColor, const ci::ColorA& blColor, 
							  const float duration, const float delay, ci::EaseFn easeFunction, const std::function<void(void)>& finishFn, const std::function<void(void)>& updateFn) {

	auto tlOpts = mEngine.getTweenline().getTimeline().apply(&mTLAnim, mTLColor, tlColor, duration, easeFunction);
	tlOpts.updateFn([this, updateFn]() { 
		setGradientColor(TL_DIRTY, mTLAnim.value(), mTLColor);
		if(updateFn) updateFn(); });
	if(finishFn) tlOpts.finishFn(finishFn);
	tlOpts.delay(delay);

	auto trOpts = mEngine.getTweenline().getTimeline().apply(&mTRAnim, mTRColor, trColor, duration, easeFunction);
	trOpts.updateFn([this, updateFn]() { 
		setGradientColor(TR_DIRTY, mTRAnim.value(), mTRColor);
		if(updateFn) updateFn(); });
	if(finishFn) trOpts.finishFn(finishFn);
	trOpts.delay(delay);

	auto brOpts = mEngine.getTweenline().getTimeline().apply(&mBRAnim, mBRColor, brColor, duration, easeFunction);
	brOpts.updateFn([this, updateFn]() { 
		setGradientColor(BR_DIRTY,mBRAnim.value(), mBRColor); 
		if(updateFn) updateFn(); });
	if(finishFn) brOpts.finishFn(finishFn);
	brOpts.delay(delay);

	auto blOpts = mEngine.getTweenline().getTimeline().apply(&mBLAnim, mBLColor, blColor, duration, easeFunction);
	blOpts.updateFn([this, updateFn]() { 
		setGradientColor(BL_DIRTY, mBLAnim.value(), mBLColor); 
		if(updateFn) updateFn(); });
	if(finishFn) blOpts.finishFn(finishFn);
	blOpts.delay(delay);
}

} // using namespace ui
} // using namespace ds
