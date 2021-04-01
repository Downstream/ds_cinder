#include "stdafx.h"

#include "pdf_player.h"


#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/util/ui_utils.h>
#include <ds/util/string_util.h>

#include <ds/ui/button/image_button.h>
#include <ds/ui/sprite/pdf.h>

#include "ds/ui/media/interface/pdf_interface.h"
#include "ds/ui/media/media_interface_builder.h"
#include "ds/ui/media/media_viewer_settings.h"

namespace ds {
namespace ui {

PDFPlayer::PDFPlayer(ds::ui::SpriteEngine& eng, bool embedInterface)
  : ds::ui::IPdf(eng)
  , mPdfInterface(nullptr)
  , mEmbedInterface(embedInterface)
  , mInterfaceBelowMedia(false)
  , mShowInterfaceAtStart(true)
  , mLetterbox(true)
{
	enable(false);
	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);

	// in case we haven't loaded pages fast enough, show white
	setTransparent(false);

	// set some callbacks in case we are ever enabled
	setTapCallback([this](ds::ui::Sprite* sprite, const ci::vec3& pos) {
		int count = getPageCount();
		int zeroIndexNextWrapped = (getPageNum() % count);
		setPageNum(zeroIndexNextWrapped + 1);
	});

	setSwipeCallback([this](ds::ui::Sprite* sprite, const ci::vec3& delta) {
		int diff = 0;

		if(delta.x < -20.0f) {
			diff = 1;
		} else if(delta.x > 20.0f) {
			diff = -1;
		}

		if(diff != 0) {
			int count = getPageCount();
			int zeroIndexNextWrapped = ((getPageNum() - 1 + diff + count) % count);
			setPageNum(zeroIndexNextWrapped + 1);
		}
	});

	mLayoutFixedAspect = true;
}

void PDFPlayer::setMedia(const std::string mediaPath) { setResource(ds::Resource(mediaPath)); }

void PDFPlayer::loadNextPage() {
	mLoadingCount++;

	// prevent an infinite loop if we're already loaded and there's only a few pages
	if(mLoadingCount > mNumPages){
		//std::cout << "Cancelling loads cause we've loaded everything" << std::endl;
		return;
	}

	
	// only load a few pages ahead, cause we don't know how many pdfs could be onscreen at once
	if(mLoadedPage > mCurrentPage + mRenderAheadPages) {
		return;
	}
	
	mLoadedPage++;

	// don't loop around when loading
	if(mLoadedPage > mPages.size()) {
		return;

		// tried out looping around, but just skip it
//		mLoadedPage = 1;
	}
	
	auto findy = mPages.find(mLoadedPage);
	if(findy != mPages.end()) {
		auto thePdf = findy->second;
		if(thePdf->getResourceFilename() != mResourceFilename) {
			thePdf->setResourceFilename(mResourceFilename);
			thePdf->setPageNum(findy->first);
		}

		float preW = thePdf->getScale().x;

		const float w = getWidth();
		const float h = getHeight();
		/// this changes the scale, which kicks off a re-render
		/// that will end in a page load notification which hits this function again
		fitInside(thePdf, ci::Rectf(0.0f, 0.0f, w, h), mLetterbox);

		/// if the pdf didn't change scale, it won't re-render, so just move ahead
		if(thePdf->getScale().x == preW) {
			loadNextPage();
		}
	}
}

void PDFPlayer::setResource(const ds::Resource& mediaResource) {

	mSourceResource	= mediaResource;
	mResourceFilename = mSourceResource.getAbsoluteFilePath();

	ci::vec2 prevSize = ci::vec2(0.0f, 0.0f);

	for (auto it : mPages){
		it.second->release();
	}

	mPages.clear();

	mCurrentPage = 1;
	mLoadingCount = 0;
	
	if(mPdfInterface) {
		mPdfInterface->linkPDF(nullptr, ds::Resource());
	}

	bool firsty = true;
	int thePage = 1;
	float theW = 0.0f;
	float theH = 0.0f;
	while(true) {
		if(!firsty && thePage > mNumPages + 1) break;

		auto newPdf = new ds::ui::Pdf(mEngine);

		newPdf->setPageLoadedCallback([this, thePage] {
			//std::cout << "Page loaded: " << thePage << std::endl;
			if(mGoodStatusCallback) mGoodStatusCallback();
			loadNextPage();

			if(mPdfInterface) {
				mPdfInterface->updateWidgets();
			}
		});

		if(firsty) {
			newPdf->setResourceFilename(mResourceFilename);
			newPdf->setPageNum(thePage);
			mLoadedPage = 1;

			newPdf->setPageSizeChangedFn([this] {
				layout();
			});

			newPdf->setLoadErrorCallback([this](const std::string& msg) {
				if(mErrorMsgCallback) mErrorMsgCallback(msg);
			});

			mNumPages = newPdf->getPageCount();

			theW = newPdf->getWidth();
			theH = newPdf->getHeight();
			firsty = false;
		} else {
			newPdf->hide();
		}

		if(mShowingLinks && mCanShowLinks) {
			newPdf->showLinks();
		} else {
			newPdf->hideLinks();
		}

		newPdf->setLinkClickedCallback([this](ds::pdf::PdfLinkInfo info) {
			if(info.mPageDest > 0) {
				setPageNum(info.mPageDest);
			} else if(mLinkClickedCallback) {
				mLinkClickedCallback(info);
			}
		});

		addChildPtr(newPdf);
		mPages[thePage] = newPdf;

		thePage++;
	}

	if (mPdfInterface) {
		mPdfInterface->release();
		mPdfInterface = nullptr;
	}

	if (mEmbedInterface) {
		mPdfInterface = dynamic_cast<PDFInterface*>(MediaInterfaceBuilder::buildMediaInterface(mEngine, this, this));

		if (mPdfInterface) {
			mPdfInterface->sendToFront();
		}
	}


	if (mPdfInterface) {
		if (mShowInterfaceAtStart) {
			mPdfInterface->show();
		} else {
			mPdfInterface->setOpacity(0.0f);
			mPdfInterface->hide();
		}
	}
	
	if(theW < 1 || theH < 1) {
		DS_LOG_WARNING("PDF Player loaded a PDF with no size! " << mediaResource.getAbsoluteFilePath());
		if(mErrorMsgCallback) mErrorMsgCallback("PDF size or file could not be found.");
	}

	setSize(theW, theH);
}

void PDFPlayer::onSizeChanged() { layout(); }

void PDFPlayer::layout() {
	const float w = getWidth();
	const float h = getHeight();

	mLoadedPage = mCurrentPage - 1;
	mLoadingCount = 0;

	loadNextPage();

	if (mPdfInterface) {
		mPdfInterface->setSize(w * 2.0f / 3.0f, mPdfInterface->getHeight());

		float yPos = h - mPdfInterface->getScaleHeight() - mInterfaceBottomPad;
		if (yPos < h / 2.0f) yPos = h / 2.0f;
		if (yPos + mPdfInterface->getScaleHeight() > h) yPos = h - mPdfInterface->getScaleHeight();
		if(mInterfaceBelowMedia) yPos = h;
		mPdfInterface->setPosition(w / 2.0f - mPdfInterface->getScaleWidth() / 2.0f, yPos);
	}
}

void PDFPlayer::showInterface() {
	if (mPdfInterface) {
		mPdfInterface->animateOn();
	}
}

void PDFPlayer::hideInterface() {
	if (mPdfInterface) {
		mPdfInterface->startIdling();
	}
}
void PDFPlayer::setMediaViewerSettings(const MediaViewerSettings& settings) {
	setLetterbox(settings.mLetterBox);
	setShowInterfaceAtStart(settings.mShowInterfaceAtStart);
	mInterfaceBelowMedia = settings.mInterfaceBelowMedia;
	mInterfaceBottomPad = settings.mInterfaceBottomPad;
	mCanShowLinks = settings.mPdfCanShowLinks;
	setLinkClickedCallback(settings.mPdfLinkTappedCallback);
}

void PDFPlayer::setShowInterfaceAtStart(bool showInterfaceAtStart) { mShowInterfaceAtStart = showInterfaceAtStart; }

void PDFPlayer::setLetterbox(const bool doLetterbox) {
	mLetterbox = doLetterbox;
	layout();
}

void PDFPlayer::showLinks() {
	if(!mCanShowLinks) return;
	mShowingLinks = true;
	for (auto it : mPages){
		it.second->showLinks();
	}
}

void PDFPlayer::hideLinks() {
	mShowingLinks = false;
	for(auto it : mPages) {
		it.second->hideLinks();
	}
}

void PDFPlayer::setPageNum(const int pageNum) {

	mCurrentPage = pageNum;

	if(mCurrentPage < 1) mCurrentPage = 1;
	if(mCurrentPage > mNumPages) mCurrentPage = mNumPages;

	ci::vec2 curSize;
	for (auto it : mPages){
		if(it.first == mCurrentPage) {
			it.second->show();
			curSize = ci::vec2(it.second->getSize());
		} else {
			it.second->hide();
		}
	}

	if(mSizeChangedCallback) {
		mSizeChangedCallback(curSize);		
	}

	if(mPageChangeCallback) {
		mPageChangeCallback();
	}

	if(mPdfInterface) {
		mPdfInterface->updateWidgets();
	}

	// ensure the current page is loaded
	mLoadingCount = 0;
	mLoadedPage = mCurrentPage - 1;
	loadNextPage();
}

int PDFPlayer::getPageNum() const {
	return mCurrentPage;
}

int PDFPlayer::getPageCount() const {
	return mNumPages;
}

void PDFPlayer::nextPage() {
	mCurrentPage++;
	if(mCurrentPage > mNumPages) mCurrentPage = 1;
	setPageNum(mCurrentPage);
}

void PDFPlayer::prevPage() {
	mCurrentPage--;
	if(mCurrentPage < 1) mCurrentPage = mNumPages;
	setPageNum(mCurrentPage);
}

void PDFPlayer::goToNextPage() {
	nextPage();
}

void PDFPlayer::goToPreviousPage() {
	prevPage();
}

}  // namespace ui
}  // namespace ds
