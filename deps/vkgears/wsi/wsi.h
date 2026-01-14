/*
 * Copyright © 2023 Collabora Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WSI_H_
#define WSI_H_

#include <stdbool.h>
#include "config.h"

#include <vulkan/vulkan.h>

#if(HARDINFO2_VK_WAYLAND)
struct wsi_interface wayland_wsi_interface(void);
#endif

#if(HARDINFO2_VK_X11)
struct wsi_interface xcb_wsi_interface(void);
#endif

struct wsi_interface
get_wsi_interface(void);

struct wsi_callbacks {
   void (*resize)(int new_width, int new_height);
  //   void (*key_press)(bool down, enum wsi_key key);
   void (*exit)();
};

struct wsi_interface {
   const char *required_extension_name;

   void (*init_display)();
   void (*fini_display)();

   void (*init_window)(const char *title, int width, int height, bool fullscreen);
   bool (*update_window)();
   void (*fini_window)();

   void (*set_wsi_callbacks)(struct wsi_callbacks);

   bool (*create_surface)(VkPhysicalDevice physical_device, VkInstance instance,
               VkSurfaceKHR *surface);
};

#endif // WSI_H_
