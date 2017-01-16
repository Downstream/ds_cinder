#pragma once
#ifndef DS_UI_DRAWING_DRAWING_CANVAS
#define DS_UI_DRAWING_DRAWING_CANVAS

#include <deque>
#include <utility>

#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/image.h>
#include "cinder/gl/Texture.h"
#include "ds/ui/sprite/shader/sprite_shader.h"
#include <ds/ui/sprite/fbo/fbo.h>
#include <ds/ui/image_source/image_owner.h>

namespace ds {
namespace ui {

class Globals;

/**
* \class dev::DrawingCanvas
*			A view that you can touch to draw on
*/
class DrawingCanvas
	: public ds::ui::Sprite
	, public ImageOwner
{

public:
	DrawingCanvas(ds::ui::SpriteEngine& eng, const std::string& brushImagePath="");


	/// The color and opacity are mixed together, though these setters may overwrite others' settings
	void								setBrushColor(const ci::ColorA& brushColor);
	void								setBrushColor(const ci::Color& brushColor);
	void								setBrushOpacity(const float brushOpacity);
	const ci::ColorA&					getBrushColor();

	/// Resizes the brush image to this pixel width and scales the height proportionally
	void								setBrushSize(const float brushSize);
	const float							getBrushSize();

	/// Draws a line from start to end with an instance of the brush texture at every pixel
	void								renderLine(const ci::vec3& start, const ci::vec3& end);

	/// Loads an image file to use for the brush
	void								setBrushImage(const std::string& imagePath);

	/// Clears any drawings a person has made
	void								clearCanvas();

	/// If true, will erase instead of drawing
	void								setEraseMode(const bool eraseMode);

	// Static Client/Server Blob registration
	static void							installAsServer( ds::BlobRegistry& );
	static void							installAsClient( ds::BlobRegistry& );


protected:
	// Queue to store points as they're drawn.  This gets serialized to clients.
	typedef std::deque< std::pair<ci::vec2, ci::vec2> > PointsQueue;
	PointsQueue							mSerializedPointsQueue;

	virtual void						drawLocalClient() override;
	virtual void						writeAttributesTo(DataBuffer&) override;
	virtual void						readAttributeFrom(const char, DataBuffer&) override;
	virtual void						onImageChanged() override;

private:

	// The shader that colorizes the brush image
	ds::ui::SpriteShader				mPointShader;
	// The intermediate fbo that brushes are drawn to
	std::unique_ptr<ds::ui::FboGeneral>	mFboGeneral;
	// The texture drawn to the screen and drawn on
	ci::gl::Texture2dRef				mDrawTexture;

	SpriteShader						mOutputShader;

	ds::ui::Image*						mBrushImage;
	float								mBrushSize;
	ci::ColorA							mBrushColor;
	bool								mEraseMode;
};

} // namespace ui
} // namespace ds

#endif
