/*
 * Hardinfo2 - System Information & Benchmark
 * Copyright hardinfo2 project, hwspeedy 2025
 * License: GPL2+
 *
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

#include "wsi.h"
#include "config.h"

#include <stdlib.h>

struct wsi_interface
get_wsi_interface(void)
{
#if(HARDINFO2_VK_WAYLAND)
#if(HARDINFO2_VK_X11)
    if(getenv("WAYLAND_DISPLAY")) {
        return wayland_wsi_interface();
    } else {
        return xcb_wsi_interface();
    }
  #else
    return wayland_wsi_interface();
  #endif
#else
  #if(HARDINFO2_VK_X11)
    return xcb_wsi_interface();
  #else
    #error NO Wayland or X11
  #endif
#endif
}
