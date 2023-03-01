#include "HyprloadOverlay.hpp"
#include "Hyprload.hpp"

#include "src/Compositor.hpp"

namespace hyprload {
    HyprloadOverlay::HyprloadOverlay() {
        m_bDrawOverlay = false;
    }

    void HyprloadOverlay::drawOverlay(const int& monitor, timespec* time) {
        CMonitor* pMonitor = g_pCompositor->getMonitorFromID(monitor);

        if (m_pLastMonitor != pMonitor || !m_pCairo || !m_pCairoSurface) {
            if (m_pCairo && m_pCairoSurface) {
                cairo_destroy(m_pCairo);
                cairo_surface_destroy(m_pCairoSurface);
            }
            g_pHyprRenderer->damageBox(&m_bLastDamage);

            m_pCairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pMonitor->vecPixelSize.x, pMonitor->vecPixelSize.y);
            m_pCairo = cairo_create(m_pCairoSurface);
            m_pLastMonitor = pMonitor;
        }

        // Draw the overlay
        if (!m_bDrawOverlay) {
            if (!m_bDamagedAfterLastDraw) {
                g_pHyprRenderer->damageBox(&m_bLastDamage);

                g_pCompositor->scheduleFrameForMonitor(pMonitor);

                m_bDamagedAfterLastDraw = true;
            }
            return;
        }

        // Render to the monitor

        // clear the pixmap
        cairo_save(m_pCairo);
        cairo_set_operator(m_pCairo, CAIRO_OPERATOR_CLEAR);
        cairo_paint(m_pCairo);
        cairo_restore(m_pCairo);

        cairo_surface_flush(m_pCairoSurface);

        wlr_box damage = doDrawOverlay(pMonitor);

        g_pHyprRenderer->damageBox(&m_bLastDamage);
        g_pHyprRenderer->damageBox(&damage);

        g_pCompositor->scheduleFrameForMonitor(pMonitor);

        m_bLastDamage = damage;

        // copy the data to an OpenGL texture we have
        const auto DATA = cairo_image_surface_get_data(m_pCairoSurface);
        m_tTexture.allocate();
        glBindTexture(GL_TEXTURE_2D, m_tTexture.m_iTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pMonitor->vecPixelSize.x, pMonitor->vecPixelSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);

        wlr_box pMonBox = {0, 0, static_cast<int>(pMonitor->vecPixelSize.x), static_cast<int>(pMonitor->vecPixelSize.y)};
        g_pHyprOpenGL->renderTexture(m_tTexture, &pMonBox, 1.f);

        m_bDamagedAfterLastDraw = false;
    }

    wlr_box HyprloadOverlay::doDrawOverlay(CMonitor* pMonitor) {
        const auto SCALE = pMonitor->scale;
        const auto SPACING = 10.f * SCALE;
        const auto HIGHLIGHTSIZE = 4.f * SCALE;

        const auto FONTSIZE = std::clamp((int)(13.f * ((pMonitor->vecPixelSize.x * SCALE) / 1080.f)), 8, 40);

        const auto MONSIZE = pMonitor->vecPixelSize;

        cairo_select_font_face(m_pCairo, "Hack Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(m_pCairo, FONTSIZE);

        cairo_text_extents_t cairoExtents;

        float offsetX = SPACING;
        float maxHeight = 0.f;

        const std::vector<std::string>& plugins = g_pHyprload->getLoadedPlugins();

        for (auto& plugin : plugins) {
            // first rect (bg, col)
            cairo_text_extents(m_pCairo, plugin.c_str(), &cairoExtents);

            cairo_set_source_rgba(m_pCairo, s_overlayBoxColor.r, s_overlayBoxColor.g, s_overlayBoxColor.b, s_overlayBoxColor.a);

            const auto PLUGINTABSIZE = Vector2D{cairoExtents.width + 20, cairoExtents.height + 16};

            // draw rects
            cairo_rectangle(m_pCairo, offsetX, MONSIZE.y - PLUGINTABSIZE.y, PLUGINTABSIZE.x, PLUGINTABSIZE.y);
            cairo_fill(m_pCairo);

            cairo_set_source_rgba(m_pCairo, s_overlayBoxHighlight.r, s_overlayBoxHighlight.g, s_overlayBoxHighlight.b, s_overlayBoxHighlight.a);

            cairo_rectangle(m_pCairo, offsetX, MONSIZE.y - PLUGINTABSIZE.y, PLUGINTABSIZE.x, HIGHLIGHTSIZE);
            cairo_fill(m_pCairo);

            // draw text
            cairo_set_source_rgba(m_pCairo, s_overlayTextColor.r, s_overlayTextColor.g, s_overlayTextColor.b, s_overlayTextColor.a);
            cairo_move_to(m_pCairo, offsetX + 10, MONSIZE.y - PLUGINTABSIZE.y + FONTSIZE + 4);
            cairo_show_text(m_pCairo, plugin.c_str());

            // adjust offset and move on
            offsetX += SPACING + PLUGINTABSIZE.x;

            if (PLUGINTABSIZE.y > maxHeight)
                maxHeight = PLUGINTABSIZE.y;
        }

        return wlr_box{(int)pMonitor->vecPosition.x, static_cast<int>((int)pMonitor->vecPosition.y + (int)pMonitor->vecSize.y - maxHeight), (int)offsetX, (int)maxHeight};
    }
}
