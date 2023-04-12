#pragma once
#define WLR_USE_UNSTABLE

#include <types.hpp>

#include <cairo/cairo.h>

#include <src/helpers/Color.hpp>
#include <src/helpers/Monitor.hpp>
#include <src/render/Texture.hpp>
#include <src/helpers/BezierCurve.hpp>

namespace hyprload::overlay {
    const CColor s_overlayBoxColor = {0x10 / 255.0f, 0x10 / 255.0f, 0x20 / 255.0f, 1.0f};
    const CColor s_overlayBoxHighlight = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};
    const CColor s_overlayTextColor = {0xff / 255.0f, 0xff / 255.0f, 0xff / 255.0f, 1.0f};

    const std::string c_overlayAnimationCurve = "plugin:hyprload:overlay:animation_curve";
    const std::string c_overlayAnimationDuration = "plugin:hyprload:overlay:animation_duration";

    CBezierCurve* getAnimationCurve();
    f32 getAnimationDuration();

    class HyprloadOverlay {
      public:
        HyprloadOverlay();

        void drawOverlay(CMonitor* monitor, timespec* time);

        void setDrawOverlay(bool bDrawOverlay);
        bool getDrawOverlay();
        void toggleDrawOverlay();

      private:
        bool m_bDrawOverlay = false;
        f32 m_fProgress = 0.0f;

        bool m_bDamagedAfterLastDraw = false;
        wlr_box doDrawOverlay(CMonitor* pMonitor);

        cairo_surface_t* m_pCairoSurface = nullptr;
        cairo_t* m_pCairo = nullptr;

        CMonitor* m_pLastMonitor = nullptr;

        CTexture m_tTexture;
        wlr_box m_bLastDamage;
        f32 m_fLastDrawTime = 0.0f;
    };

    inline std::unique_ptr<HyprloadOverlay> g_pOverlay;
}
