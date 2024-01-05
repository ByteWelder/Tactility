#pragma once

#include "app_manifest.h"
#include "view_port.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const AppManifest gui_app;

/** Gui layers */
typedef enum {
    GuiLayerDesktop, /**< Desktop layer for internal use. Like fullscreen but with status bar */
    GuiLayerWindow, /**< Window layer, status bar is shown */
    GuiLayerFullscreen, /**< Fullscreen layer, no status bar */
    GuiLayerMAX /**< Don't use or move, special value */
} GuiLayer;

typedef struct Gui Gui;

/** Add view_port to view_port tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 * @param[in]  layer      GuiLayer where to place view_port
 */
void gui_add_view_port(ViewPort* view_port, GuiLayer layer);

/** Remove view_port from rendering tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_remove_view_port(ViewPort* view_port);

#ifdef __cplusplus
}
#endif
