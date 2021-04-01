#pragma once
namespace ds {

/**
 * \class DrawParams
 * \brief Provided to sprites for draw()ing functions.
 */
class DrawParams {
public:
	DrawParams() {};
	float mParentOpacity = 1.0f;

	// if this exists, this sprite will be used as the position basis for calculating clipping planes
	// this is primarily for situations where you're drawing clipped sprites into an FBO or texture
	ds::ui::Sprite* mClippingParent = nullptr;
};

} // namespace ds

