// Copyright (C) 2016 David Reid. See included LICENSE file.

// These #defines enable us to load large files on Linux platforms. They need to be placed before including any headers.
#ifndef _WIN32
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif


// Standard headers.
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// Platform headers.
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <sys/file.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <fontconfig/fontconfig.h>
#endif

// Platform libraries, for simplifying MSVC builds.
#ifdef _WIN32
#if defined(_MSC_VER)
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#endif
#endif


// External libraries.

// TODO: Remove dr.h from the public header. This is only required because of dr_cmdline which is a member of dred_context.
#ifdef DRED_USE_EXTERNAL_REPOS
#include "../../../../dr_libs/dr.h"
#else
#include "../external/dr.h"
#endif

#ifdef DRED_USE_EXTERNAL_REPOS
#include "../../../dr_ipc/dr_ipc.h"
#else
#include "../external/dr_ipc.h"
#endif

#include "../external/dr_2d.h"
#include "../external/dr_text_engine.h"


#ifdef _MSC_VER
#define DRED_INLINE static __forceinline
#else
#define DRED_INLINE static inline
#endif

// dred header files.
#include "dred_autogenerated.h"
#include "dred_build_config.h"
#include "dred_types.h"
#include "dred_threading.h"
#include "dred_ipc.h"
#include "gui/dred_gui.h"
#include "gui/dred_scrollbar.h"
#include "gui/dred_tabbar.h"
#include "gui/dred_button.h"
#include "gui/dred_color_button.h"
#include "gui/dred_checkbox.h"
#include "gui/dred_tabgroup.h"
#include "gui/dred_tabgroup_container.h"
#include "gui/dred_textview.h"
#include "gui/dred_textbox.h"
#include "gui/dred_info_bar.h"
#include "gui/dred_cmdbar.h"
#include "dred_fs.h"
#include "dred_alias_map.h"
#include "dred_config.h"
#include "dred_accelerators.h"
#include "dred_shortcuts.h"
#include "dred_editor.h"
#include "dred_settings_editor.h"
#include "dred_highlighters.h"
#include "dred_text_editor.h"
#include "dred_font.h"
#include "dred_font_library.h"
#include "dred_image.h"
#include "dred_image_library.h"
#include "dred_menus.h"
#include "dred_about_dialog.h"
#include "dred_settings_dialog.h"
#include "dred_printing.h"
#include "dred_context.h"
#include "dred_platform_layer.h"
#include "dred_commands.h"
#include "dred_misc.h"
#include "dred_stock_themes.h"
#include "dred_codegen.h"
#include "dred_package.h"
