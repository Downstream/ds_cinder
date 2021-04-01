#pragma once

#include "ds/ui/sprite/sprite.h"
#include "ds/ui/sprite/text_defs.h"
#include <cinder/gl/Texture.h>

// Forward declare Pango/Cairo structs
struct 			_PangoContext;
struct 			_PangoLayout;
struct 			_PangoFontDescription;
struct 			_cairo_surface;
struct 			_cairo;
struct 			_cairo_font_options;
typedef struct	_PangoContext PangoContext;
typedef struct 	_PangoLayout PangoLayout;
typedef struct 	_PangoFontDescription PangoFontDescription;
typedef struct 	_cairo_surface cairo_surface_t;
typedef struct 	_cairo cairo_t;
typedef struct 	_cairo_font_options cairo_font_options_t;

namespace ds {
namespace ui {


/**
*	\class Text
*	\brief A sprite for displaying text.
*	Uses Pango and Cairo for layout and rendering instead of the previous OGLFT implementation
*	The current implementation requires fonts to be installed system-wide. (there seems to be a bug in fontconfig/pango loading a font from a file)
*	You can specify some simple markup for pango text. See this page: http://www.gtk.org/api/2.6/pango/PangoMarkupFormat.html
*	\code
*		Basics: `<markup>` root document node (optional)
*				`<span foreground='blue' style='italic'>Some blue italic text</span>` (NOTE: you must use single 'quotes' and not double "quotes" like the documentation. Double quotes = nothing shows up)
*					Span attributes: 
*						font_desc (such as 'Sans Italic 12')
*						font_family or face (such as 'Arial' or 'Sans')
*						size ('xx-small', 'x-small', 'small', 'medium', 'large', 'x-large', 'xx-large' or 'smaller' or 'larger')
*						style (One of 'normal', 'oblique', 'italic')
*						weight (One of 'ultralight', 'light', 'normal', 'bold', 'ultrabold', 'heavy', or a numeric weight like 400 for normal or 700 for bold)
*						variant ('normal' or 'smallcaps')
*						stretch (One of 'ultracondensed', 'extracondensed', 'condensed', 'semicondensed', 'normal', 'semiexpanded', 'expanded', 'extraexpanded', 'ultraexpanded')
*						foreground (An RGB color specification such as '\#00FF00' or a color name such as 'red') NOTE: you must turn on setPreserveSpanColors(true)
*						background (An RGB color specification such as '\#00FF00' or a color name such as 'red') NOTE: you must turn on setPreserveSpanColors(true)
*						underline (One of 'single', 'double', 'low', 'none')
*						underline_color (The color of underlines; an RGB color specification such as '\#00FF00' or a color name such as 'red') NOTE: you must turn on setPreserveSpanColors(true)
*						rise (Vertical displacement, in 10000ths of an em. Can be negative for subscript, positive for superscript.)
*						strikethrough ('true' or 'false' whether to strike through the text)
*						strikethrough_color (The color of strikethrough lines; an RGB color specification such as '\#00FF00' or a color name such as 'red') NOTE: you must turn on setPreserveSpanColors(true)
*						fallback ('true' or 'false' whether to enable fallback. If disabled, then characters will only be used from the closest matching font on the system. No fallback will be done to other fonts on the system that might contain the characters in the text. Fallback is enabled by default. Most applications should not disable fallback.)
*						lang (A language code, indicating the text language)
*						letter_spacing (in 1024ths of a point)
*				`<b>` bold
*				`<big>` bigger
*				`<i>` italic
*				`<s>` strikethrough
*				`<sub>` Subscript
*				`<sup>` Superscript
*				`<small>` smaller
*				`<tt>` monospace font
*				`<u>` underline
*	\endcode
*/

class Text : public ds::ui::Sprite {
public:
	Text(ds::ui::SpriteEngine& engine);
	~Text();


	/// Sets the actual text to be rendered as a std::string or std::wstring
	/// Can be plain text or use markup (see top of header) such as <span weight='bold'>some bold text</span>
	void						setText(std::string text);
	void						setText(std::wstring text);

	/// Returns the text as a std::string
	std::string					getTextAsString() const;
	/// Returns the text as a std::wstring
	std::wstring				getText() const;
	bool						hasText() const { return !mText.empty(); }

	/// Resize limit is the amount of width the text will wrap at and the height that text will no longer be rendered
	float						getResizeLimitWidth() const;
	float						getResizeLimitHeight() const;
	Text&						setResizeLimit(const float width = 0, const float height = -1.0f);

	/// Should this sprite shrink to the bounds of the texture (as opposed to shrinking to the resize_limit)
	bool						getShrinkToBounds() const;
	void						setShrinkToBounds(const bool shrinkToBounds = false);


	/*---------- Style Parameters -------------------*/
	/// This is a convenience that can set all the text attributes at once
	void						setTextStyle(std::string font, double size, ci::ColorA color = ci::Color::white(), Alignment::Enum alignment = Alignment::kLeft);
	/// Sets all style parameters from the TextStyle struct
	void						setTextStyle(ds::ui::TextStyle theStyle);
	/// Looks up the style from the engine by name (from styles.xml) and applies it. If the style name isn't found, uses the default font.
	void						setTextStyle(std::string styleName);

	/// Returns a reference to the TextStyle struct that drives the look of the text (font, size, color, etc)
	ds::ui::TextStyle&			getTextStyle() { return mStyle; }


	/// The font name must match a family or face name installed on the system. Will warn and use a default if the desired font is not installed
	Text&						setFont(const std::string& name);
	/// Returns the font face name (e.g. Arial Bold or Times New Roman) This might return a slightly different name than you set if a different name is looked up from Pango
	std::string					getFont();


	/// Sets the default color of the text for rendering. If you're using <span> tags to change the color, you'll want to enable setPreserveSpanColors()
	void						setTextColor(const ci::Color&);
	/// All setColor()'s are identical to setTextColor()
	/// Sets the default color of the text for rendering. If you're using <span> tags to change the color, you'll want to enable setPreserveSpanColors()
	virtual void				setColor(const ci::Color&) override;
	virtual void				setColor(float r, float g, float b) override;
	virtual void				setColorA(const ci::ColorA&) override;
	/// Returns the text color without the alpha component
	ci::Color					getTextColor(){	return mStyle.mColor; };


	/// Set the font size in pixels. Font sizes are scaled by an overall multiplier (currently 1.33333 for backwards compatibility)
	void						setFontSize(double fontSize);
	double						getFontSize(){ return mStyle.mSize; }


	/// Enable or disable fitting. If on, will attempt to fit the text inside the resize limit by changing the font size
	Text&						setFitToResizeLimit(const bool fitToResize);
	/// Set a vector of font sizes to select from if using "fit to resize limit", which will size the text to fit the area
	Text&						setFitFontSizes(std::vector<double> font_sizes);
	/// Set the maximum for the font size when fitting to the resize limit
	double						getFitFontSize() { return mFitCurrentTextSize; }
	
	void						setFitMaxFontSize(double fontSize);
	/// Set the minimum for the font size when fitting to the resize limit
	void						setFitMinFontSize(double fontSize);
	/// Get the maximum for the font size when fitting to the resize limit
	double						getFitMaxFontSize() { return mStyle.mFitMaxTextSize; }
	/// Get the minimum for the font size when fitting to the resize limit
	double						getFitMinFontSize() { return mStyle.mFitMinTextSize; }


	/// Set the overall text alignment (Left, Center, Right, Justify) See text_defs.h for values
	/// Justify requires having a resize width set
	void						setAlignment(Alignment::Enum alignment);
	/// Get the overall text alignment (Left, Center, Right, Justify) See text_defs.h for values
	Alignment::Enum				getAlignment();

	/// Set the leading value (space between the lines). 
	/// The default is 1.0, which is the font's default distance between lines. 
	/// Higher than 1.0 is more space, lower than 1.0 is tighter space between the lines
	Text&						setLeading(const double);
	/// Get the leading value (space between the lines of the font). 
	double						getLeading() const;


	/// Set the letter spacing value (space between the characters). 
	/// Default is 0.0, which is the font's default space. 
	/// Higher than 0.0 is more space, Less than 0 is tighter space
	Text&						setLetterSpacing(const double);
	double						getLetterSpacing() const;

	/// Whether to add ellipses to the text if it doesn't fit inside the resize limit
	/// If the resize limit is set to 0 or -1, no text wrapping will happen and no ellipses will be added
	void						setEllipsizeMode(EllipsizeMode theMode);
	EllipsizeMode				getEllipsizeMode();

	/// Sets the wrap mode. See text_defs.h for options
	/// Wraps on word, character, off, or word and character (fits the word, and if that doesn't work, splits on a character)
	void						setWrapMode(WrapMode theMode);
	WrapMode					getWrapMode();

	/*----------- End of TextStyle parameters ------- */


	/// Gets the width of the sprite, based on the actual render width of the text if shrink to bounds is on
	virtual float				getWidth() const;
	/// Gets the height of the sprite, based on the actual render height of the text if shrink to bounds is on
	virtual float				getHeight() const;

	/// If ellipsize mode is none and there's a resize width > 0 and the text had to wrap at all, returns true. otherwise false
	bool						getTextWrapped();

	/// The number of lines in the text layout
	int							getNumberOfLines();

	/// If an ordered or unordered list was detected in the current text
	bool						getHasLists();

	/// Sets the ability for the pango markup system to be used. 
	/// Default is on
	void						setAllowMarkup(const bool allow);

	/// Returns if markup is allowed or not
	bool						getAllowMarkup() { return mAllowMarkup; }

	/// Returns the 2-d position of the character in the current text string
	/// Will return 0,0 if the string is blank or the index is out-of-bounds
	/// Note that this position is fudged by 25% vertically to get the top-left corner of most characters in most fonts. 
	/// This is good for cursors. If you need the exact bounds of a character, use getRectForCharacterIndex()
	ci::vec2					getPositionForCharacterIndex(const int characterIndex);

	/// Returns the bounds of the character at the supplied index
	/// Returns 0,0,0,0 if the string is blank of the index is out of bounds
	/// Note that this box is likely taller than the actual character
	/// This is just a wrapper around pango_layout_index_to_pos(), so see that documentation for full details
	ci::Rectf					getRectForCharacterIndex(const int characterIndex);

	/// Returns the index of the character of the current text string for the position supplied
	/// Gracefully handles out-of-bounds points (will always return a valid index, assuming the current text string isn't empty)
	int							getCharacterIndexForPosition(const ci::vec2& localPosition);

	/// By default, we render text using the color from this sprite with the alpha values from pango
	/// If you turn this on, we also use the colors from pango and multiply in the color from this sprite
	/// This is for instances where you're using a color in a span tag in the markup
	/// The default is that this is false, which renders text a little more cleanly.
	void						setPreserveSpanColors(const bool preserve);
	const bool					getPreserveSpanColors() { return mPreserveSpanColors; }

	/// Text is rendered into this texture
	/// Note: this texture has pre-multiplied alpha
	const ci::gl::TextureRef	getTexture();

	/// Registers this class to be net-sync capable
	static void					installAsServer(ds::BlobRegistry&);
	static void					installAsClient(ds::BlobRegistry&);

protected:

	virtual void				onUpdateClient(const UpdateParams &updateParams) override;
	virtual void				onUpdateServer(const UpdateParams&) override;

	void						drawLocalClient();

	virtual void				writeAttributesTo(ds::DataBuffer&);
	virtual void				readAttributeFrom(const char attributeId, ds::DataBuffer&);

	virtual	void				onBuildRenderBatch() override;

	//picks a font size that fits the whole text inside resize limit rect;
	void						findFitFontSize();
	void 						findFitFontSizeFromArray();

	/// Pulls out <ol> and <ul> tags and creates the lists, returns true if there are more lists to parse
	bool parseLists();
	/// puts the layout into pango, updates any layout stuff, and measures the result
	/// This is a pre-requisite for drawPangoText().
	/// Returns true if the text was updated and needs a rendering
	bool 						measurePangoText();

	/// Renders text into the texture.
	void 						renderPangoText();
	//pango references;
	PangoContext*				mPangoContext;
	PangoLayout*				mPangoLayout;

	/// The GL texture of the text after it's rendered
	ci::gl::TextureRef			mTexture;

	/// The text for this text to display as output text
	std::string					mText;

	/// Stores text after newline filtering
	std::string					mProcessedText; 
	bool						mProbablyHasMarkup;
	bool						mAllowMarkup;

	/// See text_defs.h for definition of style params (font name, size, alignment, leading, letter spacing, fit sizes)
	TextStyle					mStyle;

	/// Rendering options
	bool						mPreserveSpanColors;
	EllipsizeMode				mEllipsizeMode;
	WrapMode					mWrapMode;
	double						mEngineFontScale;

	// Resize options
	bool						mShrinkToBounds;
	float						mResizeLimitWidth,
								mResizeLimitHeight;
	/// Fit params
	bool						mFitToResizeLimit;
	bool						mNeedsMaxResizeFontSizeUpdate;
	bool						mNeedsRefit;
	int							mMaxResizeFontSize;
	double						mFitCurrentTextSize;

	/// Info about the text layout
	bool						mWrappedText;
	int							mNumberOfLines;
	bool						mHasLists;

	/// Internal flags for state invalidation
	/// Used by measure and render methods
	bool						mNeedsFontUpdate;
	bool						mNeedsFontSizeUpdate;
	bool 						mNeedsMeasuring;
	bool 						mNeedsTextRender;
	bool 						mNeedsFontOptionUpdate;
	bool 						mNeedsMarkupDetection;	

	/// simply stored to check for change across renders
	int 						mPixelWidth;
	int							mPixelHeight;

	/// Offsets for rendering to cairo surface
	int							mPixelOffsetX;
	int							mPixelOffsetY;
	/// Offset for rendering to the screen
	ci::vec2					mRenderOffset;

	/// Pango and cairo references
	
	cairo_font_options_t*		mCairoFontOptions;
	
};
}
} // namespace kp::pango