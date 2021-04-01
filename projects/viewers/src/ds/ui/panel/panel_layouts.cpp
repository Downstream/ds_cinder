#include "stdafx.h"

#include "panel_layouts.h"

#include <ds/debug/logger.h>

#include "ds/util/pixel_packer/util/myVector2.h"
#include "ds/util/pixel_packer/util/myBox.h"
#include "ds/util/pixel_packer/pak/algoMaxRects.h"
#include "ds/util/pixel_packer/pak/algoGil.h"
#include <map>

namespace ds {
namespace ui {

struct PanelPackage {
	int mPanelIndex;
	float mAsepectRatio;
	ci::vec2 mTheSize; // for row packing
	ci::vec2 mThePos; // for row packing
	t_myVector2 mPackSize; // for the bin packing algo
	ci::Rectf mOutputRect;
};

bool sortByBigness(PanelPackage& a, PanelPackage& b){
	return a.mPackSize.x * a.mPackSize.y > b.mPackSize.x * b.mPackSize.y;
}

float adjustSizes(std::vector<PanelPackage>& packages, const float fractionalAmount, const float padding){
	float piecemealArea = 0.0f;
	for(auto it = packages.begin(); it < packages.end(); ++it){
		float tw = (float)(*it).mPackSize.x * fractionalAmount - padding;
		float th = tw / (*it).mAsepectRatio;
		tw += padding;
		th += padding;

		piecemealArea += tw * th;
		(*it).mPackSize.x = (int)(tw);
		(*it).mPackSize.y = (int)(th);
	}

	return piecemealArea;
}

bool PanelLayouts::binPack(std::vector<ds::ui::BasePanel*> panels, const ci::Rectf totalAreaRect, const float padding, const float animDur){
	if(panels.empty()) return false;

	std::vector<PanelPackage> thePackages;
	int ind = 0;

	ci::vec2 totalArea = ci::vec2(totalAreaRect.getWidth(), totalAreaRect.getHeight());

	float totalAreaAmount = totalArea.x * totalArea.y;
	float piecemealArea = 0.0f;
	for(auto it = panels.begin(); it < panels.end(); ++it){
		float tw = (*it)->getScaleWidth();
		float th = (*it)->getScaleHeight();
		if(tw < 1.0f || th < 1.0f) continue;

		PanelPackage pp;
		pp.mAsepectRatio = (*it)->getContentAspectRatio();

		if(tw > totalArea.x){
			tw = totalArea.x;
			th = tw / pp.mAsepectRatio;
		}

		if(th > totalArea.y){
			th = totalArea.y;
			tw = th * pp.mAsepectRatio;
		}

		// do the width again in case the height calculation made it too wide
		if(tw > totalArea.x){
			tw = totalArea.x;
			th = tw / pp.mAsepectRatio;
		}
		piecemealArea += tw * th;
		pp.mPackSize = t_myVector2((int)tw, (int)th);
		pp.mPanelIndex = ind;
		thePackages.push_back(pp);
		ind++;
	}

	if(thePackages.empty()) {
		DS_LOG_WARNING("Bin pack failed with no packages!");
		return false;
	}

	// be sure padding gets added
	adjustSizes(thePackages, 1.0f, padding);

	while(piecemealArea < totalAreaAmount * 0.75){
		piecemealArea = adjustSizes(thePackages, 1.1f, padding);
	}

	while(piecemealArea > totalAreaAmount * 0.9f){
		piecemealArea = adjustSizes(thePackages, 0.95f, padding);
	}


	std::vector<PanelPackage> outputPackages;
	bool isFine = false;
	t_algoMaxRects packs;

	for(int i = 0; i < 100; i++){
		isFine = false;
		std::vector<t_myVector2> sizes;
		for(auto it = thePackages.begin(); it < thePackages.end(); ++it){
			sizes.push_back((*it).mPackSize);
		}

		auto returny = packs.pack(sizes, t_myVector2((int)totalArea.x, (int)totalArea.y), isFine);
		if(isFine){

			for(auto it = returny.begin(); it != returny.end(); ++it){
				int xx = (*it).second.x;
				int yy = (*it).second.y;
				int wid = (*it).first.x;
				int hei = (*it).first.y;
				int right = xx + wid;
				int bottom = yy + hei;

				for(auto it = thePackages.begin(); it < thePackages.end(); ++it){
					if((*it).mPackSize.x == wid && (*it).mPackSize.y == hei){
						(*it).mOutputRect = ci::Rectf((float)xx, (float)yy, (float)right, (float)bottom);
						outputPackages.push_back((*it));
						thePackages.erase(it);
						break;
					}
				}
			}
			break;
		} else {
			piecemealArea = adjustSizes(thePackages, 0.95f, padding);
		}
	}

	if(outputPackages.empty()){
		DS_LOG_WARNING("Panel Layouts couldn't determine a layout!");
		return false;
	}

	float farthestRight = 0.0f;
	float farthestBotto = 0.0f;
	for(auto it = outputPackages.begin(); it < outputPackages.end(); ++it){
		ci::vec2 br = (*it).mOutputRect.getLowerRight();
		if(br.x > farthestRight) farthestRight = br.x;
		if(br.y > farthestBotto) farthestBotto = br.y;
	}

	float offsetX = (totalArea.x - farthestRight) / 2.0f;
	float offsetY = (totalArea.y - farthestBotto) / 2.0f;
	float delayey = 0.0f;
	const float deltaDelay = animDur / (float)(outputPackages.size());
	for(auto it = outputPackages.begin(); it < outputPackages.end(); ++it){
		ci::Rectf recty = (*it).mOutputRect;
		auto tmv = panels[(*it).mPanelIndex];
		ci::vec3 destination = ci::vec3(recty.getUpperLeft().x + offsetX + totalAreaRect.x1, recty.getUpperLeft().y + offsetY + totalAreaRect.y1, 0.0f);
		float destWidth = recty.getWidth() - padding;
		if(animDur > 0.0f){
			tmv->tweenStarted();
			tmv->tweenPosition(destination, animDur, delayey, ci::EaseInOutQuad(), [tmv]{ tmv->tweenEnded(); });
			tmv->animateWidthTo(destWidth / tmv->getScale().x);
		} else {
			tmv->setPosition(destination);
			tmv->setViewerWidth(destWidth / tmv->getScale().x);
		}
	}

	return true;
}

bool PanelLayouts::rowPack(std::vector<ds::ui::BasePanel*> panels, const ci::Rectf totalAreaRect, const float padding /*= 5.0f*/, const float animDur /*= 0.35f*/, const int inputRows /*= 2*/) {
	std::vector<PanelPackage> thePackages;
	int ind = 0;

	if(totalAreaRect.getWidth() < 1.0f){
		DS_LOG_WARNING("PanelLayouts:rowPack: too small of an area to fit stuff into!");
		return false;
	}

	if(panels.empty()){
		DS_LOG_VERBOSE(3, "RowPack: no panels set to arrange");
	}

	int numRows = inputRows;
	if (numRows < 1) numRows = 1;

	if (panels.size() == 1) numRows = 1;
	
	ci::vec2 totalArea = ci::vec2(totalAreaRect.getWidth(), totalAreaRect.getHeight());

	for (auto it : panels) {
		PanelPackage pp;
		// a = w / h
		// ah = w
		// h = w / a
		pp.mAsepectRatio = it->getContentAspectRatio();
		if (pp.mAsepectRatio == 0.0f) pp.mAsepectRatio = 1.0f;
		pp.mTheSize = ci::vec2(100.0f, 100.0f / pp.mAsepectRatio);
		pp.mPanelIndex = ind;
		thePackages.push_back(pp);
		ind++;
	}

	if(thePackages.empty()){
		DS_LOG_WARNING("Didn't find any valid panels to rowPack");
		return false;
	}

	float rowHeight = totalAreaRect.getHeight() / numRows;
	if(numRows > 1){
		rowHeight -= (numRows - 1) * padding;
	}

	// a = w / h
	// w = ah
	// h = w / a

	float xp = 0.0f;
	float yp = 0.0f;
	float totalWidth = 0.0f;

	/// first, find out the total width if we were to put everything in one big row
	for (auto it : thePackages) {
		totalWidth += it.mAsepectRatio * rowHeight + padding;
	}

	/// then divide that width by the number of rows
	float widthPerRow = totalWidth / numRows;

	/// then group the panels into the number of rows
	std::vector<float> rowWidths;
	std::map<int, std::vector<PanelPackage>> theRows;
	int curRow = 0;
	for(auto it : thePackages){
		float thisW = it.mAsepectRatio * rowHeight;
		if(xp > widthPerRow 
			&& theRows.size() < numRows 
			&& it.mPanelIndex != thePackages.back().mPanelIndex){
			rowWidths.emplace_back(xp);
			xp = 0.0f;
			curRow++;
		}
		theRows[curRow].emplace_back(it);
		xp += thisW + padding;
	}

	if (rowWidths.size() < theRows.size()) {
		rowWidths.emplace_back(xp);
	} else {
		rowWidths.back() += xp;
	}

	if(rowWidths.size() != theRows.size()){
		DS_LOG_WARNING("Houston we have a big problemo");
		return false;
	}

	float theBot = totalAreaRect.y1;

	/// then determine where each panel will go
	for (int i = 0; i < theRows.size(); i++) {
		xp = 0.0f;
		float thisRowHeight = rowHeight;
		if(rowWidths[i] > totalArea.x){
			float thisAsp = (rowWidths[i] - theRows[i].size() * padding) / rowHeight;
			float destWidth = totalArea.x - theRows[i].size() * padding;
			thisRowHeight = destWidth / thisAsp;
		}

		for (auto& it : theRows[i]) {
			ci::vec2 destination = ci::vec2(xp + totalAreaRect.x1, yp + totalAreaRect.y1);

			float thisBot = destination.y + thisRowHeight;
			if (thisBot > theBot) theBot = thisBot;

			float destWidth = thisRowHeight * it.mAsepectRatio;
			it.mTheSize = ci::vec2(destWidth, thisRowHeight);
			it.mThePos = destination;

			xp += destWidth + padding;

		}

		yp += thisRowHeight + padding;
	}

	// center vertically
	float deltaDif = (totalAreaRect.getY2() - theBot) / 2.0f;
	

	for (auto row : theRows) {
		for (auto it : row.second){
			auto tmv = panels[it.mPanelIndex];
			ci::vec3 destination = ci::vec3(it.mThePos.x, it.mThePos.y + deltaDif, 0.0f);
			float destWidth = it.mTheSize.x;
			if (animDur > 0.0f) {
				tmv->tweenStarted();
				tmv->tweenPosition(destination, animDur, 0.0f, ci::EaseInOutQuad(), [tmv] { tmv->tweenEnded(); });
				tmv->animateWidthTo(destWidth / tmv->getScale().x);
			} else {
				tmv->setPosition(destination);
				tmv->setViewerWidth(destWidth / tmv->getScale().x);
			}
		}
	}

	return true;
}

}
}