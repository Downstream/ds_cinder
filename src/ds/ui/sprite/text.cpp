#include "stdafx.h"

#include "text.h"

#include "cairo/cairo.h"
#include "cinder/CinderMath.h"
#include "fontconfig/fontconfig.h"
#include "pango/pangocairo.h"

#include <pango/pango-font.h>
#include <regex>

#include "ds/data/font_list.h"
#include "ds/app/blob_reader.h"
#include "ds/app/blob_registry.h"
#include "ds/data/data_buffer.h"
#include "ds/debug/logger.h"
#include "ds/ui/sprite/sprite_engine.h"
#include "ds/ui/service/pango_font_service.h"
#include "ds/util/string_util.h"
#include <Poco/Stopwatch.h>


namespace {
// Pango/cairo output is premultiplied colors, so rendering it with opacity fades like you'd expect with other sprites
// requires a custom shader that multiplies in the rest of the opacity setting
const std::string opacityFrag =
"uniform sampler2D	tex0;\n"
"uniform bool		useTexture;\n"	// dummy, Engine always sends this anyway
"uniform bool       preMultiply;\n" // dummy, Engine always sends this anyway
"in vec4			Color;\n"
"in vec2			TexCoord0;\n"
"out vec4			oColor;\n"
"void main()\n"
"{\n"
"    oColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"    if (useTexture) {\n"
"        oColor = texture2D( tex0, vec2(TexCoord0.x, 1.0-TexCoord0.y) );\n"
"    }\n"
"    // Undo the pango premultiplication\n"
"    oColor.rgb /= oColor.a;\n"
"    // Now do the normal colorize/optional premultiplication\n"
"    oColor *= Color;\n"
"    if (preMultiply)\n"
"        oColor.rgb *= oColor.a;\n"
"}\n";

const std::string vertShader =
"uniform mat4		ciModelMatrix;\n"
"uniform mat4		ciModelViewProjection;\n"
"uniform vec4		uClipPlane0;\n"
"uniform vec4		uClipPlane1;\n"
"uniform vec4		uClipPlane2;\n"
"uniform vec4		uClipPlane3;\n"
"in vec4			ciPosition;\n" 
"in vec4			ciColor;\n"
"in vec2			ciTexCoord0;\n"
"out vec2			TexCoord0;\n"
"out vec4			Color;\n"
"void main()\n"
"{\n"
"	gl_Position = ciModelViewProjection * ciPosition;\n"
"	TexCoord0 = ciTexCoord0;\n"
"	Color = ciColor;\n"
"	gl_ClipDistance[0] = dot(ciModelMatrix * ciPosition, uClipPlane0);\n"
"	gl_ClipDistance[1] = dot(ciModelMatrix * ciPosition, uClipPlane1);\n"
"	gl_ClipDistance[2] = dot(ciModelMatrix * ciPosition, uClipPlane2);\n"
"	gl_ClipDistance[3] = dot(ciModelMatrix * ciPosition, uClipPlane3);\n"
"}\n";

std::string shaderNameOpaccy = "pango_text_opacity";
}

namespace ds {
namespace ui {


namespace {
char				BLOB_TYPE = 0;

const DirtyState&	FONT_DIRTY = INTERNAL_A_DIRTY;
const DirtyState&	TEXT_DIRTY = INTERNAL_B_DIRTY;
const DirtyState&	LAYOUT_DIRTY = INTERNAL_C_DIRTY;

const char			FONTNAME_ATT = 80;
const char			TEXT_ATT = 81;
const char			LAYOUT_ATT = 82;
}

void Text::installAsServer(ds::BlobRegistry& registry)
{
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r); });
}

void Text::installAsClient(ds::BlobRegistry& registry)
{
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<Text>(r); });
}

Text::Text(ds::ui::SpriteEngine& eng)
	: ds::ui::Sprite(eng)
	, mText("")
	, mProcessedText("")
	, mNeedsMarkupDetection(false)
	, mNeedsFontUpdate(false)
	, mNeedsMeasuring(false)
	, mNeedsTextRender(false)
	, mFitToResizeLimit(false)
	, mNeedsRefit(false)
	, mNeedsMaxResizeFontSizeUpdate(false)
	, mNeedsFontOptionUpdate(false)
	, mProbablyHasMarkup(false)
	, mShrinkToBounds(false)
	, mTextFont("Sans")
	, mTextSize(12.0)
	, mTextColor(ci::Color::white())
	, mDefaultTextItalicsEnabled(false)
	, mDefaultTextSmallCapsEnabled(false)
	, mResizeLimitWidth(-1.0f)
	, mResizeLimitHeight(-1.0f)
	, mLeading(1.0f)
	, mLetterSpacing(0.0f)
	, mTextAlignment(Alignment::kLeft)
	, mDefaultTextWeight(TextWeight::kNormal)
	, mEllipsizeMode(EllipsizeMode::kEllipsizeNone)
	, mWrapMode(WrapMode::kWrapModeWordChar)
	, mPixelWidth(-1)
	, mPixelHeight(-1)
	, mNumberOfLines(0)
	, mWrappedText(false)
	, mPangoContext(nullptr)
	, mPangoLayout(nullptr)
	, mCairoFontOptions(nullptr)
{
	mBlobType = BLOB_TYPE;


	setUseShaderTexture(true);
	mSpriteShader.setShaders(vertShader, opacityFrag, shaderNameOpaccy);
	mSpriteShader.loadShaders();

	if(!mEngine.getPangoFontService().getPangoFontMap()) {
		DS_LOG_WARNING("Cannot create the pango font map, nothing will render for this pango text sprite.");
		return;
	}

	// Create Pango Context for reuse
	mPangoContext = pango_font_map_create_context(mEngine.getPangoFontService().getPangoFontMap());
	if(nullptr == mPangoContext) {
		DS_LOG_WARNING("Cannot create the pango font context.");
		return;
	}

	// Create Pango Layout for reuse
	mPangoLayout = pango_layout_new(mPangoContext);
	if(mPangoLayout == nullptr) {
		DS_LOG_WARNING("Cannot create the pango layout.");
		return;
	}

	// Initialize Cairo surface and context, will be instantiated on demand
	mCairoFontOptions = cairo_font_options_create();
	if(mCairoFontOptions == nullptr) {
		DS_LOG_WARNING("Cannot create Cairo font options.");
		return;
	}

	// Generate the default font config
	mNeedsFontOptionUpdate = true;
	//mNeedsFontUpdate = true;

	setTransparent(false);
}

Text::~Text() {
	if(mCairoFontOptions) {
		cairo_font_options_destroy(mCairoFontOptions);
		mCairoFontOptions = nullptr;
	}

	g_object_unref(mPangoContext); // this one crashes Windows?
	g_object_unref(mPangoLayout);
}

std::string Text::getTextAsString() const{
	return mText;
}

std::wstring Text::getText() const {
	return ds::wstr_from_utf8(mText);
}

void Text::setText(std::string text) {
	if(text != mText) {
		mText = text;
		mNeedsMarkupDetection = true;
		mNeedsMeasuring = true;
		mNeedsTextRender = true;
		mNeedsMaxResizeFontSizeUpdate = true;
		markAsDirty(TEXT_DIRTY);
	}
}

void Text::setText(std::wstring text) {
	setText(ds::utf8_from_wstr(text));
}

const ci::gl::TextureRef Text::getTexture() {
	return mTexture;
}

void Text::setTextStyle(std::string font, double size, ci::ColorA color, Alignment::Enum alignment) {
	setFont(font);
	setFontSize(size);
	setColor(color);
	setAlignment(alignment);
}

Alignment::Enum Text::getAlignment() {
	return mTextAlignment;
}

void Text::setAlignment(Alignment::Enum alignment) {
	if(mTextAlignment != alignment) {
		mTextAlignment = alignment;
		mNeedsMeasuring = true;
		mNeedsTextRender = true;
		mNeedsRefit = true;
		markAsDirty(FONT_DIRTY);
	}
}

float Text::getLeading() const {
	return mLeading;
}

Text& Text::setLeading(const float leading) {
	if(mLeading != leading) {
		mLeading = leading;
		mNeedsMeasuring = true;
		mNeedsTextRender = true;
		mNeedsRefit = true;
		markAsDirty(FONT_DIRTY);
	}
	return *this;
}

float Text::getLetterSpacing() const {
	return mLetterSpacing;
}

Text& Text::setLetterSpacing(const float letterSpacing) {
	if(mLetterSpacing != letterSpacing) {
		mLetterSpacing = letterSpacing;
		mNeedsMeasuring = true;
		mNeedsTextRender = true;
		mNeedsRefit = true;
		markAsDirty(FONT_DIRTY);
	}
	return *this;
}

float Text::getResizeLimitWidth() const {
	return mResizeLimitWidth;
}

float Text::getResizeLimitHeight() const {
	return mResizeLimitHeight;
}

Text& Text::setResizeLimit(const float maxWidth, const float maxHeight) {
	if(mResizeLimitWidth != maxWidth || mResizeLimitHeight != maxHeight){
		mResizeLimitWidth = maxWidth;
		mResizeLimitHeight = maxHeight;

		if(mResizeLimitWidth < 1){
			mResizeLimitWidth = -1.0f; // negative one turns off text wrapping
		}

		/// prevent a situation where a default value of 0.0f only shows 1 line of text
		if(mResizeLimitHeight == 0.0f){
			mResizeLimitHeight = -1.0f;
		}
		mNeedsMeasuring = true;
		mNeedsRefit = true;
		markAsDirty(LAYOUT_DIRTY);
	}

	return *this;
}

Text& Text::setFitFontSizes(std::vector<double> font_sizes)
{
		mFontSizes = font_sizes;
		return *this;
}

Text& Text::setFitToResizeLimit(const bool fitToResize)
{
	if (mFitToResizeLimit != fitToResize) {
		
		mNeedsMeasuring - true;
		mFitToResizeLimit = true;
		mNeedsMaxResizeFontSizeUpdate = true;
		markAsDirty(LAYOUT_DIRTY);
	}

	return *this;
}

bool Text::getShrinkToBounds() const {
	return mShrinkToBounds;
}

void Text::setShrinkToBounds(const bool shrinkToBounds /* = false */) {
	if(mShrinkToBounds == shrinkToBounds) return;

	mShrinkToBounds = shrinkToBounds;
	mNeedsMeasuring = true;

	markAsDirty(LAYOUT_DIRTY);
}

void Text::setTextColor(const ci::Color& color) {
	if(mTextColor != color) {
		mTextColor = color;
		mNeedsTextRender = true;

		markAsDirty(FONT_DIRTY);
	}
}

void Text::setFontSize(double size) {
	if(mTextSize != size) {
		mTextSize = size;
		mNeedsFontSizeUpdate = true;
		mNeedsMeasuring = true;

		markAsDirty(FONT_DIRTY);
	}
}

void Text::setColor(const ci::Color& c){
	setTextColor(c);
}

void Text::setColor(float r, float g, float b){
	setTextColor(ci::Color(r, g, b));
}

void Text::setColorA(const ci::ColorA& c){
	setTextColor(ci::Color(c));
	setOpacity(c.a);
}

Text& Text::setFont(const std::string& font, const double fontSize) {
	if(mTextFont != font || mTextSize != fontSize) {
		mTextFont = mEngine.getFonts().getFontNameForShortName(font);

		mTextSize = fontSize;
		mNeedsFontUpdate = true;
		mNeedsMeasuring = true;
		mNeedsMaxResizeFontSizeUpdate = true;
		markAsDirty(FONT_DIRTY);

		/*
		if(!mEngine.getPangoFontService().getFamilyExists(mTextFont) && !mEngine.getPangoFontService().getFaceExists(mTextFont)){
			DS_LOG_WARNING("Text: Family or face not found: " << mTextFont);
		}
		*/
	}
	return *this;
}

Text& Text::setFont(const std::string& name){
	return setFont(name, mTextSize);
}

float Text::getWidth() const {
	if(mNeedsMeasuring) {
		(const_cast<Text*>(this))->measurePangoText();
	}
	return mWidth;
}

float Text::getHeight() const {
	if(mNeedsMeasuring) {
		(const_cast<Text*>(this))->measurePangoText();
	}
	return mHeight;
}

void Text::setEllipsizeMode(EllipsizeMode theMode){
	if(theMode == mEllipsizeMode) return;

	mEllipsizeMode = theMode;
	mNeedsMeasuring = true;
	mNeedsRefit = true;
	markAsDirty(LAYOUT_DIRTY);
}

EllipsizeMode Text::getEllipsizeMode(){
	return mEllipsizeMode;
}

void Text::setWrapMode(WrapMode theMode){
	if(theMode == mWrapMode) return;

	mWrapMode = theMode;
	mNeedsMeasuring = true;
	mNeedsRefit = true;
	markAsDirty(LAYOUT_DIRTY);
}

WrapMode Text::getWrapMode(){
	return mWrapMode;
}

void Text::onBuildRenderBatch(){
	float preWidth = 0.0f;
	float preHeight = 0.0f;
	if(mTexture){
		preWidth = static_cast<float>(mTexture->getWidth());
		preHeight = static_cast<float>(mTexture->getHeight());
	}

	renderPangoText();

	if(!mTexture){
		mRenderBatch = nullptr;
		return;
	}

	// if we already have a batch of this size, don't rebuild it
	if(mRenderBatch && preHeight == mTexture->getHeight() && preWidth == mTexture->getWidth()){
		mNeedsBatchUpdate = false;
		return;
	}

	auto drawRect = ci::Rectf(0.0f, 0.0f, static_cast<float>(mTexture->getWidth()), static_cast<float>(mTexture->getHeight()));
	if(getPerspective()){
		drawRect = ci::Rectf(0.0f, static_cast<float>(mTexture->getHeight()), static_cast<float>(mTexture->getWidth()), 0.0f);
	}
	auto theGeom = ci::geom::Rect(drawRect);
	if(mRenderBatch){
		mRenderBatch->replaceVboMesh(ci::gl::VboMesh::create(theGeom));
	} else {
		mRenderBatch = ci::gl::Batch::create(theGeom, mSpriteShader.getShader());
	}
	
}

void Text::drawLocalClient(){
	if(mTexture && !mText.empty()){
		ci::gl::color(mColor.r, mColor.g, mColor.b, mDrawOpacity);
		ci::gl::ScopedTextureBind scopedTexture(mTexture);

		ci::gl::ScopedModelMatrix scopedMat;
		ci::gl::translate(mRenderOffset);

		if(mRenderBatch){
			mRenderBatch->draw();
		} else {
			if(getPerspective()) {
				ci::gl::drawSolidRect(ci::Rectf(0.0f, static_cast<float>(mTexture->getHeight()), static_cast<float>(mTexture->getWidth()), 0.0f));
			} else {
				ci::gl::drawSolidRect(ci::Rectf(0.0f, 0.0f, static_cast<float>(mTexture->getWidth()), static_cast<float>(mTexture->getHeight())));
			}
		}
	}
}

int Text::getCharacterIndexForPosition(const ci::vec2& lp){
	measurePangoText();

	int outputIndex = 0;
	if(mPangoLayout){
		int trailing = 0;
		auto success = pango_layout_xy_to_index(mPangoLayout, (int)lp.x * PANGO_SCALE, (int)lp.y * PANGO_SCALE, &outputIndex, &trailing);
		// the "trailing" is if the xy is more than halfway to the next character. this is required to be added for the cursor to be able to be placed after the last character
		outputIndex += trailing;

		//std::cout << "Selected index: " << outputIndex << " " << ds::utf8_from_wstr(mText) << " " << mText.size() << std::endl;

	}	
	return outputIndex;
}
ci::vec2 Text::getPositionForCharacterIndex(const int characterIndex){
	measurePangoText();

	ci::vec2 outputPos = ci::vec2();
	if(mPangoLayout && !mText.empty()){
		PangoRectangle outputRectangle;
		pango_layout_index_to_pos(mPangoLayout, characterIndex, &outputRectangle);

		outputPos.x = (float)outputRectangle.x / (float)PANGO_SCALE;
		// Note: the rectangle returned is to the very top of the very tallest possible character (I think), which makes it a good distance above the top of most characters
		// So I fudged the output of this for a reasonable position for the 'start' of each character from the top-left
		// The exact output for the rectangle of each character can be got from getRectForCharacterIndex() 
		outputPos.y = (float)outputRectangle.y / (float)PANGO_SCALE +(float)outputRectangle.height / (float)PANGO_SCALE / 4.0f; 
	}
	return outputPos;
	
}

ci::Rectf Text::getRectForCharacterIndex(const int characterIndex){
	measurePangoText();

	ci::Rectf outputRect = ci::Rectf();
	if(mPangoLayout && !mText.empty()){
		PangoRectangle outputRectangle;
		pango_layout_index_to_pos(mPangoLayout, characterIndex, &outputRectangle);

		float xx = (float)outputRectangle.x / (float)PANGO_SCALE;
		float yy = (float)outputRectangle.y / (float)PANGO_SCALE;
		outputRect.set(xx, yy, xx + (float)outputRectangle.width / (float)PANGO_SCALE, yy + (float)outputRectangle.height / (float)PANGO_SCALE);
	}
	return outputRect;
}

bool Text::getTextWrapped(){
	// calculate current state if needed
	measurePangoText();
	return mWrappedText;
}

int Text::getNumberOfLines(){
	// calculate current state if needed
	measurePangoText();
	return mNumberOfLines;
}


void Text::onUpdateClient(const UpdateParams&){
	measurePangoText();
}

void Text::onUpdateServer(const UpdateParams&){
	measurePangoText();
}

//bool Text::sizeToFit()
//{
//	if(mFitToResizeLimit)
//	{
//		Poco::Stopwatch sw;
//		sw.start();
//		if(mNeedsMaxResizeFontSizeUpdate)
//		{
//			mNeedsRefit = true;
//			float w = 0;
//			float h = 0;
//
//			std::vector<std::string> words;
//			auto keep_resize_limit_width = getResizeLimitWidth();
//			
//			//handle <br> tags
//			std::regex e("<br\\s?/?>", std::regex_constants::icase);
//			auto text = std::regex_replace(mText, e, "\n");
//			
//			//handle markup tags
//			e = std::regex("(<[^<]*>)(.*?)(</.*?>)");
//			auto search_text = text;
//			std::stringstream unmarkedup_text;
//			std::smatch match;
//			while (std::regex_search(search_text, match, e))
//			{
//				unmarkedup_text << match.prefix();
//				auto openTag = match[1].str();
//				auto content = match[2].str();
//				auto closeTag = match[3].str();
//
//				std::stringstream wordBuilder;
//				std::regex e3("\\s+");
//				for (auto itr = std::sregex_token_iterator(content.begin(), content.end(), e3, -1); itr != std::sregex_token_iterator(); ++itr) {
//					wordBuilder << openTag << itr->str() << closeTag;
//					words.push_back(wordBuilder.str());
//					wordBuilder.str(std::string());
//				}
//				search_text = match.suffix();
//			}
//			
//			unmarkedup_text << search_text;
//			const std::string rest_text = unmarkedup_text.str();
//			std::regex e3("\\s+");
//			for (auto itr = std::sregex_token_iterator(rest_text.begin(), rest_text.end(), e3, -1); itr != std::sregex_token_iterator(); ++itr) {
//				words.push_back(itr->str());
//			}
//			
//			//find the longest relative word
//			std::string widestWord;
//			float widestWidth = 0;
//			for (auto tok : words)
//			{
//
//				setText(tok);
//				w = getWidth();
//				if (w>widestWidth)
//				{
//					widestWidth = w;
//					widestWord = tok;
//				}
//			}
//			
//			
//			
//			//find the max font size.
//			double max_font_size = 1;
//			if (!widestWord.empty())
//			{
//				setText(widestWord);
//				
//				int c = widestWord.length();
//				double fs = 5;
//				double increment=1;
//				while (!getTextWrapped())
//				{
//					//fs = getFontSize();
//					mMaxResizeFontSize = fs;
//					auto fs_new = fs + increment;
//					setFontSize(fs_new);
//					
//					if(getTextWrapped() && increment>1)
//					{
//						increment = 1;
//						setFontSize(fs);
//						continue;
//					}
//					fs = fs_new;
//					increment *= 2;
//					//measurePangoText();
//				}
//
//			}
//			
//			//set the text;
//			setText(text);
//		}
//		if(mNeedsRefit)
//		{
//			double fs = 5;
//			double increment = 1;
//			setFontSize(5);
//			double h = getHeight();
//			while (h<getResizeLimitHeight())
//			{
//				
//				setFontSize(fs + increment);
//				h = getHeight();
//				
//				if(h>getResizeLimitHeight() && increment>1)
//				{
//					increment = 1;
//					setFontSize(fs);
//					h = getHeight();
//					continue;
//				}
//				fs = getFontSize();
//				increment *= 2;
//			}
//
//			fs = getFontSize()-1.5;
//			fs = std::min(fs, double(mMaxResizeFontSize));
//			setFontSize(fs);
//			
//			mNeedsRefit = false;
//			mNeedsMaxResizeFontSizeUpdate = false;
//			mNeedsTextRender = true;
//			//DS_LOG_INFO("Time to find font size: " << double(sw.elapsed()) / sw.resolution());
//		}
//		
//		return true;
//	} else
//	{
//		return false;
//	}
//}

void Text::findFitFontSize()
{
	if (mFitToResizeLimit && mNeedsRefit) {
		if(mFontSizes.size()>0)
		{
			findFitFontSizeFromArray();
			return;
		}
		//------------------------------------
		double fs = 5;
		double increment = 1;
		PangoRectangle extentRect = PangoRectangle();
		PangoRectangle inkRect = PangoRectangle();
				
		auto constFontDescription = pango_layout_get_font_description(mPangoLayout);
		PangoFontDescription* fontDescription = nullptr;
		if (constFontDescription) {
			fontDescription = pango_font_description_copy(constFontDescription);
		}

				
		if (fontDescription) {
			//handle height;
					
			pango_font_description_set_absolute_size(fontDescription, fs * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
			double h = std::max(extentRect.height, inkRect.height);
			while (h < mResizeLimitHeight)
			{

				//set font
				pango_font_description_set_absolute_size(fontDescription, (fs+increment) * 1.3333333333333 * 1024.0);
				pango_layout_set_font_description(mPangoLayout, fontDescription);

				//get height
				pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
				h = std::max(extentRect.height, inkRect.height);
						

				if (h > getResizeLimitHeight() && increment > 1)
				{
					increment = 1;
					//setFontSize(fs);
					pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
					pango_layout_set_font_description(mPangoLayout, fontDescription);
					//h = getHeight();
					pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
					h = std::max(extentRect.height, inkRect.height);
					continue;
				}
				fs = fs + increment;
				increment *= 2;
			}

			//fs = getFontSize() - 1.5;
			auto height_fs = fs - 1.5;

			//handle width;
					
			fs = 5;
			pango_font_description_set_absolute_size(fontDescription, fs * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
			double w = std::max(extentRect.width, inkRect.width);
			while (w < mResizeLimitWidth)
			{

				//set font
				pango_font_description_set_absolute_size(fontDescription, (fs + increment) * 1.3333333333333 * 1024.0);
				pango_layout_set_font_description(mPangoLayout, fontDescription);

				//get height
				pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
				w = std::max(extentRect.width, inkRect.width);


				if (w > getResizeLimitWidth() && increment > 1)
				{
					increment = 1;
					//setFontSize(fs);
					pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
					pango_layout_set_font_description(mPangoLayout, fontDescription);
					//h = getHeight();
					pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
					w = std::max(extentRect.width, inkRect.width);
					continue;
				}
				fs = fs + increment;
				increment *= 2;
			}
			fs = fs - 1.5;
			//pick the smaller one;
			fs = std::min(height_fs, fs);
			//setFontSize(fs);
			pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			mNeedsRefit = false;
			mNeedsMaxResizeFontSizeUpdate = false;
			mNeedsTextRender = true;
		}
		//-----------------------------------------------------------
	}
}

void Text::findFitFontSizeFromArray()
{
	if (mFitToResizeLimit && mNeedsRefit) {

		//------------------------------------
		double fs = 5;
		double increment = 1;
		int idx = 0;
		PangoRectangle extentRect = PangoRectangle();
		PangoRectangle inkRect = PangoRectangle();

		auto constFontDescription = pango_layout_get_font_description(mPangoLayout);
		PangoFontDescription* fontDescription = nullptr;
		if (constFontDescription) {
			fontDescription = pango_font_description_copy(constFontDescription);
		}
		

		if (fontDescription) {
			//sort the font sizes (default small to large)
			std::sort(mFontSizes.begin(), mFontSizes.end());
			
			//handle height;
			fs = mFontSizes[idx];
			pango_font_description_set_absolute_size(fontDescription, fs * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
			double h = std::max(extentRect.height, inkRect.height);
			while (h < mResizeLimitHeight)
			{
				if(idx>=mFontSizes.size()-1)
				{
					break;
				}
				
				fs = mFontSizes[++idx];
				//set font
				pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
				pango_layout_set_font_description(mPangoLayout, fontDescription);

				//get height
				pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
				h = std::max(extentRect.height, inkRect.height);

				if (h > getResizeLimitHeight())
				{
					idx--;
					break;
					
				}
			}
			
			//fs = getFontSize() - 1.5;
			auto height_fs = mFontSizes[idx];

			//handle width;
			idx = 0;
			fs = mFontSizes[idx];
			
			pango_font_description_set_absolute_size(fontDescription, fs * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
			double w = std::max(extentRect.width, inkRect.width);
			while (w < mResizeLimitWidth)
			{
				
				if (idx >= mFontSizes.size() - 1)
				{
					break;
				}
				
				fs = mFontSizes[++idx];
				//set font
				pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
				pango_layout_set_font_description(mPangoLayout, fontDescription);

				//get height
				pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);
				w = std::max(extentRect.width, inkRect.width);

				if (w > getResizeLimitWidth())
				{
					idx--;
					break;
				}
				
			}
			fs = mFontSizes[idx];
			//pick the smaller one;
			fs = std::min(height_fs, fs);
			//setFontSize(fs);
			pango_font_description_set_absolute_size(fontDescription, (fs) * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);

			mNeedsRefit = false;
			mNeedsMaxResizeFontSizeUpdate = false;
			mNeedsTextRender = true;
		}
		//-----------------------------------------------------------
	}
}

bool Text::measurePangoText() {
	if(mNeedsFontUpdate || mNeedsMeasuring || mNeedsTextRender || mNeedsMarkupDetection) {
		
		
		if(mText.empty() || mTextSize <= 0.0f){
			if(mWidth > 0.0f || mHeight > 0.0f){
				setSize(0.0f, 0.0f);
			}
			mNeedsMarkupDetection = false;
			mNeedsMeasuring = false;
			mNeedsBatchUpdate = true;
			return false;
		}

		mNeedsTextRender = true;
		bool hadMarkup = mProbablyHasMarkup;

		if(mNeedsMarkupDetection) {

			// Pango doesn't support HTML-esque line-break tags, so
			// find break marks and replace with newlines, e.g. <br>, <BR>, <br />, <BR />
			std::regex e("<br\\s?/?>", std::regex_constants::icase);
			mProcessedText = std::regex_replace(mText, e, "\n");

			// Let's also decide and flag if there's markup in this string
			// Faster to use pango_layout_set_text than pango_layout_set_markup later on if
			// there's no markup to bother with.
			// Be pretty liberal, there's more harm in false-postives than false-negatives
			mProbablyHasMarkup =  ((mProcessedText.find("<") != std::wstring::npos) && (mProcessedText.find(">") != std::wstring::npos)) || mProcessedText.find("&amp;") != std::wstring::npos;

			mNeedsMarkupDetection = false;
		}

		// First run, and then if the fonts change
		if(mNeedsFontOptionUpdate) {
			// TODO, expose these?

			cairo_font_options_set_antialias(mCairoFontOptions, CAIRO_ANTIALIAS_SUBPIXEL);
			cairo_font_options_set_hint_style(mCairoFontOptions, CAIRO_HINT_STYLE_DEFAULT);
			cairo_font_options_set_hint_metrics(mCairoFontOptions, CAIRO_HINT_METRICS_ON);
			cairo_font_options_set_subpixel_order(mCairoFontOptions, CAIRO_SUBPIXEL_ORDER_RGB);

			pango_cairo_context_set_font_options(mPangoContext, mCairoFontOptions);

			mNeedsFontOptionUpdate = false;
		}

		if(mNeedsFontUpdate || mNeedsFontSizeUpdate) {

			PangoFontDescription* fontDescription = pango_font_description_from_string(mTextFont.c_str());// +" " + std::to_string(mTextSize)).c_str());
			pango_font_description_set_absolute_size(fontDescription, mTextSize * 1.3333333333333 * 1024.0);
			pango_layout_set_font_description(mPangoLayout, fontDescription);
			if (mNeedsFontUpdate) {
				pango_font_map_load_font(mEngine.getPangoFontService().getPangoFontMap(), mPangoContext, fontDescription);
			}
			pango_font_description_free(fontDescription);

			mNeedsFontUpdate = false;
			mNeedsFontSizeUpdate = false;
		}

		


		// If the text or the bounds change
		if(mNeedsMeasuring) {
			const int lastPixelWidth = mPixelWidth;
			const int lastPixelHeight = mPixelHeight;

			if (mWrapMode != WrapMode::kWrapModeOff) {
				pango_layout_set_width(mPangoLayout, (int)mResizeLimitWidth * PANGO_SCALE);
			} else
			{
				pango_layout_set_width(mPangoLayout, -1);
			}
			if(mResizeLimitHeight < 0) {
				pango_layout_set_height(mPangoLayout, (int)mResizeLimitHeight);
			} else {
				pango_layout_set_height(mPangoLayout, (int)mResizeLimitHeight * PANGO_SCALE);
			}

			// Pango separates alignment and justification... I prefer a simpler API here to handling certain edge cases.
			if(mTextAlignment == Alignment::kJustify) {
				pango_layout_set_justify(mPangoLayout, true);
				pango_layout_set_alignment(mPangoLayout, PANGO_ALIGN_LEFT);
			} else {
				PangoAlignment aligny = PANGO_ALIGN_LEFT;
				if(mTextAlignment == Alignment::kCenter){
					aligny = PANGO_ALIGN_CENTER;
				} else if(mTextAlignment == Alignment::kRight){
					aligny = PANGO_ALIGN_RIGHT;
				} else if(mTextAlignment == Alignment::kJustify){ // handled above, but just to be safe
					aligny = PANGO_ALIGN_LEFT;
				}

				pango_layout_set_justify(mPangoLayout, false);
				pango_layout_set_alignment(mPangoLayout, aligny);
			}

			if(mWrapMode == WrapMode::kWrapModeChar){
				pango_layout_set_wrap(mPangoLayout, PANGO_WRAP_CHAR);
			} else if(mWrapMode == WrapMode::kWrapModeWord){
				pango_layout_set_wrap(mPangoLayout, PANGO_WRAP_WORD);
			} else {
				pango_layout_set_wrap(mPangoLayout, PANGO_WRAP_WORD_CHAR);
			}

			PangoEllipsizeMode elipsizeMode = PANGO_ELLIPSIZE_NONE;
			if(mEllipsizeMode == EllipsizeMode::kEllipsizeEnd){
				elipsizeMode = PANGO_ELLIPSIZE_END;
			} else if(mEllipsizeMode == EllipsizeMode::kEllipsizeMiddle){
				elipsizeMode = PANGO_ELLIPSIZE_MIDDLE;
			} else if(mEllipsizeMode == EllipsizeMode::kEllipsizeStart){
				elipsizeMode = PANGO_ELLIPSIZE_START;
			}

			pango_layout_set_ellipsize(mPangoLayout, elipsizeMode);
			pango_layout_set_spacing(mPangoLayout, (int)(mTextSize * (mLeading - 1.0f)) * PANGO_SCALE);

			// Set text, use the fastest method depending on what we found in the text
			int newPixelWidth = 0;
			int newPixelHeight = 0;
			if(mProbablyHasMarkup){
				pango_layout_set_markup(mPangoLayout, mProcessedText.c_str(), static_cast<int>(mProcessedText.size()));

				// check the pixel size, if it's empty, then we can try again without markup
				pango_layout_get_pixel_size(mPangoLayout, &newPixelWidth, &newPixelHeight);
			}

			if(!mProbablyHasMarkup || newPixelWidth < 1) {
				if(hadMarkup){
					pango_layout_set_markup(mPangoLayout, mProcessedText.c_str(), static_cast<int>(mProcessedText.size()));
				}
				pango_layout_set_text(mPangoLayout, mProcessedText.c_str(), -1);
			}

			if(mLetterSpacing != 0.0f) {
				auto attrs = pango_layout_get_attributes(mPangoLayout);
				bool createdNew = false;
				if(attrs == nullptr) {
					attrs = pango_attr_list_new();
					createdNew = true;
				}

				// Set letter spacing: 0.0f=normal; 1.0f = 1pt extra spacing;
				pango_attr_list_insert(attrs, pango_attr_letter_spacing_new((int)(mLetterSpacing * PANGO_SCALE)));

				// Enable ligatures, kerning, and auto-conversion of simple fractions to a single character representation
				//pango_attr_list_insert(attrs, pango_attr_font_features_new("liga=1, -kern, afrc on, frac on"));

				pango_layout_set_attributes(mPangoLayout, attrs);

				if(createdNew) {
					pango_attr_list_unref(attrs);
				}
			}

			//If we are sizing for limits we do that logic here after all the attributes have be set.
			//at the end of this only the font size should be changed.
			findFitFontSize();
			

			mWrappedText = pango_layout_is_wrapped(mPangoLayout) != FALSE;
			mNumberOfLines = pango_layout_get_line_count(mPangoLayout);
			

			// use this instead: pango_layout_get_pixel_extents
			PangoRectangle extentRect = PangoRectangle();
			PangoRectangle inkRect = PangoRectangle();
			pango_layout_get_pixel_extents(mPangoLayout, &inkRect, &extentRect);

			// The offset for rendering to the cairo surface
			mPixelOffsetX = -extentRect.x;
			mPixelOffsetY = -extentRect.y;

			// Instead of making the image textue larger, we will offset the drawing to the correct position
			mRenderOffset = ci::vec2(extentRect.x, extentRect.y);

			// To account for the case where the inkRect goes outside of the extentRect:
			//   move the cairo & render offsets appropriately by opposite amounts
			if (inkRect.x < extentRect.x) {
				mRenderOffset.x -= extentRect.x - inkRect.x;
				mPixelOffsetX += extentRect.x - inkRect.x;
			}

			if (inkRect.y < extentRect.y) {
				mRenderOffset.y -= extentRect.y - inkRect.y;
				mPixelOffsetY += extentRect.y - inkRect.y;
			}

			if((extentRect.width == 0 || extentRect.height == 0) && !mText.empty()){
				DS_LOG_WARNING("No size detected for pango text size. Font not detected or invalid markup are likely causes. Text: " << getTextAsString());
			}
			
			// DS_LOG_INFO("the Text: " << getTextAsString());
			// DS_LOG_INFO("\nInk rect: " << inkRect.x << " " << inkRect.y << " " << inkRect.width << " " << inkRect.height);
			// DS_LOG_INFO("Ext rect: " << extentRect.x << " " << extentRect.y << " " << extentRect.width << " " << extentRect.height << "\n");

			// Set the final width/height for the texture, handling the case where inkRect is larger than extentRect
			mPixelWidth = std::max(extentRect.width, inkRect.width);
			mPixelHeight = std::max(extentRect.height, inkRect.height);

			// This is required to not break combinations of layout align & text align
			if (extentRect.width < (int)mResizeLimitWidth) {
				if(!mShrinkToBounds){
					setSize(mResizeLimitWidth, (float)mPixelHeight);
				}else{
					mRenderOffset.x -= extentRect.x;
					setSize((float)mPixelWidth, (float)mPixelHeight);
				}
			} else {
				setSize((float)mPixelWidth, (float)mPixelHeight);
			}


			mNeedsMeasuring = false;

		}
		
		mNeedsBatchUpdate = true;
		return true;
	} else {
		return false;
	}
}

void Text::renderPangoText(){
	if(mNeedsTextRender && mPixelWidth > 0 && mPixelHeight > 0) {
		// Create appropriately sized cairo surface
		const bool grayscale = false; // Not really supported
		_cairo_format cairoFormat = grayscale ? CAIRO_FORMAT_A8 : CAIRO_FORMAT_ARGB32;


		cairo_surface_t* cairoSurface = cairo_image_surface_create(cairoFormat, mPixelWidth, mPixelHeight);

		auto cairoSurfaceStatus = cairo_surface_status(cairoSurface);
		if(CAIRO_STATUS_SUCCESS != cairoSurfaceStatus) {
			DS_LOG_WARNING("Error creating Cairo surface. Status:" << cairoSurfaceStatus << " w:" << mPixelWidth << " h:" << mPixelHeight << " text:" << mText);
			// make sure we don't render garbage
			if(mTexture){
				mTexture = nullptr;
			}
			return;
		}

		cairo_t* cairoContext = nullptr;
		if(cairoSurface){
			// Create context
			cairoContext = cairo_create(cairoSurface);

			auto cairoStatus = cairo_status(cairoContext);

			if(CAIRO_STATUS_NO_MEMORY == cairoStatus) {
				DS_LOG_WARNING("Out of memory, error creating Cairo context");
				cairo_surface_destroy(cairoSurface);
				return;
			}

			if(CAIRO_STATUS_SUCCESS != cairoStatus){
				DS_LOG_WARNING("Error creating Cairo context " << cairoStatus);
				cairo_surface_destroy(cairoSurface);
				return;
			}
		}

		if(cairoContext) {

			// Draw the text into the buffer
			cairo_set_source_rgb(cairoContext, mTextColor.r, mTextColor.g, mTextColor.b);

			// Move the layout into the correct position on the surface/context before drawing!
			// This removes the need for additional texture padding & fixes clipping ascenders
			cairo_translate(cairoContext, mPixelOffsetX, mPixelOffsetY);
			pango_cairo_update_layout(cairoContext, mPangoLayout);
			pango_cairo_show_layout(cairoContext, mPangoLayout);

			//	cairo_surface_write_to_png(cairoSurface, "test_font.png");

			// Copy it out to a texture
			unsigned char *pixels = cairo_image_surface_get_data(cairoSurface);

			ci::gl::Texture::Format format;
			format.enableMipmapping(true);
			//format.setMagFilter(GL_NEAREST);
			//format.setMinFilter(GL_NEAREST);
			mTexture = ci::gl::Texture::create(pixels, GL_BGRA, mPixelWidth, mPixelHeight, format);
			mTexture->setTopDown(true);
			mNeedsTextRender = false;

			cairo_destroy(cairoContext);			
		}


		if(cairoSurface) {
			cairo_surface_destroy(cairoSurface);
		}
	} 
}

void Text::writeAttributesTo(ds::DataBuffer& buf){
	ds::ui::Sprite::writeAttributesTo(buf);

	if(mDirty.has(TEXT_DIRTY)){
		buf.add(TEXT_ATT);
		buf.add(mText);
	}

	if(mDirty.has(FONT_DIRTY)) {
		buf.add(FONTNAME_ATT);
		buf.add(mTextFont);
		buf.add(mTextSize);
		buf.add(mLeading);
		buf.add(mLetterSpacing);
		buf.add(mTextColor);
		buf.add((int)mTextAlignment);
	}
	if(mDirty.has(LAYOUT_DIRTY)) {
		buf.add(LAYOUT_ATT);
		buf.add(mResizeLimitWidth);
		buf.add(mResizeLimitHeight);
		buf.add(mFitToResizeLimit);
		buf.add((int)mFontSizes.size());
		for (auto font_size : mFontSizes) {
			buf.add(font_size);
		}
	}
}

void Text::readAttributeFrom(const char attributeId, ds::DataBuffer& buf){
	if(attributeId == TEXT_ATT) {
		setText(buf.read<std::string>());
	} else if(attributeId == FONTNAME_ATT) {

		std::string fontName = buf.read<std::string>();
		double fontSize = buf.read<double>();
		float leading = buf.read<float>();
		float letterSpacing = buf.read<float>();
		ci::Color fontColor = buf.read<ci::Color>();
		auto alignment = (ds::ui::Alignment::Enum)(buf.read<int>());

		setFont(fontName, fontSize);
		setLeading(leading);
		setLetterSpacing(letterSpacing);
		setTextColor(fontColor);
		setAlignment(alignment);

	} else if(attributeId == LAYOUT_ATT) {
		float rsw = buf.read<float>();
		float rsh = buf.read<float>();
		bool fit = buf.read<bool>();
		int font_count = buf.read<int>();
		std::vector<double> fontSizes;
		for(int i=0;i<font_count;i++)
		{
			auto font_size = buf.read<double>();
			fontSizes.push_back(font_size);
		}
		
		setResizeLimit(rsw, rsh);
		setFitToResizeLimit(fit);
		setFitFontSizes(fontSizes);
	} else {
		ds::ui::Sprite::readAttributeFrom(attributeId, buf);
	}
}

} // namespace ui
} // namespace ds
