#pragma once
#ifndef DS_UI_MEDIA_VIEWER_PDF_PLAYER
#define DS_UI_MEDIA_VIEWER_PDF_PLAYER

#include <ds/data/resource.h>
#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/pdf.h>

namespace ds {
namespace ui {
class MediaInterface;
class PDFInterface;
struct MediaViewerSettings;

/**
 * \class PDFPlayer
 *			Shows a scrollable PDF and puts an interface on top of it.
 *			Note: for PDF thumbnail viewer to show up, the PDF needs to be loaded via a Resource
 *					that has a children vector of resources of the thumbnails set, and the children need to have the correct parentIndex
 *(i.e. page number) set.
 */
class PDFPlayer : public ds::ui::IPdf {
  public:
	PDFPlayer(ds::ui::SpriteEngine& eng, bool embedInterface = true);

	void setMedia(const std::string mediaPath);
	virtual void setResource(const ds::Resource& mediaResource) override;

	ds::Resource& getResource() { return mSourceResource; }

	void layout();

	void		  showInterface();
	void		  hideInterface();
	PDFInterface* getPDFInterface() { return mPdfInterface; }

	/// This is provided primarily for backwards-compatibility
	/// In the past, there used to only be 1 pdf sprite associated with each player
	/// Now, we use this to load a series of pages
	/// So you can use this sprite as the interface for the underlying pdf functions
	ds::ui::IPdf* getPDF() {return this;}

	/// For legacy uses - this is the same as goToNextPage()
	void nextPage();
	/// For legacy uses - this is the same as goToPreviousPage()
	void prevPage();

	// PDF API
	/** Displays the page given, from 1 to getPageCount() */
	virtual void				setPageNum(const int pageNum) override;
	/** Returns the page currently being displayed, 1-indexed (page 1 is the first page, there is no page 0) */
	virtual int					getPageNum() const override;
	/** Returns the number of pages in this document. PDF's are 1-indexed, so the number returned from this function can be set as the last page */
	virtual int					getPageCount() const override;

	/** Advance the current page by 1, does not wrap */
	virtual void				goToNextPage() override;
	/** Decrement the current page by 1, does not wrap */
	virtual void				goToPreviousPage() override;

	void setGoodStatusCallback(std::function<void()> func) { mGoodStatusCallback = func; }
	void setErrorCallback(std::function<void(const std::string&)> func) { mErrorMsgCallback = func; }

	/// Sets all applicable settings from a MediaViewerSettings
	void setMediaViewerSettings(const MediaViewerSettings& settings);

	void setShowInterfaceAtStart(bool showInterfaceAtStart);
	void setSizeChangedCallback(std::function<void(const ci::vec2& size)> func) { mSizeChangedCallback = func; }
	void setLetterbox(const bool doLetterbox);

	virtual void setPageChangeCallback(std::function<void()> func) override { mPageChangeCallback = func; }

	virtual void showLinks();
	virtual void hideLinks();

  protected:
	virtual void	onSizeChanged();

	void loadNextPage();

	std::map<int, ds::ui::Pdf*> mPages;
	ds::Resource	mSourceResource;
	std::string		mResourceFilename;

	int			mCurrentPage = 0;
	int			mLoadedPage = 0;
	int			mLoadingCount = 0;
	int			mNumPages = 0;
	int			mRenderAheadPages = 3;

	PDFInterface*							mPdfInterface;
	bool									mEmbedInterface;
	bool									mShowInterfaceAtStart;
	bool									mInterfaceBelowMedia;
	float									mInterfaceBottomPad = 50.0f;
	bool									mLetterbox;
	bool									mShowingLinks = false;
	bool									mCanShowLinks = true;

	std::function<void(void)>				mPageChangeCallback;
	std::function<void(void)>				mGoodStatusCallback;
	std::function<void(const ci::vec2&)>	mSizeChangedCallback;
	std::function<void(const std::string&)> mErrorMsgCallback;
};

}  // namespace ui
}  // namespace ds

#endif
