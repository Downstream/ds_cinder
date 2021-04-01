#include "stdafx.h"

#include "layout_sprite.h"

#include <ds/app/environment.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/text.h>
#include <ds/util/string_util.h>


namespace ds {
namespace ui {

LayoutSprite::LayoutSprite(ds::ui::SpriteEngine& engine)
	: ds::ui::Sprite(engine)
	, mLayoutType(kLayoutVFlow)
	, mSpacing(0.0f)
	, mShrinkToChildren(kShrinkNone)
	, mOverallAlign(0)
	, mLayoutUpdatedFunction(nullptr)
	, mSkipHiddenChildren(false)
{

}

void LayoutSprite::runLayout(){
	if(mLayoutType == kLayoutNone){
		runNoneLayout();
	} else if(mLayoutType == kLayoutVFlow){
		runFlowLayout(true);
	} else if(mLayoutType == kLayoutHFlow){
		runFlowLayout(false);
	} else if(mLayoutType == kLayoutSize){
		runSizeLayout();
	} else if(mLayoutType == kLayoutVWrap){
		runFlowLayout(true, true);
	} else if(mLayoutType == kLayoutHWrap){
		runFlowLayout(false, true);
	}

	onLayoutUpdate();
}

void LayoutSprite::runNoneLayout(){
	for(auto chillin : mChildren){
		if(auto layoutSprite = dynamic_cast<LayoutSprite*>(chillin)){
			layoutSprite->runLayout();
		}
	}
}

void LayoutSprite::runSizeLayout(){
	const float layoutWidth = getWidth();
	const float layoutHeight = getHeight();

	// Size layout just adjusts the size of child elements to fit, if specified.
	// If children should ignore the size, don't add a layout size and set them to fixed.
	// Otherwise, stretch and flex both "gracefully" match sprites to the size of this layout
	for(auto chillin : mChildren){
		if(mSkipHiddenChildren && !chillin->visible()) continue;
		if(chillin->mLayoutUserType == kFixedSize){
			if(chillin->mLayoutSize.x > 0.0f && chillin->mLayoutSize.y > 0.0f){
				ds::ui::Text* tp = dynamic_cast<ds::ui::Text*>(chillin);
				if(tp){
					tp->setResizeLimit(chillin->mLayoutSize.x, chillin->mLayoutSize.y);
				} else if(chillin->mLayoutFixedAspect){
					// restore position after calculating the box size
					ci::vec3 prePos = chillin->getPosition();
					fitInside(chillin, ci::Rectf(0.0f, 0.0f, chillin->mLayoutSize.x, chillin->mLayoutSize.y), true);
					chillin->setPosition(prePos);
				} else {
					chillin->setSize(mLayoutSize);
				}
			}
		} else if(chillin->mLayoutUserType == kStretchSize || chillin->mLayoutUserType == kFlexSize || chillin->mLayoutUserType == kFillSize){
			const float fixedW = layoutWidth - chillin->mLayoutLPad - chillin->mLayoutRPad;
			const float fixedH = layoutHeight - chillin->mLayoutTPad - chillin->mLayoutBPad;

			ds::ui::Text* tp = dynamic_cast<ds::ui::Text*>(chillin);
			LayoutSprite* ls = dynamic_cast<LayoutSprite*>(chillin);
			if(tp){
				tp->setResizeLimit(fixedW, fixedH);
			} else if(chillin->mLayoutFixedAspect){
				// restore position after calculating the box size
				ci::vec3 prePos = chillin->getPosition();
				fitInside(chillin, ci::Rectf(0.0f, 0.0f, fixedW, fixedH), chillin->mLayoutUserType != kStretchSize);
				chillin->setPosition(prePos);
			} else if(ls){
				ls->setSize(fixedW, fixedH);
				ls->runLayout();
			} else {
				chillin->setSize(fixedW, fixedH);
			}
		}


		if(auto ls = dynamic_cast<LayoutSprite*>(chillin)){
			ls->runLayout();
		}
	}

}

void LayoutSprite::runFlowLayout(const bool vertical, const bool wrap /* = false*/){
	bool hasFills = false;
	int numStretches = 0;
	float totalSize = 0.0f;
	float layoutWidth = getWidth();
	float layoutHeight = getHeight();
	float maxWidth = 0.0f;
	float maxHeight = 0.0f;
	float maxSize = (vertical ? layoutHeight : layoutWidth);
	
	// Look through all children to determine total size and how many items are set to stretch to fill the space
	// Also run recursive layouts on any non-stretch layouts and size any flexible items
	int spacedChildren = 0;

	for(auto chillin : mChildren) {

		if(mSkipHiddenChildren && !chillin->visible()) continue;

		if(chillin->mLayoutUserType == kFillSize){
			hasFills = true;
		} else {
			spacedChildren++;
			if(chillin->mLayoutUserType == kStretchSize){
				// stretch sizes will be set later
				numStretches++;
			} else {
				ds::ui::Text* tp = dynamic_cast<ds::ui::Text*>(chillin);
				LayoutSprite* ls = dynamic_cast<LayoutSprite*>(chillin);
				
				if(chillin->mLayoutUserType == kFixedSize){
					// see if we need to force a particular size, since images and text might resize themselves
					if(chillin->mLayoutSize.x > 0.0f && chillin->mLayoutSize.y > 0.0f){
						if(tp){
							tp->setResizeLimit(chillin->mLayoutSize.x, chillin->mLayoutSize.y);
						} else if(chillin->mLayoutFixedAspect){
							fitInside(chillin, ci::Rectf(0.0f, 0.0f, chillin->mLayoutSize.x, chillin->mLayoutSize.y), true);
						} else {
							chillin->setSize(chillin->mLayoutSize);
						}
					}
				} else if(chillin->mLayoutUserType == kFlexSize){
					// expand the flex children along the opposite axis from the flow
					float fixedW = layoutWidth - chillin->mLayoutLPad - chillin->mLayoutRPad;
					float fixedH = layoutHeight - chillin->mLayoutTPad - chillin->mLayoutBPad;
					if(tp){
						if(vertical){
							tp->setResizeLimit(fixedW);
						} else {
							tp->setResizeLimit(tp->getResizeLimitWidth(), fixedH);
						}
					} else if(chillin->mLayoutFixedAspect){
						if(vertical){
							if(chillin->getWidth() > 0.0f){
								chillin->setScale(fixedW / chillin->getWidth());
							}
						} else {
							if(chillin->getHeight() > 0.0f){
								chillin->setScale(fixedH / chillin->getHeight());
							}
						}
					} else {
						if(vertical){
							chillin->setSize(fixedW, chillin->getHeight());
						} else {
							chillin->setSize(chillin->getWidth(), fixedH);
						}
					}
				}

				// run layouts in case they change their size
				if(ls){
					ls->runLayout();
				}

				// figure out how big this layout is going to be
				if(vertical){
					totalSize += chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad;
					maxWidth = std::fmaxf(maxWidth, (chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad));
				} else {
					totalSize += chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad;
					maxHeight = std::fmaxf(maxHeight, (chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad));
				}
			}
		}
	}

	if(spacedChildren > 1){
		totalSize += (mSpacing * (spacedChildren - 1));
	}

	// figure out how much space is left, and where to start laying out children
	if((mShrinkToChildren == kShrinkWidth) || (mShrinkToChildren == kShrinkBoth)){
		if(vertical){
			layoutWidth = maxWidth;
		} else {
			layoutWidth = maxSize = totalSize;
		}
	}
	if((mShrinkToChildren == kShrinkHeight) || (mShrinkToChildren == kShrinkBoth)){
		if(vertical){
			layoutHeight = maxSize = totalSize;
		} else {
			layoutHeight = maxHeight;
		}
	}
	setSize(layoutWidth, layoutHeight);

	// figure out what's left over and how to use it properly
	float leftOver = 0.0f;
	float perStretch = 0.0f;
	float offset = 0.0f;	
	float wrapOffset = 0.0f;
	float biggest = 0.0f;
	
	if(numStretches > 0){
		leftOver = maxSize - totalSize;
		perStretch = leftOver / numStretches;
	} else {
		// we only calculate offsets if there are no stretches, because otherwise all the space will be used anyway
		if(mOverallAlign == kMiddle || mOverallAlign == kCenter){
			offset = maxSize / 2.0f - totalSize / 2.0f;
		} else if(mOverallAlign == kBottom || mOverallAlign == kRight){
			offset = maxSize - totalSize;
		}
	}

	// now that we know the offset and per stretch size, go through the children again, set position for all fixed, flex, and stretch children 
	// and set the size of any stretch children
	for(auto chillin : mChildren) {

		if(mSkipHiddenChildren && !chillin->visible()) continue;

		if(chillin->mLayoutUserType == kFillSize) {
			continue;
		}
		
		if(chillin->mLayoutUserType == kStretchSize){
			// now stretch sizes can be set
			const float stretchW = (vertical ? layoutWidth : perStretch) - chillin->mLayoutLPad - chillin->mLayoutRPad;
			const float stretchH = (vertical ? perStretch : layoutHeight) - chillin->mLayoutTPad - chillin->mLayoutBPad;

			ds::ui::Text* tp = dynamic_cast<ds::ui::Text*>(chillin);
			LayoutSprite* ls = dynamic_cast<LayoutSprite*>(chillin);
			if(tp){
				tp->setResizeLimit(stretchW, stretchH);
			} else if(chillin->mLayoutFixedAspect){
				fitInside(chillin, ci::Rectf(0.0f, 0.0f, stretchW, stretchH), true);
			} else if(ls){
				ls->setSize(stretchW, stretchH);
				ls->runLayout();
			} else {
				chillin->setSize(stretchW, stretchH);
			}
		} 

		// WIP WRAP
		if(wrap){
			if(!vertical){
				const auto widdy = chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad;
				if(offset+widdy > getWidth() && widdy < getWidth()){
					offset = 0.0f;
					wrapOffset += biggest + mSpacing;
					biggest = chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad;
				} else
				{
					biggest = std::max(biggest, chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad);
				}
			}else{
				const auto hiddy = chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad;
				if(offset+hiddy > getHeight() && hiddy < getHeight()){
					offset = 0.0f;
					wrapOffset += biggest + mSpacing;
					biggest = chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad;
				} else
				{
					biggest = std::max(biggest, chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad);
				}
			}
		}

		// calculate position of child to respect its alignment
		float xPos = 0.0f;
		float yPos = 0.0f;
		if(vertical){
			yPos = offset + chillin->mLayoutTPad;
			if(chillin->mLayoutHAlign == kLeft){
				xPos = chillin->mLayoutLPad;
			} else if(chillin->mLayoutHAlign == kCenter){
				xPos = layoutWidth / 2.0f - chillin->getScaleWidth() / 2.0f;
			} else if(chillin->mLayoutHAlign == kRight){
				xPos = layoutWidth - chillin->getScaleWidth() - chillin->mLayoutRPad;
			}
		} else {
			xPos = offset + chillin->mLayoutLPad;
			if(chillin->mLayoutVAlign == kTop){
				yPos = chillin->mLayoutTPad;
			} else if(chillin->mLayoutVAlign == kMiddle){
				yPos = layoutHeight / 2.0f - chillin->getScaleHeight() / 2.0f;
			} else if(chillin->mLayoutVAlign == kBottom){
				yPos = layoutHeight - chillin->getScaleHeight() - chillin->mLayoutBPad;
			}
		}
		
		// finally set the position of the child
		ci::vec2 childCenter(chillin->getCenter().x * chillin->getScaleWidth(), chillin->getCenter().y * chillin->getScaleHeight());
		ci::vec2 totalOffset = ci::vec2(chillin->mLayoutFudge) + childCenter;
		if(wrap){
		//	if(vertical)
			totalOffset += (vertical)
				? ci::vec2(wrapOffset, 0.f)
				: ci::vec2(0.f, wrapOffset);

		}
		chillin->setPosition(xPos + totalOffset.x, yPos + totalOffset.y, chillin->mLayoutFudge.z);	
		
		// move along through the layout
		if(vertical){
			offset += chillin->mLayoutTPad + chillin->getScaleHeight() + chillin->mLayoutBPad;
		} else {
			offset += chillin->mLayoutLPad + chillin->getScaleWidth() + chillin->mLayoutRPad;
		}
		offset += mSpacing;
	}

	if(wrap && wrapOffset > 0.f){
		if(vertical){
			setSize(wrapOffset+getWidth(), getHeight());
		}else{
			setSize(getWidth(), wrapOffset+getHeight());
		}
	}

	// finally set the position and size of any fill children
	if(hasFills){
		for(auto chillin : mChildren) {

			if(mSkipHiddenChildren && !chillin->visible()) continue;

			if(chillin->mLayoutUserType == kFillSize){
				const float fixedW = layoutWidth - chillin->mLayoutLPad - chillin->mLayoutRPad;
				const float fixedH = layoutHeight - chillin->mLayoutTPad - chillin->mLayoutBPad;

				ds::ui::Text* tp = dynamic_cast<ds::ui::Text*>(chillin);
				LayoutSprite* ls = dynamic_cast<LayoutSprite*>(chillin);
				if(tp){
					tp->setResizeLimit(fixedW, fixedH);
				} else if(chillin->mLayoutFixedAspect){
					fitInside(chillin, ci::Rectf(0.0f, 0.0f, fixedW, fixedH), false);
				} else if(ls){
					ls->setSize(fixedW, fixedH);
					ls->runLayout();
				} else {
					chillin->setSize(fixedW, fixedH);
				}

				// It's possible, after all this, that the child still might not have the full size of (fixedW, fixedH).
				// For example, images will be resized within their aspect, so they'll possibly be off.
				// Compensate for this by centering the child within the area defined by the padding, and respecting the center and fudge factors.

				ci::vec2 centerOffset((fixedW - chillin->getScaleWidth()) * 0.5f, (fixedH - chillin->getScaleHeight()) * 0.5f);
				ci::vec2 childCenter(chillin->getCenter().x * chillin->getScaleWidth(), chillin->getCenter().y * chillin->getScaleHeight());
				ci::vec2 totalOffset = ci::vec2(chillin->mLayoutFudge) + childCenter + centerOffset;
				chillin->setPosition(chillin->mLayoutLPad + totalOffset.x, chillin->mLayoutTPad + totalOffset.y, chillin->mLayoutFudge.z);
			}
		}
	}
}

void LayoutSprite::onLayoutUpdate(){
	if(mLayoutUpdatedFunction){
		mLayoutUpdatedFunction();
	}
}

void LayoutSprite::setLayoutUpdatedFunction(const std::function<void()> layoutUpdatedFunction){
	mLayoutUpdatedFunction = layoutUpdatedFunction;
}

void LayoutSprite::fitInside(ds::ui::Sprite* sp, const ci::Rectf area, const bool letterbox){
	if(!sp) return;

	// make sure the sprite actually has something to fit inside
	if(sp->getWidth() == 0.0f || sp->getHeight() == 0.0f){
		return;
	}
	// a = w / h;
	// h = w /a;
	// w = ah;
	const float spriteAspect = sp->getWidth() / sp->getHeight();
	const float areaAspect = area.getWidth() / area.getHeight();
	bool finalLetterbox = letterbox;
	float destScale = sp->getScale().x;
	if (sp->mLayoutFixedAspectMode == kAspectFill) {
		finalLetterbox = false;
	}
	else if (sp->mLayoutFixedAspectMode == kAspectLetterbox) {
		finalLetterbox = true;
	}


	if(finalLetterbox){
		// When letterboxing, if the sprite is narrower then the dest area, fill the height
		if(spriteAspect < areaAspect){
			destScale = area.getHeight() / sp->getHeight();
		} else {
			destScale = area.getWidth() / sp->getWidth();
		}
	} else {
		// When NOT letterboxing, if the sprite is wider then the dest area, fill the height
		if(spriteAspect > areaAspect){
			destScale = area.getHeight() / sp->getHeight();
		} else {
			destScale = area.getWidth() / sp->getWidth();
		}
	}

	sp->setScale(destScale, destScale, 1.0f);
	sp->setPosition(area.getX1() + area.getWidth() / 2.0f - sp->getScaleWidth() / 2.0f, area.getY1() + area.getHeight() / 2.0f - sp->getScaleHeight() / 2.0f);
}



std::string LayoutSprite::getLayoutSizeModeString(const int sizeMode){
	std::string sizeString = "fixed";
	if(sizeMode == ds::ui::LayoutSprite::kFlexSize)	sizeString = "flex";
	else if(sizeMode == ds::ui::LayoutSprite::kStretchSize) sizeString = "stretch";
	else if(sizeMode == ds::ui::LayoutSprite::kFillSize) sizeString = "fill";
	return sizeString;
}

std::string LayoutSprite::getLayoutVAlignString(const int vAlign){
	std::string sizeString = "top";
	if(vAlign == ds::ui::LayoutSprite::kMiddle)	sizeString = "middle";
	else if(vAlign == ds::ui::LayoutSprite::kBottom)	sizeString = "bottom";
	return sizeString;
}

std::string LayoutSprite::getLayoutHAlignString(const int vAlign){
	std::string sizeString = "left";
	if(vAlign == ds::ui::LayoutSprite::kCenter)	sizeString = "center";
	else if(vAlign == ds::ui::LayoutSprite::kRight)	sizeString = "right";
	return sizeString;
}

std::string LayoutSprite::getLayoutTypeString(const ds::ui::LayoutSprite::LayoutType& propertyValue){
	std::string sizeString = "none";
	if(propertyValue == ds::ui::LayoutSprite::kLayoutVFlow)	sizeString = "vert";
	else if(propertyValue == ds::ui::LayoutSprite::kLayoutHFlow) sizeString = "horiz";
	else if(propertyValue == ds::ui::LayoutSprite::kLayoutVWrap)	sizeString = "vert_wrap";
	else if(propertyValue == ds::ui::LayoutSprite::kLayoutHWrap) sizeString = "horiz_wrap";
	else if(propertyValue == ds::ui::LayoutSprite::kLayoutSize)	sizeString = "size";
	return sizeString;
}

std::string LayoutSprite::getShrinkToChildrenString(const ds::ui::LayoutSprite::ShrinkType& propertyValue){
	std::string sizeString = "none";
	if(propertyValue == ds::ui::LayoutSprite::kShrinkWidth)	sizeString = "width";
	else if(propertyValue == ds::ui::LayoutSprite::kShrinkHeight) sizeString = "height";
	else if(propertyValue == ds::ui::LayoutSprite::kShrinkBoth)	sizeString = "both";
	return sizeString;
}

} // namespace ui
} // namespace ds
