#pragma once



#include "cef_render_handler.h"
#include "osr_d3d11_gl_win.h"
#include <chrono>
//#include "winUtils.h"
namespace ds {
	namespace web {
		class WebHandler;
		class BrowserLayer : public d3d11::Layer {
		public:
			explicit BrowserLayer(const std::shared_ptr<d3d11::Device>& device);
			void on_paint(void* share_handle);

			// After calling on_paint() we can query the texture size.
			std::pair<uint32_t, uint32_t> texture_size() const;

		private:

			DISALLOW_COPY_AND_ASSIGN(BrowserLayer);
		};

		class PopupLayer : public BrowserLayer {
		public:
			explicit PopupLayer(const std::shared_ptr<d3d11::Device>& device);

			void set_bounds(const CefRect& bounds);

			bool contains(int x, int y) const { return bounds_.Contains(x, y); }
			int xoffset() const { return original_bounds_.x - bounds_.x; }
			int yoffset() const { return original_bounds_.y - bounds_.y; }

		private:
			CefRect original_bounds_;
			CefRect bounds_;

			DISALLOW_COPY_AND_ASSIGN(PopupLayer);
		};

		//--------------------------------------------------------------
		class AcceleratedRenderHandler : public CefRenderHandler {
		public:
			AcceleratedRenderHandler(CefRefPtr<WebHandler> client);

			void reshape(int width, int height);
			void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) OVERRIDE;
			void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) OVERRIDE;

			bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info);
			void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
			float fps();
			void AcceleratedRenderHandler::OnAcceleratedPaint(
				CefRefPtr<CefBrowser> browser,
				PaintElementType type,
				const RectList& dirtyRects,
				void* share_handle) override;
			void OnPaint(CefRefPtr<CefBrowser> browser,
				PaintElementType type,
				const RectList &dirtyRects,
				const void* buffer,
				int width,
				int height) override;
			const CefRect& popup_rect() const { return popup_rect_; }
			const CefRect& original_popup_rect() const { return original_popup_rect_; }
			void draw();
			bool send_begin_frame() const { return false; }

		private:
			CefRect popup_rect_;
			CefRect original_popup_rect_;

			int width_, height_;  //for browser window, different from screen window
			bool transparent_;
			bool show_update_rect;
			bool initialized_;

			uint64_t start_time_;
			uint32_t frame_ = 0;
			uint64_t fps_start_ = 0;
			float fps_;
			std::chrono::time_point<std::chrono::high_resolution_clock>  fstart_time;
			std::chrono::time_point<std::chrono::high_resolution_clock>  fend_time;


			std::shared_ptr<d3d11::Device> d3d11_device_;
			std::shared_ptr<d3d11::SwapChain> swap_chain_;
			std::shared_ptr<d3d11::Composition> composition_;
			std::shared_ptr<BrowserLayer> browser_layer_;
			std::shared_ptr<PopupLayer> popup_layer_;

			CefRefPtr<WebHandler> mClient;

			IMPLEMENT_REFCOUNTING(AcceleratedRenderHandler);
		};
	}
}


