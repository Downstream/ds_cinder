#pragma once
#ifndef DS_UI_LAYOUT_LAYOUT_SPRITE
#define DS_UI_LAYOUT_LAYOUT_SPRITE


#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/sprite_engine.h>

namespace ds {
namespace ui {

/**
* \class LayoutSprite
*		A sprite that can run recursive flow layouts. Children can be normal sprites or other layouts.
*/
class LayoutSprite : public ds::ui::Sprite  {
public:
	LayoutSprite(ds::ui::SpriteEngine& engine);

	/// None = No action taken on elements, but will run recursive layouts
	/// VFlow = Size elements based on their size settings, then position them, top-to-bottom
	/// HFlow = Same as VFlow, but horizontally from left-to-right
	/// Size = only adjust the size of the children, but do not position
	typedef enum { kLayoutNone, kLayoutVFlow, kLayoutHFlow, kLayoutSize, kLayoutVWrap, kLayoutHWrap } LayoutType;

	/// FixedSize = Sprite sized to mLayoutSize (on Sprite.h), or left alone if it's not set
	/// FlexSize = Sprite is adjusted to fit the layout. For example, in a V layout, the text is resized to the width, and the height is calculated
	/// StretchSize = Sprite will fill to the rest of the available space. Multiple Stretch sprites will evenly split the remainder
	/// FillSize = Sprite isn't used in V/H layout calculations, but is sized to the whole layout (minus padding). Great for backgrounds of flex size layouts.
	enum { kFixedSize = 0, kFlexSize, kStretchSize, kFillSize } SizeType;

	typedef enum { kShrinkNone = 0, kShrinkWidth, kShrinkHeight, kShrinkBoth } ShrinkType;

	/// AspectDefault = fixed aspect is based on the layout. 
	/// AspectFill = fixed aspect will always try to fill the target rect. This may result in overflow, so you may want to clip
	/// AspectLetterbox = fixed aspect will letterbox the sprite. 
	typedef enum { kAspectDefault = 0, kAspectFill, kAspectLetterbox } AspectMode;

	/// In VFlow layouts, adjusts the x-position during layout
	enum { kLeft = 0, kCenter, kRight } HAlignment;
	/// In HFlow layouts, adjusts the y-position during layout
	enum { kTop = 0, kMiddle, kBottom } VAlignment;

	/// Fits the sprite supplied into the target area
	static void				fitInside(ds::ui::Sprite* sp, const ci::Rectf area, const bool letterbox);

	void					runLayout();

	const LayoutType&		getLayoutType(){ return mLayoutType; }
	void					setLayoutType(const LayoutType& typey){ mLayoutType = typey; }

	void					setLayoutUpdatedFunction(const std::function<void()> layoutUpdatedFunction);
	void					onLayoutUpdate();

	/// Returns the spacing between each element in the layout (use padding on each element to do add specific spacing)
	float					getSpacing(){ return mSpacing; }
	/// Sets the spacing between each element in the layout (use padding on each element to do add specific spacing)
	void					setSpacing(const float spacing){ mSpacing = spacing; }

	/// For V or H flow layouts, sets the overall alignment for the children. 
	/// Generally only has an effect for sizeType = kFixedSize, kStretchSize or fill; layoutType = kLayoutVFlow or kLayoutHFlow; and there are no stretch children
	void					setOverallAlignment(const int align){ mOverallAlign = align; }
	int						getOverallAlignment(){ return mOverallAlign; }

	/**determines how the sprite adjusts to it's children. 
		1. none: will not touch the size of this layout sprite after laying out the children
		2. width: Adjusts the width of this sprite to it's children (for vertical, the widest child, for horiz, the total height of the children)
		3. height: Adjusts the height of this sprite to it's children (for vertical, the total height of the children, for horiz, the tallest child)
		4. both: Both width and height*/
	const ShrinkType&		getShrinkToChildren() { return mShrinkToChildren; }
	void					setShrinkToChildren(const ShrinkType& shrink) { mShrinkToChildren = shrink; }

	/// If a child is hidden (hide() not setTransparent(false)) will be completely ignored in layouts. Default = false
	const bool				getSkipHiddenChildren() { return mSkipHiddenChildren; }
	void					setSkipHiddenChildren(const bool skipHidden) { mSkipHiddenChildren = skipHidden; }

	/// Helper functions for constructing xml sheets for layouts
	static std::string		getLayoutSizeModeString(const int sizeMode);
	static std::string		getLayoutVAlignString(const int vAlign);
	static std::string		getLayoutHAlignString(const int vAlign);
	static std::string		getLayoutTypeString(const ds::ui::LayoutSprite::LayoutType& propertyValue);
	static std::string		getShrinkToChildrenString(const ds::ui::LayoutSprite::ShrinkType& propertyValue);

protected:
	/// See enum declaration for descriptions
	/// virtual in case you want to override with your own layout jimmies.
	virtual void			runNoneLayout();
	/// virtual in case you want to override with your own layout jimmies.
	virtual void			runSizeLayout();
	/// virtual in case you want to override with your own layout jimmies.
	virtual void			runFlowLayout(const bool vertical, const bool wrap = false);

	std::function<void()>	mLayoutUpdatedFunction;

	float					mSpacing;
	LayoutType				mLayoutType;
	int						mOverallAlign; // can align children if this is not a flex size and there are no stretch children
	ShrinkType				mShrinkToChildren;
	bool					mSkipHiddenChildren;

};

} // namespace ui
} // namespace ds

#endif
