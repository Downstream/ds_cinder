#include "stdafx.h"
#pragma comment (lib, "D3D11.lib")
// Loosley based on CEF 3.3578.1860  tests\cefclient\browser\osr_render_handler_win_d3d11
#include "accelerated_render_handler.h"
#include "web_handler.h"

#define NOGLDEBUG
// DCHECK on gl errors.
#ifndef NOGLDEBUG
#define VERIFY_NO_ERROR { \
int _gl_error = glGetError(); \
DCHECK(_gl_error == GL_NO_ERROR) << \
"glGetError returned " << _gl_error; \
}
#else
#define VERIFY_NO_ERROR
#endif
namespace ds {
	namespace web {
		BrowserLayer::BrowserLayer(const std::shared_ptr<d3d11::Device>& device)
			: d3d11::Layer(device) {
		}



		void BrowserLayer::on_paint(void* share_handle) {
			frame_buffer_->on_paint(share_handle);
		}

		std::pair<uint32_t, uint32_t> BrowserLayer::texture_size() const {
			const auto texture = frame_buffer_->texture();
			return std::make_pair(texture->width(), texture->height());
		}

		// currently PopupLayer never gets called, would need to implement CreatePopupWindow() like cefclient
		// and then take notice of bounds when drawing
		PopupLayer::PopupLayer(const std::shared_ptr<d3d11::Device>& device)
			: BrowserLayer(device) {
		}

		void PopupLayer::set_bounds(const CefRect& bounds) {
			const auto comp = composition();
			if (!comp)
				return;

			const auto outer_width = comp->width();
			const auto outer_height = comp->height();
			if (outer_width == 0 || outer_height == 0)
				return;

			original_bounds_ = bounds;
			bounds_ = bounds;

			// If x or y are negative, move them to 0.
			if (bounds_.x < 0)
				bounds_.x = 0;
			if (bounds_.y < 0)
				bounds_.y = 0;
			// If popup goes outside the view, try to reposition origin
			if (bounds_.x + bounds_.width > outer_width)
				bounds_.x = outer_width - bounds_.width;
			if (bounds_.y + bounds_.height > outer_height)
				bounds_.y = outer_height - bounds_.height;
			// If x or y became negative, move them to 0 again.
			if (bounds_.x < 0)
				bounds_.x = 0;
			if (bounds_.y < 0)
				bounds_.y = 0;

			const auto x = bounds_.x / float(outer_width);
			const auto y = bounds_.y / float(outer_height);
			const auto w = bounds_.width / float(outer_width);
			const auto h = bounds_.height / float(outer_height);
			move(x, y, w, h);
		}

		//--------------------------------------------------------------
		float AcceleratedRenderHandler::fps() {
			return fps_;
		}

		//--------------------------------------------------------------
		AcceleratedRenderHandler::AcceleratedRenderHandler(CefRefPtr<WebHandler> webHandler):mClient(webHandler) {
			
			transparent_ = true;
			
			width_ = 1920;
			height_ = 1080;

			// create a D3D11 device for sharing texture with openGL
			d3d11_device_ = d3d11::Device::create();
			if (!d3d11_device_) assert(0);

			// Create the browser layer and its openGL texture
			browser_layer_ = std::make_shared<BrowserLayer>(d3d11_device_);
			browser_layer_->setTexture(width_, height_);

			// Set up the composition.
			composition_ = std::make_shared<d3d11::Composition>(d3d11_device_, width_, height_);
			composition_->add_layer(browser_layer_);

			// Size to the whole composition.
			browser_layer_->move(0.0f, 0.0f, 1.0f, 1.0f);
			frame_ = 0;
			//frame_timer_.start();
			fstart_time = std::chrono::steady_clock::now();
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::reshape(int width, int height) {
			width_ = width;
			height_ = height;
			composition_->resize(width, height);
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) {
			if (show) {
				// Create a new layer.
				popup_layer_ = std::make_shared<PopupLayer>(d3d11_device_);
				composition_->add_layer(popup_layer_);
			}
			else {
				composition_->remove_layer(popup_layer_);
				popup_layer_ = nullptr;
			}
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) {
			popup_layer_->set_bounds(rect);
		}
		//--------------------------------------------------------------
		bool AcceleratedRenderHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
			return true;
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
			rect = CefRect(0, 0, width_, height_);
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::OnAcceleratedPaint(
			CefRefPtr<CefBrowser> browser,
			PaintElementType type,
			const RectList& dirtyRects,
			void* share_handle) {

			if (type == PET_POPUP) {
				popup_layer_->on_paint(share_handle);
			}
			else {
				browser_layer_->on_paint(share_handle);

				frame_++;
				fend_time = std::chrono::steady_clock::now();
				std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(fend_time - fstart_time);
				double seconds = ((double)ms.count()) / std::chrono::microseconds::period::den;
				if (seconds >= 1.0) {
					fps_ = frame_;
					frame_ = 0;
					fstart_time = std::chrono::steady_clock::now();
				}

			}
		}

		//--------------------------------------------------------------
		void AcceleratedRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,
			PaintElementType type,
			const RectList &dirtyRects,
			const void* buffer,
			int width,
			int height) {
			// Not used with this implementation.
		}
		//--------------------------------------------------------------
		void AcceleratedRenderHandler::draw() {
			composition_->draw();
		}
	}
}
