#pragma once

#include "view_port.h"
#include "lvgl.h"
#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const NbApp gui_app;

/** Canvas Orientation */
typedef enum {
    CanvasOrientationHorizontal,
    CanvasOrientationHorizontalFlip,
    CanvasOrientationVertical,
    CanvasOrientationVerticalFlip,
} CanvasOrientation;

/** Gui layers */
typedef enum {
    GuiLayerDesktop, /**< Desktop layer for internal use. Like fullscreen but with status bar */

    GuiLayerWindow, /**< Window layer, status bar is shown */

    GuiLayerStatusBarLeft, /**< Status bar left-side layer, auto-layout */
    GuiLayerStatusBarRight, /**< Status bar right-side layer, auto-layout */

    GuiLayerFullscreen, /**< Fullscreen layer, no status bar */

    GuiLayerMAX /**< Don't use or move, special value */
} GuiLayer;

/** Gui Canvas Commit Callback */
typedef void (*GuiCanvasCommitCallback)(
    uint8_t* data,
    size_t size,
    CanvasOrientation orientation,
    void* context);

#define RECORD_GUI "gui"

typedef struct NbGui NbGui;

/** Add view_port to view_port tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 * @param[in]  layer      GuiLayer where to place view_port
 */
void gui_add_view_port(NbGui* gui, ViewPort* view_port, GuiLayer layer);

/** Remove view_port from rendering tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_remove_view_port(NbGui* gui, ViewPort* view_port);

/** Send ViewPort to the front
 *
 * Places selected ViewPort to the top of the drawing stack
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_view_port_send_to_front(NbGui* gui, ViewPort* view_port);

/** Send ViewPort to the back
 *
 * Places selected ViewPort to the bottom of the drawing stack
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_view_port_send_to_back(NbGui* gui, ViewPort* view_port);

/** Set lockdown mode
 *
 * When lockdown mode is enabled, only GuiLayerDesktop is shown.
 * This feature prevents services from showing sensitive information when flipper is locked.
 *
 * @param      gui       Gui instance
 * @param      lockdown  bool, true if enabled
 */
void gui_set_lockdown(NbGui* gui, bool lockdown);

#ifdef __cplusplus
}
#endif
