/*
 * Hardinfo2 - System Information & Benchmark
 * Copyright hardinfo2 project, hwspeedy 2025
 * License: GPL2+
 *
 * Copyright © 2022 Collabora Ltd
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

#include "matrix.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <sys/time.h>

#include <vulkan/vulkan.h>

#include "wsi/wsi.h"

/*Change this version on any changes to benchmark*/
#define VERSION 1

#ifndef VK_API_VERSION_MAJOR
/* retain compatibility with old vulkan headers */
#define VK_API_VERSION_MAJOR VK_VERSION_MAJOR
#define VK_API_VERSION_MINOR VK_VERSION_MINOR
#define VK_API_VERSION_PATCH VK_VERSION_PATCH
#endif

static struct wsi_interface wsi;

static VkInstance instance;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceMemoryProperties mem_props;
static VkDevice device;
static VkQueue queue;

/* swap chain */
static int width, height, new_width, new_height;
static bool fullscreen;
static VkPresentModeKHR desidered_present_mode;
static VkSampleCountFlagBits sample_count;
static uint32_t image_count;
static VkRenderPass render_pass;
static VkCommandPool cmd_pool;
static VkFormat image_format;
static VkFormat depth_format;
static VkSurfaceKHR surface;
static VkSwapchainKHR swap_chain;
static VkImage color_msaa, depth_image;
static VkImageView color_msaa_view, depth_view;
static VkDeviceMemory color_msaa_memory, depth_memory;
static VkSemaphore back_buffer_semaphore, present_semaphore;

struct {
   VkImage image;
   VkImageView view;
   VkFramebuffer framebuffer;
   VkFence fence;
   VkCommandBuffer cmd_buffer;
} swap_chain_data[5];

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* gear data */
static VkDescriptorSet descriptor_set;
static VkDeviceMemory ubo_mem;
static VkDeviceMemory vertex_mem;
static VkBuffer ubo_buffer;
static VkBuffer vertex_buffer;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;
size_t vertex_offset, normals_offset;

struct {
   uint32_t first_vertex;
   uint32_t vertex_count;
} gears[3];

static float view_rot[3] = { 20.0, 30.0, 0.0 };
static bool animate = true;

static void
errorv(const char *format, va_list args)
{
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n");
   exit(1);
}

static void
error(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   errorv(format, args);
   va_end(args);
}

static double
current_time(void)
{
   struct timeval tv;
   (void) gettimeofday(&tv, NULL );
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void
init_vk(const char *extension)
{
   VkResult res = vkCreateInstance(
      &(VkInstanceCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
         .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Benchmark",
            .apiVersion = VK_MAKE_VERSION(1, 1, 0),
         },
         .enabledExtensionCount = extension ? 2 : 0,
         .ppEnabledExtensionNames = (const char *[2]) {
            VK_KHR_SURFACE_EXTENSION_NAME,
            extension,
         },
      },
      NULL,
      &instance);

   if (res != VK_SUCCESS)
      error("Failed to create Vulkan instance.\n");

   uint32_t count;
   res = vkEnumeratePhysicalDevices(instance, &count, NULL);
   if (res != VK_SUCCESS || count == 0)
      error("No Vulkan devices found.\n");

   VkPhysicalDevice pd[count];
   res = vkEnumeratePhysicalDevices(instance, &count, pd);
   assert(res == VK_SUCCESS);
   physical_device = pd[0];

   vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

   vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
   assert(count > 0);
   VkQueueFamilyProperties props[count];
   vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, props);
   assert(props[0].queueFlags & VK_QUEUE_GRAPHICS_BIT);

   vkCreateDevice(physical_device,
      &(VkDeviceCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
         .queueCreateInfoCount = 1,
         .pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .flags = 0,
            .pQueuePriorities = (float []) { 1.0f },
         },
         .enabledExtensionCount = 1,
         .ppEnabledExtensionNames = (const char * const []) {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
         },
      },
      NULL,
      &device);

   vkGetDeviceQueue2(device,
      &(VkDeviceQueueInfo2) {
         .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
         .flags = 0,
         .queueFamilyIndex = 0,
         .queueIndex = 0,
      },
      &queue);

   vkCreateCommandPool(device,
      &(const VkCommandPoolCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
         .queueFamilyIndex = 0,
         .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      },
      NULL,
      &cmd_pool);

   vkCreateSemaphore(device,
      &(VkSemaphoreCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      },
      NULL,
      &back_buffer_semaphore);

   vkCreateSemaphore(device,
      &(VkSemaphoreCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      },
      NULL,
      &present_semaphore);
}

static int
find_memory_type(const VkMemoryRequirements *reqs,
                 VkMemoryPropertyFlags flags)
{
    for (unsigned i = 0; (1u << i) <= reqs->memoryTypeBits &&
                         i <= mem_props.memoryTypeCount; ++i) {
        if ((reqs->memoryTypeBits & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }
    return -1;
}

static int
image_allocate(VkImage image, VkMemoryRequirements reqs, int memory_type, VkDeviceMemory *image_memory)
{
   int res = vkAllocateMemory(device,
      &(VkMemoryAllocateInfo) {
         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
         .allocationSize = reqs.size,
         .memoryTypeIndex = memory_type,
      },
      NULL,
      image_memory);
   if (res != VK_SUCCESS)
      return -1;

   res = vkBindImageMemory(device, image, *image_memory, 0);
   if (res != VK_SUCCESS)
      return -1;

   return 0;
}

static int
create_image(VkFormat format,
             VkExtent3D extent,
             VkSampleCountFlagBits samples,
             VkImageUsageFlags usage,
             VkImage *image)
{
   VkResult res = vkCreateImage(device,
      &(VkImageCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
         .flags = 0,
         .imageType = VK_IMAGE_TYPE_2D,
         .format = format,
         .extent = extent,
         .mipLevels = 1,
         .arrayLayers = 1,
         .samples = samples,
         .tiling = VK_IMAGE_TILING_OPTIMAL,
         .usage = usage,
		   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      }, 0, image);
   if (res != VK_SUCCESS)
      return -1;

   return 0;
}

static int
create_image_view(VkImage image,
                  VkFormat view_format,
                  VkImageAspectFlags aspect_mask,
                  VkImageView *image_view)
{
   int res = vkCreateImageView(device,
      &(VkImageViewCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
         .image = image,
         .viewType = VK_IMAGE_VIEW_TYPE_2D,
         .format = view_format,
         .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
         },
         .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
         },
      },
      NULL,
      image_view);

   if(res != VK_SUCCESS)
      return -1;
   return 0;
}

static void
create_render_pass()
{
   int attachment_count, color_attachment_index;
   VkAttachmentReference *resolve_attachments = (VkAttachmentReference []) {
      {
         .attachment = 0,
         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      }
   };

   if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
      attachment_count = 3;
      color_attachment_index = 2;
   } else {
      attachment_count = 2;
      resolve_attachments = NULL;
      color_attachment_index = 0;
   }

   vkCreateRenderPass(device,
      &(VkRenderPassCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
         .attachmentCount = attachment_count,
         .pAttachments = (VkAttachmentDescription[]) {
            {
               .format = image_format,
               .samples = VK_SAMPLE_COUNT_1_BIT,
               .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
               .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
               .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            {
               .format = depth_format,
               .samples = sample_count,
               .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
               .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
               .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
               .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
               .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
            {
               .format = image_format,
               .samples = sample_count,
               .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
               .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
               .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
         },
         .subpassCount = 1,
         .pSubpasses = (VkSubpassDescription []) {
            {
               .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
               .inputAttachmentCount = 0,
               .colorAttachmentCount = 1,
               .pColorAttachments = (VkAttachmentReference []) {
                  {
                     .attachment = color_attachment_index,
                     .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                  }
               },
               .pDepthStencilAttachment = (VkAttachmentReference []) {
                  {
                     .attachment = 1,
                     .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                  }
               },
               .pResolveAttachments = resolve_attachments,
               .preserveAttachmentCount = 0,
               .pPreserveAttachments = NULL,
            }
         },
         .dependencyCount = 0
      },
      NULL,
      &render_pass);
}

static void
create_swapchain()
{
   VkSurfaceCapabilitiesKHR surface_caps;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                             &surface_caps);
   assert(surface_caps.supportedCompositeAlpha &
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

   VkBool32 supported;
   vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, 0, surface,
                                        &supported);
   assert(supported);

   uint32_t count;
   vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                             &count, NULL);
   VkPresentModeKHR present_modes[count];
   vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                             &count, present_modes);
   uint32_t i;
   VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;//VK_PRESENT_MODE_FIFO_KHR;
   for (i = 0; i < count; i++) {
      if (present_modes[i] == desidered_present_mode) {
         present_mode = desidered_present_mode;
         break;
      }
   }

   uint32_t minImageCount = 2;
   if (minImageCount < surface_caps.minImageCount) {
      if (surface_caps.minImageCount > ARRAY_SIZE(swap_chain_data))
          error("surface_caps.minImageCount is too large (is: %d, max: %d)",
                surface_caps.minImageCount, ARRAY_SIZE(swap_chain_data));
      minImageCount = surface_caps.minImageCount;
   }

   if (surface_caps.maxImageCount > 0 &&
       minImageCount > surface_caps.maxImageCount) {
      minImageCount = surface_caps.maxImageCount;
   }

   vkCreateSwapchainKHR(device,
      &(VkSwapchainCreateInfoKHR) {
         .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
         .flags = 0,
         .surface = surface,
         .minImageCount = minImageCount,
         .imageFormat = image_format,
         .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
         .imageExtent = { width, height },
         .imageArrayLayers = 1,
         .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
         .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
         .queueFamilyIndexCount = 1,
         .pQueueFamilyIndices = (uint32_t[]) { 0 },
         .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
         .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
         .presentMode = present_mode,
      }, NULL, &swap_chain);

   vkGetSwapchainImagesKHR(device, swap_chain,
                           &image_count, NULL);
   assert(image_count > 0);
   VkImage swap_chain_images[image_count];
   vkGetSwapchainImagesKHR(device, swap_chain,
                           &image_count, swap_chain_images);


   int res;
   if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
       res = create_image(image_format,
         (VkExtent3D) {
            .width = width,
            .height = height,
            .depth = 1,
         },
         sample_count,
         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
         &color_msaa);
      if (res)
         error("Failed to create resolve image");

      VkMemoryRequirements msaa_reqs;
      vkGetImageMemoryRequirements(device, color_msaa, &msaa_reqs);
      int memory_type = find_memory_type(&msaa_reqs, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
      if (memory_type < 0) {
         memory_type = find_memory_type(&msaa_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
         if (memory_type < 0)
            error("find_memory_type failed");
      }
      res = image_allocate(color_msaa, msaa_reqs, memory_type, &color_msaa_memory);
      if (res)
         error("Failed to allocate memory for the resolve image");

      res = create_image_view(color_msaa, image_format, VK_IMAGE_ASPECT_COLOR_BIT,
                                        &color_msaa_view);

      if (res)
         error("Failed to create the image view for the resolve image");
   }

   res = create_image(depth_format,
      (VkExtent3D) {
         .width = width,
         .height = height,
         .depth = 1,
      },
      sample_count,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
      &depth_image);

   if (res)
      error("Failed to create depth image");

   VkMemoryRequirements depth_reqs;
   vkGetImageMemoryRequirements(device, depth_image, &depth_reqs);
   int memory_type = find_memory_type(&depth_reqs, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
   if (memory_type < 0) {
      memory_type = find_memory_type(&depth_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      if (memory_type < 0)
         error("find_memory_type failed");
   }
   res = image_allocate(depth_image, depth_reqs, memory_type, &depth_memory);
   if (res)
      error("Failed to allocate memory for the depth image");

   res = create_image_view(depth_image,
      depth_format,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      &depth_view);

   if (res)
      error("Failed to create the image view for the depth image");

   int attachment_count = sample_count != VK_SAMPLE_COUNT_1_BIT ? 3 : 2;

   for (uint32_t i = 0; i < image_count; i++) {
      swap_chain_data[i].image = swap_chain_images[i];
      vkCreateImageView(device,
         &(VkImageViewCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap_chain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_format,
            .components = {
               .r = VK_COMPONENT_SWIZZLE_R,
               .g = VK_COMPONENT_SWIZZLE_G,
               .b = VK_COMPONENT_SWIZZLE_B,
               .a = VK_COMPONENT_SWIZZLE_A,
            },
            .subresourceRange = {
               .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
               .baseMipLevel = 0,
               .levelCount = 1,
               .baseArrayLayer = 0,
               .layerCount = 1,
            },
         },
         NULL,
         &swap_chain_data[i].view);

      vkCreateFramebuffer(device,
         &(VkFramebufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = attachment_count,
            .pAttachments = (VkImageView[]) {
               swap_chain_data[i].view,
               depth_view,
               color_msaa_view,
            },
            .width = width,
            .height = height,
            .layers = 1
         },
         NULL,
         &swap_chain_data[i].framebuffer);

      vkCreateFence(device,
         &(VkFenceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
         },
         NULL,
         &swap_chain_data[i].fence);

      vkAllocateCommandBuffers(device,
         &(VkCommandBufferAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
         },
         &swap_chain_data[i].cmd_buffer);
   }
}

static void
free_swapchain_data()
{
   for (uint32_t i = 0; i < image_count; i++) {
      vkFreeCommandBuffers(device, cmd_pool, 1, &swap_chain_data[i].cmd_buffer);
      vkDestroyFence(device, swap_chain_data[i].fence, NULL);
      vkDestroyFramebuffer(device, swap_chain_data[i].framebuffer, NULL);
      vkDestroyImageView(device, swap_chain_data[i].view, NULL);
   }

   vkDestroyImageView(device, depth_view, NULL);
   vkDestroyImage(device, depth_image, NULL);
   vkFreeMemory(device, depth_memory, NULL);

   if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
      vkDestroyImageView(device, color_msaa_view, NULL);
      vkDestroyImage(device, color_msaa, NULL);
      vkFreeMemory(device, color_msaa_memory, NULL);
   }
}

static void
recreate_swapchain()
{
   free_swapchain_data();
   vkDestroySwapchainKHR(device, swap_chain, NULL);
   width = new_width, height = new_height;
   create_swapchain();
}

static VkBuffer
create_buffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
   VkBuffer buffer;

   VkResult result =
      vkCreateBuffer(device,
         &(VkBufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .flags = 0
         },
         NULL,
         &buffer);

   if (result != VK_SUCCESS)
      error("Failed to create buffer");

   return buffer;
}

static VkDeviceMemory
allocate_buffer_mem(VkBuffer buffer, VkDeviceSize mem_size)
{
   VkMemoryRequirements reqs;
   vkGetBufferMemoryRequirements(device, buffer, &reqs);

   int memory_type = find_memory_type(&reqs,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
   if (memory_type < 0)
      error("failed to find coherent memory type");

   VkDeviceMemory mem;
   vkAllocateMemory(device,
      &(VkMemoryAllocateInfo) {
         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
         .allocationSize = mem_size,
         .memoryTypeIndex = memory_type,
      },
      NULL,
      &mem);
   return mem;
}

static uint32_t vs_spirv_source[] = {
#include "gear.vert.spv.h"
};

static uint32_t fs_spirv_source[] = {
#include "gear.frag.spv.h"
};

struct ubo {
   float projection[16];
};

struct push_constants {
   float modelview[16];
   float material_color[3];
};

struct gear {
   int nvertices;
};

#define GEAR_VERTEX_STRIDE 6

static int
create_gear(float verts[],
            float inner_radius, float outer_radius, float width,
            int teeth, float tooth_depth)
{
   // normal state
   float current_normal[3];
#define SET_NORMAL(x, y, z) do { \
      current_normal[0] = x; \
      current_normal[1] = y; \
      current_normal[2] = z; \
   } while (0)

   // vertex buffer state
   unsigned num_verts = 0;

#define EMIT_VERTEX(x, y, z) do { \
      verts[num_verts * GEAR_VERTEX_STRIDE + 0] = x; \
      verts[num_verts * GEAR_VERTEX_STRIDE + 1] = y; \
      verts[num_verts * GEAR_VERTEX_STRIDE + 2] = z; \
      memcpy(verts + num_verts * GEAR_VERTEX_STRIDE + 3, \
             current_normal, sizeof(current_normal)); \
      num_verts++; \
   } while (0)

   // strip restart-logic
   int cur_strip_start = 0;
#define START_STRIP() do { \
   cur_strip_start = num_verts; \
   if (cur_strip_start) \
      num_verts += 2; \
} while(0);

#define END_STRIP() do { \
   if (cur_strip_start) { \
      memcpy(verts + cur_strip_start * GEAR_VERTEX_STRIDE, \
             verts + (cur_strip_start - 1) * GEAR_VERTEX_STRIDE, \
             sizeof(float) * GEAR_VERTEX_STRIDE); \
      memcpy(verts + (cur_strip_start + 1) * GEAR_VERTEX_STRIDE, \
             verts + (cur_strip_start + 2) * GEAR_VERTEX_STRIDE, \
             sizeof(float) * GEAR_VERTEX_STRIDE); \
   } \
} while (0)

   float r0 = inner_radius;
   float r1 = outer_radius - tooth_depth / 2.0;
   float r2 = outer_radius + tooth_depth / 2.0;

   float da = 2.0 * M_PI / teeth / 4.0;

   SET_NORMAL(0.0, 0.0, 1.0);

   /* draw front face */
   START_STRIP();
   for (int i = 0; i <= teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      if (i < teeth) {
         EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), width * 0.5);
         EMIT_VERTEX(r1 * cos(angle + 3 * da),
                     r1 * sin(angle + 3 * da), width * 0.5);
      }
   }
   END_STRIP();

   /* draw front sides of teeth */
   for (int i = 0; i < teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      START_STRIP();
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      END_STRIP();
   }

   SET_NORMAL(0.0, 0.0, -1.0);

   /* draw back face */
   START_STRIP();
   for (int i = 0; i <= teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      if (i < teeth) {
         EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
               -width * 0.5);
         EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      }
   }
   END_STRIP();

   /* draw back sides of teeth */
   for (int i = 0; i < teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      START_STRIP();
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      END_STRIP();
   }

   /* draw outward faces of teeth */
   for (int i = 0; i < teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      float u = r2 * cos(angle + da) - r1 * cos(angle);
      float v = r2 * sin(angle + da) - r1 * sin(angle);
      float len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      SET_NORMAL(v, -u, 0.0);
      START_STRIP();
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      EMIT_VERTEX(r1 * cos(angle), r1 * sin(angle), -width * 0.5);

      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      END_STRIP();

      SET_NORMAL(cos(angle), sin(angle), 0.0);
      START_STRIP();
      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da),
		 width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + da), r2 * sin(angle + da),
		 -width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
      width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
      -width * 0.5);
      END_STRIP();

      u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      SET_NORMAL(v, -u, 0.0);
      START_STRIP();
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      EMIT_VERTEX(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      END_STRIP();

      SET_NORMAL(cos(angle), sin(angle), 0.0);
      START_STRIP();
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 4 * da), r1 * sin(angle + 4 * da),
      width * 0.5);
      EMIT_VERTEX(r1 * cos(angle + 4 * da), r1 * sin(angle + 4 * da),
      -width * 0.5);
      END_STRIP();
   }

   /* draw inside radius cylinder */
   START_STRIP();
   for (int i = 0; i <= teeth; i++) {
      float angle = i * 2.0 * M_PI / teeth;
      SET_NORMAL(-cos(angle), -sin(angle), 0.0);
      EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      EMIT_VERTEX(r0 * cos(angle), r0 * sin(angle), width * 0.5);
   }
   END_STRIP();

   return num_verts;
}


static void
init_gears()
{
   VkResult r;

   VkDescriptorSetLayout set_layout;
   vkCreateDescriptorSetLayout(device,
      &(VkDescriptorSetLayoutCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
         .bindingCount = 1,
         .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
               .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
               .descriptorCount = 1,
               .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
               .pImmutableSamplers = NULL
            }
         }
      },
      NULL,
      &set_layout);

   vkCreatePipelineLayout(device,
      &(VkPipelineLayoutCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
         .setLayoutCount = 1,
         .pSetLayouts = &set_layout,
         .pPushConstantRanges = (VkPushConstantRange[]) {
            {
               .offset = 0,
               .size = sizeof(struct push_constants),
               .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
         },
         .pushConstantRangeCount = 1,
      },
      NULL,
      &pipeline_layout);

   VkShaderModule vs_module;
   vkCreateShaderModule(device,
      &(VkShaderModuleCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
         .codeSize = sizeof(vs_spirv_source),
         .pCode = vs_spirv_source,
      },
      NULL,
      &vs_module);

   VkShaderModule fs_module;
   vkCreateShaderModule(device,
      &(VkShaderModuleCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
         .codeSize = sizeof(fs_spirv_source),
         .pCode = fs_spirv_source,
      },
      NULL,
      &fs_module);

   vkCreateGraphicsPipelines(device,
      (VkPipelineCache) { VK_NULL_HANDLE },
      1,
      &(VkGraphicsPipelineCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
         .stageCount = 2,
         .pStages = (VkPipelineShaderStageCreateInfo[]) {
             {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vs_module,
                .pName = "main",
             },
             {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fs_module,
                .pName = "main",
             },
         },
         .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 2,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
               {
                  .binding = 0,
                  .stride = 6 * sizeof(float),
                  .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
               },
               {
                  .binding = 1,
                  .stride = 6 * sizeof(float),
                  .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
               },
            },
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
               {
                  .location = 0,
                  .binding = 0,
                  .format = VK_FORMAT_R32G32B32_SFLOAT,
                  .offset = 0
               },
               {
                  .location = 1,
                  .binding = 1,
                  .format = VK_FORMAT_R32G32B32_SFLOAT,
                  .offset = 0
               },
            }
         },
         .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = false,
         },

         .pViewportState = &(VkPipelineViewportStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
         },

         .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .rasterizerDiscardEnable = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f,
         },

         .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = sample_count,
         },
         .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
         },

         .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = (VkPipelineColorBlendAttachmentState []) {
               { .colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                   VK_COLOR_COMPONENT_R_BIT |
                                   VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT },
            }
         },

         .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = (VkDynamicState[]) {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
            },
         },

         .flags = 0,
         .layout = pipeline_layout,
         .renderPass = render_pass,
         .subpass = 0,
         .basePipelineHandle = (VkPipeline) { 0 },
         .basePipelineIndex = 0
      },
      NULL,
      &pipeline);

#define MAX_VERTS 10000
   float verts[MAX_VERTS * GEAR_VERTEX_STRIDE];

   gears[0].first_vertex = 0;
   gears[0].vertex_count = create_gear(verts + gears[0].first_vertex * GEAR_VERTEX_STRIDE,
                                       1.0, 4.0, 1.0, 20, 0.7);
   gears[1].first_vertex = gears[0].first_vertex + gears[0].vertex_count;
   gears[1].vertex_count = create_gear(verts + gears[1].first_vertex * GEAR_VERTEX_STRIDE,
                                       0.5, 2.0, 2.0, 10, 0.7);
   gears[2].first_vertex = gears[1].first_vertex + gears[1].vertex_count;
   gears[2].vertex_count = create_gear(verts + gears[2].first_vertex * GEAR_VERTEX_STRIDE,
                                       1.3, 2.0, 0.5, 10, 0.7);

   unsigned num_verts = gears[2].first_vertex + gears[2].vertex_count;
   unsigned mem_size = sizeof(float) * GEAR_VERTEX_STRIDE * num_verts;
   vertex_offset = 0;
   normals_offset = sizeof(float) * 3;
   ubo_buffer = create_buffer(sizeof(struct ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT);
   vertex_buffer = create_buffer(mem_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

   ubo_mem = allocate_buffer_mem(ubo_buffer, sizeof(struct ubo));
   vertex_mem = allocate_buffer_mem(vertex_buffer, mem_size);

   void *map;
   r = vkMapMemory(device, vertex_mem, 0, mem_size, 0, &map);
   if (r != VK_SUCCESS)
      error("vkMapMemory failed");
   memcpy(map, verts, mem_size);
   vkUnmapMemory(device, vertex_mem);

   vkBindBufferMemory(device, ubo_buffer, ubo_mem, 0);
   vkBindBufferMemory(device, vertex_buffer, vertex_mem, 0);

   VkDescriptorPool desc_pool;
   const VkDescriptorPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = (VkDescriptorPoolSize[]) {
         {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
         },
      }
   };

   vkCreateDescriptorPool(device, &create_info, NULL, &desc_pool);

   vkAllocateDescriptorSets(device,
      &(VkDescriptorSetAllocateInfo) {
         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
         .descriptorPool = desc_pool,
         .descriptorSetCount = 1,
         .pSetLayouts = &set_layout,
      }, &descriptor_set);

   vkUpdateDescriptorSets(device, 1,
      (VkWriteDescriptorSet []) {
         {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
               .buffer = ubo_buffer,
               .offset = 0,
               .range = sizeof(struct ubo),
            }
         }
      },
      0, NULL);
}

static void
draw_gear(VkCommandBuffer cmdbuf, const float view[16],
          float position[2], float angle,
          const float material_color[3],
          unsigned first_vertex, unsigned vertex_count)
{
   /* Translate and rotate the gear */
   float modelview[16];
   mat4_identity(modelview);
   mat4_multiply(modelview, view);
   mat4_translate(modelview, position[0], position[1], 0);
   mat4_rotate(modelview, 2 * M_PI * angle / 360.0, 0, 0, 1);

   float h = (float)height / width;
   float projection[16];
   mat4_identity(projection);
   mat4_frustum_vk(projection, -1.0, 1.0, -h, +h, 5.0f, 60.0f);

   struct push_constants push_constants;
   mat4_identity(push_constants.modelview);
   mat4_multiply(push_constants.modelview, modelview);
   memcpy(push_constants.material_color, material_color, sizeof(push_constants.material_color));

   vkCmdPushConstants(cmdbuf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                      0, sizeof(push_constants), &push_constants);
   vkCmdDraw(cmdbuf, vertex_count, 1, first_vertex, 0);
}

float angle = 0.0;

#define G2L(x) ((x) < 0.04045 ? (x) / 12.92 : powf(((x) + 0.055) / 1.055, 2.4))

static void
draw_gears(VkCommandBuffer cmdbuf, const float view[16])
{
   vkCmdBindVertexBuffers(cmdbuf, 0, 2,
      (VkBuffer[]) {
         vertex_buffer,
         vertex_buffer,
      },
      (VkDeviceSize[]) {
         vertex_offset,
         normals_offset
      });

   vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

   vkCmdBindDescriptorSets(cmdbuf,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline_layout,
      0, 1,
      &descriptor_set, 0, NULL);

   vkCmdSetViewport(cmdbuf, 0, 1,
      &(VkViewport) {
         .x = 0,
         .y = 0,
         .width = width,
         .height = height,
         .minDepth = 0,
         .maxDepth = 1,
      });

   vkCmdSetScissor(cmdbuf, 0, 1,
      &(VkRect2D) {
         .offset = { 0, 0 },
         .extent = { width, height },
      });

   float positions[3][2] = {
      {-3.0, -2.0},
      { 3.1, -2.0},
      {-3.1,  4.2},
   };

   float angles[3] = {
      angle,
      -2.0 * angle - 9.0,
      -2.0 * angle - 25.0,
   };

   const float material_colors[3][3] = {
      { G2L(0.8), G2L(0.1), G2L(0.0) },
      { G2L(0.0), G2L(0.8), G2L(0.2) },
      { G2L(0.2), G2L(0.2), G2L(1.0) },
   };

   for (int i = 0; i < 3; ++i) {
      draw_gear(cmdbuf, view, positions[i], angles[i], material_colors[i],
                gears[i].first_vertex, gears[i].vertex_count);
   }
}

static const char *
get_devtype_str(VkPhysicalDeviceType devtype)
{
   static char buf[256];
   switch (devtype) {
   case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      return "other";
   case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return "integrated GPU";
   case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return "discrete GPU";
   case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return "virtual GPU";
   case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return "CPU";
   default:
      snprintf(buf, sizeof(buf), "Unknown (%08x)", devtype);
      return buf;
   }
}

static void
usage(void)
{
   printf("Usage:\n");
   printf("  -samples N              run in multisample mode with N samples\n");
   printf("  -present-mailbox        run with present mode mailbox\n");
   printf("  -fullscreen             run in fullscreen mode\n");
   printf("  -info                   display Vulkan device info\n");
   printf("  -size WxH               window size\n");
}

static void
print_info()
{
   VkPhysicalDeviceProperties properties;
   vkGetPhysicalDeviceProperties(physical_device, &properties);
   printf("apiVersion    = %d.%d.%d\n",
            VK_API_VERSION_MAJOR(properties.apiVersion),
            VK_API_VERSION_MINOR(properties.apiVersion),
            VK_API_VERSION_PATCH(properties.apiVersion));
   printf("driverVersion = %04x\n", properties.driverVersion);
   printf("vendorID      = %04x\n", properties.vendorID);
   printf("deviceID      = %04x\n", properties.deviceID);
   printf("deviceType    = %s\n", get_devtype_str(properties.deviceType));
   printf("deviceName    = %s\n", properties.deviceName);

   uint32_t num_extensions = 0;
   VkExtensionProperties *extensions;
   vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, NULL);
   if (num_extensions > 0) {
      extensions = calloc(num_extensions, sizeof(VkExtensionProperties));
      if (!extensions)
         error("Failed to allocate memory");

      vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, extensions);
      printf("deviceExtensions:\n");
      for (uint32_t i = 0; i < num_extensions; ++i)
         printf("\t%s\n", extensions[i].extensionName);
   }
}

static VkFormat
find_depth_format()
{
   // either VK_FORMAT_D32_SFLOAT or VK_FORMAT_X8_D24_UNORM_PACK32 needs to be supported; find out which one
   VkFormatProperties props;
   vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_D32_SFLOAT, &props);
   return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ?
      VK_FORMAT_D32_SFLOAT : VK_FORMAT_X8_D24_UNORM_PACK32;
}

static VkSampleCountFlagBits
sample_count_flag(int sample_count)
{
   switch (sample_count) {
   case 1:
      return VK_SAMPLE_COUNT_1_BIT;
   case 2:
      return VK_SAMPLE_COUNT_2_BIT;
   case 4:
      return VK_SAMPLE_COUNT_4_BIT;
   case 8:
      return VK_SAMPLE_COUNT_8_BIT;
   case 16:
      return VK_SAMPLE_COUNT_16_BIT;
   case 32:
      return VK_SAMPLE_COUNT_32_BIT;
   case 64:
      return VK_SAMPLE_COUNT_64_BIT;
   default:
      error("Invalid sample count");
   }
   return VK_SAMPLE_COUNT_1_BIT;
}

static bool
check_sample_count_support(VkSampleCountFlagBits sample_count)
{
   VkPhysicalDeviceProperties properties;
   vkGetPhysicalDeviceProperties(physical_device, &properties);

   return (sample_count & properties.limits.framebufferColorSampleCounts) &&
      (sample_count & properties.limits.framebufferDepthSampleCounts);
}

static void
wsi_resize(int p_new_width, int p_new_height)
{
   new_width = p_new_width;
   new_height = p_new_height;
}

static void
wsi_key_press(bool down, enum wsi_key key) {
   if (!down)
      return;
   switch (key) {
      case WSI_KEY_ESC:
         exit(0);
      case WSI_KEY_UP:
         view_rot[0] += 5.0;
         break;
      case WSI_KEY_DOWN:
         view_rot[0] -= 5.0;
         break;
      case WSI_KEY_LEFT:
         view_rot[1] += 5.0;
         break;
      case WSI_KEY_RIGHT:
         view_rot[1] -= 5.0;
         break;
      case WSI_KEY_A:
         animate = !animate;
         break;
      default:
         break;
   }
}

static void
wsi_exit()
{
   exit(0);
}

static struct wsi_callbacks wsi_callbacks = {
   .resize = wsi_resize,
   .key_press = wsi_key_press,
   .exit = wsi_exit,
};

int
main(int argc, char *argv[])
{
   bool printInfo = false;
   sample_count = VK_SAMPLE_COUNT_1_BIT;
   desidered_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;//VK_PRESENT_MODE_FIFO_KHR;
   width = 1024;
   height = 800;
   fullscreen = false;

   for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-info") == 0) {
         printInfo = true;
      }
      else if (strcmp(argv[i], "-samples") == 0 && i + 1 < argc) {
         i++;
         sample_count = sample_count_flag(atoi(argv[i]));
      }
      else if (strcmp(argv[i], "-present-mailbox") == 0) {
         desidered_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      }
      else if (strcmp(argv[i], "-size") == 0 && i + 1 < argc) {
         i++;
         char * token;
         token = strtok(argv[i], "x");
         if (!token)
            continue;
         width = atoi(token);
         if((token = strtok(NULL, "x"))) {
            height = atoi(token);
         }
      }
      else if (strcmp(argv[i], "-fullscreen") == 0) {
         fullscreen = true;
      }
      else {
         usage();
         return -1;
      }
   }

   new_width = width, new_height = height;
   wsi = get_wsi_interface();
   wsi.set_wsi_callbacks(wsi_callbacks);

   wsi.init_display();
   wsi.init_window("Vulkan Benchmark", width, height, fullscreen);

   init_vk(wsi.required_extension_name);

   if (!check_sample_count_support(sample_count))
      error("Sample count not supported");

   image_format = VK_FORMAT_B8G8R8A8_SRGB;

   if (printInfo)
      print_info();

   depth_format = find_depth_format();

   if (!wsi.create_surface(physical_device, instance, &surface))
      error("Failed to create surface!");

   create_render_pass();
   create_swapchain();
   init_gears();

   while (1) {
      static int frames = 0;
      static double tRot0 = -1.0, tRate0 = -1.0;
      double dt, t = current_time();

      if (tRot0 < 0.0)
         tRot0 = t;
      dt = t - tRot0;
      tRot0 = t;

      if (animate) {
         /* advance rotation for next frame */
         angle += 70.0 * dt;  /* 70 degrees per second */
         if (angle > 3600.0)
            angle -= 3600.0;
      }

      if (wsi.update_window()) {
         printf("update window failed\n");
         break;
      }

      uint32_t index;
      VkResult result =
         vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
                               back_buffer_semaphore, VK_NULL_HANDLE,
                               &index);
      if (result == VK_SUBOPTIMAL_KHR ||
          width != new_width || height != new_height) {
         for (uint32_t i = 0; i < image_count; i++)
            vkWaitForFences(device, 1, &swap_chain_data[i].fence, VK_TRUE, UINT64_MAX);
         recreate_swapchain();
         continue;
      }
      assert(result == VK_SUCCESS);

      assert(index < ARRAY_SIZE(swap_chain_data));

      vkWaitForFences(device, 1, &swap_chain_data[index].fence, VK_TRUE, UINT64_MAX);
      vkResetFences(device, 1, &swap_chain_data[index].fence);

      vkBeginCommandBuffer(swap_chain_data[index].cmd_buffer,
         &(VkCommandBufferBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0
         });

      /* projection matrix */
      float h = (float)height / width;
      struct ubo ubo;
      mat4_identity(ubo.projection);
      mat4_frustum_vk(ubo.projection, -1.0, 1.0, -h, +h, 5.0f, 60.0f);
      vkCmdUpdateBuffer(swap_chain_data[index].cmd_buffer, ubo_buffer, 0, sizeof(ubo), &ubo);

      /* Translate and rotate the view */
      float view[16];
      mat4_identity(view);
      mat4_translate(view, 0, 0, -40);
      mat4_rotate(view, 2 * M_PI * view_rot[0] / 360.0, 1, 0, 0);
      mat4_rotate(view, 2 * M_PI * view_rot[1] / 360.0, 0, 1, 0);
      mat4_rotate(view, 2 * M_PI * view_rot[2] / 360.0, 0, 0, 1);

      vkCmdBeginRenderPass(swap_chain_data[index].cmd_buffer,
         &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = swap_chain_data[index].framebuffer,
            .renderArea = { { 0, 0 }, { width, height } },
            .clearValueCount = 3,
            .pClearValues = (VkClearValue []) {
               { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } },
               { .depthStencil.depth = 1.0f },
               { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } },
            }
         },
         VK_SUBPASS_CONTENTS_INLINE);

      draw_gears(swap_chain_data[index].cmd_buffer, view);

      vkCmdEndRenderPass(swap_chain_data[index].cmd_buffer);
      vkEndCommandBuffer(swap_chain_data[index].cmd_buffer);

      vkQueueSubmit(queue, 1,
         &(VkSubmitInfo) {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &back_buffer_semaphore,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &present_semaphore,
            .pWaitDstStageMask = (VkPipelineStageFlags []) {
               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            },
            .commandBufferCount = 1,
            .pCommandBuffers = &swap_chain_data[index].cmd_buffer,
         }, swap_chain_data[index].fence);

      vkQueuePresentKHR(queue,
         &(VkPresentInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pWaitSemaphores = &present_semaphore,
            .waitSemaphoreCount = 1,
            .swapchainCount = 1,
            .pSwapchains = (VkSwapchainKHR[]) { swap_chain, },
            .pImageIndices = (uint32_t[]) { index, },
            .pResults = &result,
         });

      frames++;

      if (tRate0 < 0.0)
         tRate0 = t;
      if (t - tRate0 >= 3.0) {
         float seconds = t - tRate0;
         float fps = frames / seconds;
         printf("Ver=%d, Result:%6.3f\n", VERSION, fps);
         fflush(stdout);
         tRate0 = t;
         frames = 0;
	 break;
      }
   }

   wsi.fini_window();
   wsi.fini_display();
   return 0;
}
