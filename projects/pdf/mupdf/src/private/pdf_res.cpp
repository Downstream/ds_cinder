#include "stdafx.h"

#include "private/pdf_res.h"

#include <ds/debug/logger.h>

extern "C" {
#include "mupdf/fitz.h"
#include "mupdf/pdf.h"
#include "mupdf/fitz/pixmap.h"
#include "mupdf/fitz/colorspace.h"
#include "mupdf/fitz/context.h"
}

namespace ds {
namespace pdf {

namespace {

/* LOAD
 * Bundle up the details of loading a PDF
 ******************************************************************/
class Load {
public:
	class Op {
	public:
		Op()			{ }
		virtual ~Op()	{ }
		virtual bool	run(fz_context&, fz_document&, fz_page&) = 0;
	};

public:
	Load() : mCtx(nullptr), mDoc(nullptr), mPage(nullptr) { } 
	~Load()												{ clear(); }

	bool run(Op& op, const std::string& file, const int pageNumber) {
		clear();
		if (!setTo(file, pageNumber)) return false;
		return op.run(*mCtx, *mDoc, *mPage);
	}

private:
	bool setTo(const std::string& file, const int pageNumber) {
		clear();
		try {
			bool			ans = false;
			if ((mCtx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED)) == nullptr) return false;

			fz_register_document_handlers(mCtx);

			// This is pretty ugly because MuPDF uses custom C++-like error handing that
			// has stringent rules, like you're not allowed to return.
			fz_try(mCtx) {
				if ((mDoc = fz_open_document(mCtx, file.c_str()))) {

					const int		pageCount = fz_count_pages(mCtx, mDoc);
					if (!(pageCount <= 0 || pageNumber > pageCount)) {
						
						if ((mPage = fz_load_page(mCtx, mDoc, pageNumber - 1))) {
							ans = true;
						}
						
					}
					
				}
			}
			fz_always(mCtx)
			{
			}
			fz_catch(mCtx)
			{
			}
			return ans;
		} catch(std::exception& ex) {
			DS_LOG_WARNING("pdf_res.cpp::Load::setTo() exception=" << ex.what());
		}
		DS_LOG_WARNING("ds::ui::sprite::Pdf unable to load document \"" << file << "\".");
		return false;
	}

	void clear() {
		try {
			if(mPage) {
				fz_drop_page(mCtx, mPage);
			}
		} catch(...) {}
		try {
			if(mDoc) {
				fz_drop_document(mCtx, mDoc);
			}
		} catch(...) {}
		try {
			if(mCtx) {
				fz_drop_context(mCtx);
			}
		} catch(...) {}
		mPage = nullptr;
		mDoc = nullptr;
		mCtx = nullptr;
	}

	fz_context*			mCtx;
	fz_document*		mDoc;
	fz_page*			mPage;
};

/* EXAMINE
 * Get the top level details of a PDF
 ******************************************************************/
class Examine : public Load::Op {
public:
	int				mWidth, mHeight, mPageCount;

public:
	Examine() : mWidth(0), mHeight(0), mPageCount(0) { }

	virtual bool	run(fz_context& ctx, fz_document& doc, fz_page& page) {
		fz_rect bounds;
		fz_bound_page(&ctx, &page, &bounds);
		if (fz_is_empty_rect(&bounds) || fz_is_infinite_rect(&bounds)) return false;

		mWidth = ceilf(bounds.x1 - bounds.x0);
		mHeight = ceilf(bounds.y1 - bounds.y0);
//		mWidth = page.mediabox.x1;
//		mHeight = page.mediabox.y1;
		mPageCount = fz_count_pages(&ctx, &doc);
		return mWidth > 0 && mHeight > 0 && mPageCount > 0;
	}
};

/* DRAW
 * Draw the page to a surface.
 ******************************************************************/
class Draw : public Load::Op {
public:
	Draw(ds::pdf::PdfRes::Pixels& pixels, const float scale)
		: mPixels(pixels)
		, mScaledWidth(0)
		, mScaledHeight(0)
		, mScale(scale)
		, mWidth(0)
		, mHeight(0)
		{ }


	const ci::ivec2&	getPageSize() const {
		return mPageSize;
	}

	virtual bool	run(fz_context& ctx, fz_document& doc, fz_page& page) {
		if(mScaledWidth > 12000.0f || mScaledHeight > 12000.0f){
			DS_LOG_WARNING("Aborting PdfRes render due to too large of a size of a pdf w/h: " << mScaledWidth << " " << mScaledHeight);
			return false;
		}

		bool					ans = false;
		try {

			fz_pixmap*			pixmap = nullptr;
			// This is pretty ugly because MuPDF uses custom C++-like error handing that
			// has stringent rules, like you're not allowed to return.
			fz_try((&ctx)) {
				if(!getPageSize(ctx, page)) return false;

				// Take the page bounds and transform them by the same matrix that
				// we will use to render the page.
				fz_rect			rect;
				fz_bound_page(&ctx, &page, &rect);
				

				mWidth = static_cast<float>(mPageSize.x);
				mHeight = static_cast<float>(mPageSize.y);
				mScaledWidth = static_cast<int>(mScale * mWidth);
				mScaledHeight = static_cast<int>(mScale * mHeight);


				/// load links!
				mLinks.clear();
				auto linky = fz_load_links(&ctx, &page);
				if(linky) {
					while(true) {
						ds::pdf::PdfLinkInfo this_link;
						this_link.mRawUri = linky->uri;
						this_link.mRect = ci::Rectf(linky->rect.x0 / mWidth, linky->rect.y0 / mHeight, linky->rect.x1 / mWidth, linky->rect.y1 / mHeight);

						if(this_link.mRawUri.find("#") == 0) {
							auto pageNum = this_link.mRawUri.substr(1);
							auto findy = pageNum.find(",");
							if(findy != std::string::npos) {
								pageNum = pageNum.substr(0, findy);
								this_link.mPageDest = ds::string_to_int(pageNum);
							}
						} else {
							this_link.mUrl = this_link.mRawUri;
						}

						//std::cout << "Found link in page: " << this_link.mPageDest << " " << this_link.mRawUri << " " << this_link.mRect << " " << this_link.mUrl << std::endl;

						mLinks.emplace_back(this_link);

						if(linky->next) {
							linky = linky->next;
						} else {
							break;
						}
					}
				}


				const float		zoom = static_cast<float>(mScaledWidth) / mWidth;
				const float		rotation = 1.0f;
				fz_matrix		transform = fz_identity;
				fz_scale(&transform, zoom, zoom);

				fz_transform_rect(&rect, &transform);

				// Create a blank pixmap to hold the result of rendering. The
				// pixmap bounds used here are the same as the transformed page
				// bounds, so it will contain the entire page. The page coordinate
				// space has the origin at the top left corner and the x axis
				// extends to the right and the y axis extends down.
				int w = mScaledWidth, h = mScaledHeight;
				if (mPixels.setSize(w, h)) {
					mPixels.clearPixels();

					pixmap = fz_new_pixmap_with_data(&ctx, fz_device_rgb(&ctx), w, h, 0, w * 3, mPixels.mData);

					if(pixmap){
						fz_clear_pixmap_with_value(&ctx, pixmap, 0xff);
						fz_device* device = fz_new_draw_device(&ctx, &transform, pixmap);
						if(device){
							fz_run_page(&ctx, &page, device, &fz_identity, NULL);
							ans = true;
							fz_close_device(&ctx, device);
							fz_drop_device(&ctx, device);
						}
					}
				}
			}
			fz_always((&ctx)){}
			fz_catch((&ctx)){
				DS_LOG_WARNING("PdfRes: render page error: fz catch mode 1: " << fz_caught_message(&ctx));
			}
			if(pixmap) {
				fz_drop_pixmap(&ctx, pixmap);
			}
		} catch (std::exception const& e) { 
			DS_LOG_WARNING("Exception in PdfRes rendering page: " << e.what());
		}
		return ans;
	}

	virtual bool	getPageSize(fz_context& ctx, fz_page& page) {
		fz_rect bounds;
		fz_bound_page(&ctx, &page, &bounds);
		if (fz_is_empty_rect(&bounds) || fz_is_infinite_rect(&bounds)) return false;
		mPageSize.x = ceilf(bounds.x1 - bounds.x0);
		mPageSize.y = ceilf(bounds.y1 - bounds.y0);
		return mPageSize.x > 0 && mPageSize.y > 0;
	}

	std::vector<ds::pdf::PdfLinkInfo>	mLinks;
	ds::pdf::PdfRes::Pixels&	mPixels;
	int							mScaledWidth,
								mScaledHeight;
	const float					mScale;
	float						mWidth,
								mHeight;
	ci::ivec2					mOutSize,
								mPageSize;
};

} // namespace

/**
 * \class ds::ui::sprite::PdfRes
 */
ci::Surface8uRef PdfRes::renderPage(const std::string& path) {
	ci::Surface8uRef		s;

	Examine					examine;
	Load					load;
	const int				page_num = 1;
	if (!load.run(examine, path, page_num)) return s;
	if (examine.mWidth < 1 || examine.mHeight < 1) return s;

	Pixels					pixels;
	pixels.setSize(examine.mWidth, examine.mHeight);
	Draw					draw(pixels, 1.0f);
	if (!load.run(draw, path, page_num)) return s;

	return ci::Surface::create(pixels.mData, examine.mWidth, examine.mHeight, examine.mWidth * 3, ci::SurfaceChannelOrder(ci::SurfaceChannelOrder::BGRA));
}

PdfRes::PdfRes(ds::GlThread& t)
		: ds::GlThreadClient<PdfRes>(t)
		, mPageCount(0)
		, mPixelsChanged(false) 
		, mPrintedError(false)
{
	mDrawState.mPageNum = 0;
}

void PdfRes::scheduleDestructor() {
	performOnWorkerThread(&PdfRes::_destructor);
}

void PdfRes::_destructor() {
	delete this;
}

PdfRes::~PdfRes() {
}

bool PdfRes::loadPDF(const std::string& fileName) {
	mPrintedError = false;
	// I'd really like to do this initial examine stuff in the
	// worker thread, but I suspect the client is expecting this
	// info to be valid as soon as this is called.
	Examine							examine;
	Load							load;
	if (load.run(examine, fileName, 1)) {
		std::lock_guard<decltype(mMutex)>	l(mMutex);
		mFileName = fileName;
		mState.mWidth = examine.mWidth;
		mState.mHeight = examine.mHeight;
		mPageCount = examine.mPageCount;
		return mPageCount > 0;
	}
	return false;
}

void PdfRes::goToNextPage() {
	if(mState.mPageNum >= mPageCount){
		setPageNum(1); 
	} else {
		setPageNum(mState.mPageNum + 1);
	}
}

void PdfRes::goToPreviousPage() {
	if(mState.mPageNum <= 1){
		setPageNum(mPageCount);
	} else {
		setPageNum(mState.mPageNum - 1);
	}
}


void PdfRes::clearSurface() {
	mSurface = nullptr;
	mSurfacePixels.clearPixels();
}

std::vector<ds::pdf::PdfLinkInfo> PdfRes::getLinks() {
	return mState.mLinks;
}

float PdfRes::getWidth() const {
	if (mSurface) return mSurface->getWidth();
	return (float)mState.mWidth;
}

float PdfRes::getHeight() const {
	if (mSurface) return mSurface->getHeight();
	return (float)mState.mHeight;
}

void PdfRes::setPageNum(int thePageNum) {
	std::lock_guard<decltype(mMutex)>		l(mMutex);

	if (thePageNum < 1) thePageNum = 1;
	if (thePageNum > mPageCount) thePageNum = mPageCount;
	if (thePageNum == mState.mPageNum) return;
	mState.mPageNum = thePageNum;
}

int PdfRes::getPageNum() const {
	std::lock_guard<decltype(mMutex)>		l(mMutex);
	return mState.mPageNum;
}

int PdfRes::getPageCount() const {
	std::lock_guard<decltype(mMutex)>		l(mMutex);
	return mPageCount;
}

ci::ivec2 PdfRes::getPageSize() const {
	std::lock_guard<decltype(mMutex)>		l(mMutex);
	return mState.mPageSize;
}

void PdfRes::setScale(const float theScale) {
	if (mState.mScale == theScale) return;

	std::lock_guard<decltype(mMutex)>		l(mMutex);
	mState.mScale = theScale;	
}

bool PdfRes::update() {
	// Update the page, if necessary.  Batch process -- once my value has been
	// set, I only need the next pending redraw to perform, everything else
	// is unnecessary.
	if(needsUpdate()){
		performOnWorkerThread(&PdfRes::_redrawPage, true);
	}

	bool pixelsWereUpdated = false;
	{
		std::lock_guard<decltype(mMutex)>		l(mMutex);
		if (mPixelsChanged) {

			pixelsWereUpdated = true;
			mPixelsChanged = false;
			;
			if (mPixels.empty()) {
				mSurface = nullptr;
			} else {
				mSurfacePixels.setSize(mPixels.mW, mPixels.mH);
				memcpy((unsigned char *)mSurfacePixels.mData, mPixels.mData, mPixels.mDataSize);

				mPixels.deleteData();

				mSurface = ci::Surface8u::create(mSurfacePixels.mData, mSurfacePixels.mW, mSurfacePixels.mH, mSurfacePixels.mW * 3, ci::SurfaceChannelOrder::RGB);

				if(!mSurface) {
					return false;
				}
			}
		}
		mState.mPageSize = mDrawState.mPageSize;
		mState.mLinks = mDrawState.mLinks;
	}

	return pixelsWereUpdated;
}

bool PdfRes::needsUpdate() {
	std::lock_guard<decltype(mMutex)>		l(mMutex);
	if (mPageCount < 1) return false;
	return mState != mDrawState;
}

void PdfRes::_redrawPage() {
	// Pop out the pieces we need
	bool							printedError;
	state							drawState;
	std::string						fn;
	{
		std::lock_guard<decltype(mMutex)>		l(mMutex);
		
		// No reason to regenerate the same page.
		const float scaleEpsilon = 1e-2f;
		bool isScaleCloseEnough = (abs(mDrawState.mScale - mState.mScale) < scaleEpsilon);

		bool isSameStateIgnoringScale = false;

		float savedDrawStateScale = mDrawState.mScale;
		float savedStateScale = mState.mScale;
		mDrawState.mScale = 1.0f;
		mState.mScale = 1.0f;
		isSameStateIgnoringScale = (mDrawState == mState);
		mDrawState.mScale = savedDrawStateScale;
		mState.mScale = savedStateScale;

		bool isSameFile = (mDrawFileName == mFileName);
		
		if(isScaleCloseEnough && isSameStateIgnoringScale && isSameFile) {
			return;
		}
		drawState = mState;
		fn = mFileName;
		// Prevent the main thread from loading the pixels while
		// I'll be modifying them.
		mPixelsChanged = false;
		printedError = mPrintedError;


		// Setup parameters
		int scaledWidth = (float)drawState.mWidth * drawState.mScale;
		if(scaledWidth < 1) scaledWidth = 1;
		int scaledHeight = scaledWidth * drawState.mHeight / drawState.mWidth;
		if(scaledHeight < 1) scaledHeight = 1;

		// Prevent trying to draw a PDF that's too large (can cause a memory overload and crashy thingy)
		if(scaledWidth > 12000 || scaledHeight > 12000){
			float newW = (float)scaledWidth;
			float newH = (float)scaledHeight;
			float asp = newW / newH;
			newW = 12000;
			newH = 12000;
			if(asp < 1.0f){
				newW = newH * asp;
			} else {
				newH = newW / asp;
			}
			scaledWidth = (int)floorf(newW);
			scaledHeight = (int)floorf(newH);
			drawState.mScale = (float)scaledWidth / (float)drawState.mPageSize.x;
			mState.mScale = drawState.mScale;
			mDrawState.mScale = drawState.mScale;
		}
	}


	if(drawState.mScale < 0.0f){
		DS_LOG_WARNING("Something terrible happened with the drawing scale for your pdf!");
		return;
	}

	// Render to the texture
	Draw							draw(mPixels, drawState.mScale);
	Load							load;

	if(drawState.mScale < 0.0f){
		DS_LOG_WARNING("Something terrible happened with the drawing scale for your pdf!");
		return;
	}

	if(!load.run(draw, fn, drawState.mPageNum)) {
		if(!printedError){
			DS_LOG_WARNING("ds::pdf::PdfRes unable to rasterize document \"" << fn << "\".");
			std::lock_guard<decltype(mMutex)>			l(mMutex);
			mPrintedError = true;
		}
		return;
	}
	drawState.mPageSize = draw.getPageSize();
	drawState.mLinks = draw.mLinks;

	std::lock_guard<decltype(mMutex)>			l(mMutex);
	mPixelsChanged = true;
	mDrawState = drawState;
	// No reason to copy the string which will generally be the same
	if (mDrawFileName != fn) mDrawFileName = fn;
}

/**
 * \class ds::ui::sprite::Pdf::state
 */
PdfRes::state::state()
		: mWidth(0)
		, mHeight(0)
		, mPageNum(1)
		, mScale(1.0f)
		, mPageSize(0, 0) {
}

bool PdfRes::state::operator==(const PdfRes::state& o) {
	return mPageNum == o.mPageNum && mScale == o.mScale && mWidth == o.mWidth && mHeight == o.mHeight;
}

bool PdfRes::state::operator!=(const PdfRes::state& o) {
	return !(*this == o);
}

/**
 * \class ds::ui::sprite::Pdf::Pixels
 */
PdfRes::Pixels::Pixels()
		: mW(0)
		, mH(0)
		, mData(nullptr) {
}

PdfRes::Pixels::~Pixels() {
	deleteData();
}

bool PdfRes::Pixels::empty() const {
	return mW < 1 || mH < 1 || !mData;
}

bool PdfRes::Pixels::setSize(const int w, const int h) {
	if (mW == w && mH == h) return true;

	deleteData();

	mW = 0;
	mH = 0;
	if (w > 0 && h > 0) {
		mDataSize = (w * h) * 3;
		mData = new unsigned char[mDataSize];
	}
	if (!mData) return false;
	mW = w;
	mH = h;
	return true;
}

void PdfRes::Pixels::clearPixels() {
	if (mW < 1 || mH < 1) return;
	const int			size = mW * mH * 3;
	memset(mData, 0, size);
}


void PdfRes::Pixels::deleteData() {
	if(mData) {
		delete mData;
		mData = nullptr;
	}

	mW = 0;
	mH = 0;
}

} // using namespace pdf
} // using namespace ds
