#include "engine_renderer_discontinuous.h"

#include <cinder/gl/Fbo.h>

#include <ds/app/engine/engine.h>
#include <ds/app/engine/engine_data.h>
#include <ds/app/engine/engine_roots.h>

namespace ds
{

EngineRendererDiscontinuous::EngineRendererDiscontinuous(Engine& e)
	: EngineRenderer(e)
{
	ci::gl::Fbo::Format fmt;
	fmt.setColorTextureFormat(ci::);

	const auto w = static_cast<int>(e.getWidth());
	const auto h = static_cast<int>(e.getHeight());

	mFbo = ci::gl::Fbo::create(w, h, fmt);
}

void EngineRendererDiscontinuous::drawClient()
{
	auto framebufferSave = ci::gl::ScopedFramebuffer(mFbo);
	//ci::gl::SaveFramebufferBinding bindingSaver;
	mFbo.bindFramebuffer();
	ci::gl::enableAlphaBlending();
	clearScreen();
	for (auto it = mEngine.getRoots().cbegin(), end = mEngine.getRoots().cend(); it != end; ++it) {
		(*it)->drawClient(mEngine.getDrawParams(), mEngine.getAutoDrawService());
	}
	mFbo.unbindFramebuffer();

	clearScreen();
	for (const auto& world_slice : mEngine.getEngineData().mWorldSlices)
	{
		ci::gl::draw(mFbo.getTexture(), world_slice.first, world_slice.second);
	}
}

void EngineRendererDiscontinuous::drawServer()
{
	//ALPHA TEST needs to be moved to shader, as modern open gl doesnt support it
	ci::gl::enable(GL_ALPHA_TEST, true);
	glAlphaFunc(GL_GREATER, 0.001f);

	ci::gl::enable(GL_ALPHA_TEST);
	ci::gl::enableAlphaBlending();
	clearScreen();

	for (auto it = mEngine.getRoots().cbegin(), end = mEngine.getRoots().cend(); it != end; ++it)
	{
		(*it)->drawServer(mEngine.getDrawParams());
	}

	glAlphaFunc(GL_ALWAYS, 0.001f);
}

}