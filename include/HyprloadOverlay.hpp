#pragma once
#define WLR_USE_UNSTABLE

#include <cairo/cairo.h>

#include <src/helpers/Color.hpp>
#include <src/helpers/Monitor.hpp>
#include <src/render/Texture.hpp>

namespace hyprload {
    const CColor s_overlayBoxColor = {0x10 / 255.0f, 0x10 / 255.0f, 0x20 / 255.0f, 1.0f};
    const CColor s_overlayBoxHighlight = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};
    const CColor s_overlayTextColor = {0xff / 255.0f, 0xff / 255.0f, 0xff / 255.0f, 1.0f};

    class HyprloadOverlay {
      public:
        bool m_bDrawOverlay = false;

        HyprloadOverlay();

        void drawOverlay(const int& monitor, timespec* time);

      private:
        bool m_bDamagedAfterLastDraw = false;
        wlr_box doDrawOverlay(CMonitor* pMonitor);

        cairo_surface_t* m_pCairoSurface = nullptr;
        cairo_t* m_pCairo = nullptr;

        CMonitor* m_pLastMonitor = nullptr;

        CTexture m_tTexture;
        wlr_box m_bLastDamage;
    };

    inline std::unique_ptr<HyprloadOverlay> g_pOverlay;
}
