/*
 * vk1_gears
 * copyright: hardinfo2
 * License: GPL2+
 *
 * === History ===
 * hwspeedy GPL2+
 *
 * Michael Clark (Public Domain)
 *   - Conversion from fixed function OpenGL to Vulkan
 *
 * Marcus Geelnard: (Public Domain)
 *   - Conversion to GLFW
 *   - Time based rendering (frame rate independent)
 *   - Slightly modified camera that should work better for stereo viewing
 *
 * Camilla Löwy: (Public Domain)
 *   - Removed FPS counter (this is not a benchmark)
 *   - Added a few comments
 *   - Enabled vsync
 *
 * Brian Paul (Public Domain)
 *   - Orignal version
 */

#undef NDEBUG

#define VERSION 1
/* Increase version on changes */

#if defined(_MSC_VER)
 // Make MS math.h define M_PI
 #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "linmath.h"

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <config_vkgears.h>

/* return current time (in seconds) */
static double
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}
static const char* frag_shader_filename = FRAG_SHADER;
static const char* vert_shader_filename = VERT_SHADER;

typedef unsigned uint;

typedef union vec2 { float vec[2]; struct { float x, y;       } m; } vec2f;
typedef union vec3 { float vec[3]; struct { float x, y, z;    } m; } vec3f;
typedef union vec4 { float vec[4]; struct { float x, y, z, w; } m; } vec4f;

typedef struct vertex {
    vec3f pos;
    vec3f norm;
    vec2f uv;
    vec4f col;
} gears_vertex;

typedef struct gears_swapchain_buffer
{
    VkImage image;
    VkImageView view;
    VkFramebuffer framebuffer;
} gears_swapchain_buffer;

typedef struct gears_image_buffer
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkFormat format;
} gears_image_buffer;

typedef struct gears_buffer
{
    size_t size;
    size_t total;
    size_t count;
    void *data;
    VkDeviceMemory memory;
    VkBuffer buffer;
} gears_buffer;

typedef struct gears_view_rotation
{
    float rotx;
    float roty;
    float rotz;
    float angle;
} gears_view_rotation;

typedef struct gears_uniform_data
{
    mat4x4 projection;
    mat4x4 model;
    mat4x4 view;
    vec3 lightpos;
} gears_uniform_data;

typedef struct gears_uniform_memory
{
    VkDeviceMemory memory;
    VkBuffer buffer;
} gears_uniform_memory;

typedef struct gears_uniform_buffer
{
    gears_uniform_data data;
    gears_uniform_memory *mem;
} gears_uniform_buffer;

typedef struct gears_app_gear
{
    gears_buffer vb;
    gears_buffer ib;
    gears_uniform_buffer ub;
} gears_app_gear;

typedef struct gears_app
{
    const char* name;
    uint width;
    uint height;
    GLFWwindow* window;
    VkSurfaceKHR surface;
    VkInstance instance;
    VkPhysicalDevice physdev;
    VkPhysicalDeviceProperties physdev_props;
    uint32_t queue_family_index;
    VkDevice device;
    VkQueue queue;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSwapchainKHR swapchain;
    VkSampleCountFlags sample_count;
    uint32_t swapchain_image_count;
    gears_swapchain_buffer *swapchain_buffers;
    gears_image_buffer resolve_buffer;
    gears_image_buffer depth_buffer;
    VkCommandPool cmd_pool;
    VkCommandBuffer *cmd_buffers;
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_complete_semaphores;
    VkFence *swapchain_fences;
    VkRenderPass render_pass;
    VkShaderModule frag_shader;
    VkShaderModule vert_shader;
    VkDescriptorPool desc_pool;
    VkDescriptorSetLayout desc_set_layout;
    uint32_t desc_set_count;
    VkDescriptorSet *desc_sets;
    VkPipelineLayout pipeline_layout;
    VkPipelineCache pipeline_cache;
    VkPipeline pipeline;
    float view_dist;
    gears_view_rotation rotation;
    gears_app_gear gear[3];
    int max_swapchain_images;
    uint max_samples;
    int verbose_enable;
    int animation_enable;
    int multisampling_enable;
    int validation_enable;
    int api_dump_enable;
} gears_app;

typedef enum {
    gears_topology_triangles,
    gears_topology_triangle_strip,
    gears_topology_quads,
    gears_topology_quad_strip,
} gears_primitive_type;

enum {
    GEARS_VERTEX_INITIAL_COUNT = 16,
    GEARS_INDEX_INITIAL_COUNT = 64
};

static void gears_buffer_init(gears_buffer *buffer, size_t size, size_t count);
static void gears_buffer_freeze(gears_app *app, gears_buffer *buffer, int usage);
static void gears_buffer_destroy(gears_app *app, gears_buffer *buffer);
static void* gears_buffer_data(gears_buffer *vb);
static size_t gears_buffer_size(gears_buffer *vb);
static uint32_t gears_buffer_count(gears_buffer *vb);
static uint32_t gears_buffer_add_vertex(gears_buffer *vb, gears_vertex vertex);
static void gears_buffer_add_indices(gears_buffer *vb,
    const uint32_t *indices, uint32_t count, uint32_t addend);
static void gears_buffer_add_indices_primitves(gears_buffer *vb,
    gears_primitive_type type, uint count, uint addend);

void panic(const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);
    exit(9);
}

#define str(s) #s
#define VK_CALL(func) { VkResult err = func; if (err != VK_SUCCESS) \
 { panic("%s:%d " str(func) " failed: err=%d\n", __FILE__, __LINE__, err); } }
// else { printf("%s:%d " str(func) "\n", __FILE__, __LINE__); } }

static uint32_t memory_type_index(VkMemoryRequirements *memory_reqs,
    VkFlags mask, VkPhysicalDeviceMemoryProperties *memory_props)
{
    uint32_t type_bits = memory_reqs->memoryTypeBits;
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((type_bits & 1) &&
            (memory_props->memoryTypes[i].propertyFlags & mask) == mask)
                return i;
        type_bits >>= 1;
    }
    return 0;
}

static void gears_buffer_alloc(gears_app *app, VkBuffer *buffer,
    VkDeviceMemory *memory, size_t buffer_size, VkBufferUsageFlagBits usage)
{
    uint32_t mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = buffer_size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL
    };
    VK_CALL(vkCreateBuffer(app->device, &buffer_create_info, NULL, buffer));

    VkPhysicalDeviceMemoryProperties mem_props;
    VkMemoryRequirements mem_reqs;

    vkGetPhysicalDeviceMemoryProperties(app->physdev, &mem_props);
    vkGetBufferMemoryRequirements(app->device, *buffer, &mem_reqs);

    const VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = memory_type_index(&mem_reqs, mem_flags, &mem_props)
    };
    VK_CALL(vkAllocateMemory(app->device, &mem_alloc_info, NULL, memory));
    VK_CALL(vkBindBufferMemory(app->device, *buffer, *memory, 0));
}

static void gears_buffer_init(gears_buffer *vb, size_t size, size_t count)
{
    vb->size = size;
    vb->total = count;
    vb->count = 0;
    vb->data = malloc(vb->size * vb->total);
    vb->memory = VK_NULL_HANDLE;
    vb->buffer = VK_NULL_HANDLE;
}

static void gears_buffer_freeze(gears_app *app, gears_buffer *vb, int usage)
{
    void* data;
    size_t buffer_size = gears_buffer_size(vb);
    gears_buffer_alloc(app, &vb->buffer, &vb->memory, buffer_size, usage);
    VK_CALL(vkMapMemory(app->device, vb->memory, 0, buffer_size, 0, &data));
    memcpy(data, vb->data, buffer_size);
    vkUnmapMemory(app->device, vb->memory);
}

static void gears_buffer_destroy(gears_app *app, gears_buffer *vb)
{
    free(vb->data);
    vb->data = NULL;
    vkDestroyBuffer(app->device, vb->buffer, NULL);
    vb->buffer = VK_NULL_HANDLE;
    vkFreeMemory(app->device, vb->memory, NULL);
    vb->memory = VK_NULL_HANDLE;
}

static uint32_t gears_buffer_count(gears_buffer *vb) { return vb->count; }
static void* gears_buffer_data(gears_buffer *vb) { return vb->data; }
static size_t gears_buffer_size(gears_buffer *vb) { return vb->count*vb->size; }

static uint32_t gears_buffer_add_vertex(gears_buffer *vb,
    gears_vertex vertex)
{
    if (vb->count >= vb->total) {
        vb->total <<= 1;
        vb->data = realloc(vb->data,
            vb->size * vb->total);
    }
    uint32_t idx = vb->count++;
    ((gears_vertex*)vb->data)[idx] = vertex;
    return idx;
}

static void gears_buffer_add_indices(gears_buffer *vb,
    const uint32_t *indices, uint32_t count, uint32_t addend)
{
    if (vb->count + count >= vb->total) {
        do { vb->total <<= 1; }
        while (vb->count + count > vb->total);
        vb->data = realloc(vb->data,
            vb->size * vb->total);
    }
    for (uint32_t i = 0; i < count; i++) {
        ((uint32_t*)vb->data)[vb->count++] = indices[i] + addend;
    }
}

static void gears_buffer_add_indices_primitves(gears_buffer *vb,
    gears_primitive_type type, uint count, uint addend)
{
    static const uint tri[] = {0,1,2};
    static const uint tri_strip[] = {0,1,2,2,1,3};
    static const uint quads[] = {0,1,2,0,2,3};

    switch (type) {
    case gears_topology_triangles:
        for (size_t i = 0; i < count; i++) {
            gears_buffer_add_indices(vb, tri, 3, addend);
            addend += 3;
        }
        break;
    case gears_topology_triangle_strip:
        assert((count&1) == 0);
        for (size_t i = 0; i < count; i += 2) {
            gears_buffer_add_indices(vb, tri_strip, 6, addend);
            addend += 2;
        }
        break;
    case gears_topology_quads:
        for (size_t i = 0; i < count; i++) {
            gears_buffer_add_indices(vb, quads, 6, addend);
            addend += 4;
        }
        break;
    case gears_topology_quad_strip:
        for (size_t i = 0; i < count; i++) {
            gears_buffer_add_indices(vb, tri_strip, 6, addend);
            addend += 2;
        }
        break;
    }
}

static void gears_glfw_error(int error_code, const char* error_desc)
{
    fprintf(stderr, "glfw_error: code=%d desc=%s\n", error_code, error_desc);
}

static void gears_glfw_key( GLFWwindow* window, int k, int s, int action, int mods )
{
    gears_app *app = (gears_app*)glfwGetWindowUserPointer(window);

    if( action != GLFW_PRESS ) return;

    float shiftz = (mods & GLFW_MOD_SHIFT ? -1.0 : 1.0);

    switch (k) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_X: app->animation_enable = !app->animation_enable; break;
    case GLFW_KEY_Z: app->rotation.rotz += 5.0 * shiftz; break;
    case GLFW_KEY_C: app->view_dist += 5.0 * shiftz; break;
    case GLFW_KEY_W: app->rotation.rotx += 5.0; break;
    case GLFW_KEY_S: app->rotation.rotx -= 5.0; break;
    case GLFW_KEY_A: app->rotation.roty += 5.0; break;
    case GLFW_KEY_D: app->rotation.roty -= 5.0; break;
    default: return;
    }
}

static void gears_init_app(gears_app *app, const char *app_name)
{
    memset(app, 0, sizeof(gears_app));
    app->name = "Vulkan Benchmark";
    app->width = 1024;
    app->height = 800;

    app->view_dist = -40.0;
    app->rotation.rotx = 20.f;
    app->rotation.roty = 30.f;
    app->rotation.rotz = 0.f;
    app->rotation.angle = 0.f;

    app->verbose_enable = VK_FALSE;
    app->api_dump_enable = VK_FALSE;
    app->validation_enable = VK_FALSE;
    app->multisampling_enable = VK_TRUE;
    app->animation_enable = VK_TRUE;
    app->max_samples = VK_SAMPLE_COUNT_64_BIT;
    app->max_swapchain_images = 3;
}

typedef struct commandline_option
{
    const char* name;
    int *ptr_boolean;
    int *ptr_integer;
} commandline_option;

static void gears_parse_options(gears_app *app, const int argc, const char **argv)
{
    int v = 1;
    int print_help = 0;

    /* assert(strlen(some_spaces)) >=
       max(strlen(options[n].name)) for n in length(options) */
    const char* some_spaces = "                    ";

    const commandline_option options[] = {
        { .name = "max-swapchain-images", .ptr_integer = &app->max_swapchain_images },
        { .name = "max-samples", .ptr_integer = &app->max_samples },
        { .name = "api-dump", .ptr_boolean = &app->api_dump_enable },
        { .name = "validation", .ptr_boolean = &app->validation_enable },
        { .name = "multisampling", .ptr_boolean = &app->multisampling_enable },
        { .name = "animation", .ptr_boolean = &app->animation_enable },
        { .name = "verbose", .ptr_boolean = &app->verbose_enable },
        { .name = "width", .ptr_integer = &app->width },
        { .name = "height", .ptr_integer = &app->height },
        { .name = "help", .ptr_integer = &print_help },
    };

    while(v < argc) {
        int opt_idx = -1, bool_val = VK_TRUE;
        for (uint32_t i = 0; i < sizeof(options)/sizeof(options[0]); i++) {
            size_t optlen = strlen(options[i].name);
            if (strlen(argv[v]) == optlen + 1 &&
                memcmp(argv[v], "-", 1) == 0 &&
                memcmp(argv[v]+1, options[i].name, optlen) == 0) {
                opt_idx = i;
                break;
            } else if (options[i].ptr_boolean != NULL &&
                strlen(argv[v]) == optlen + 4 &&
                memcmp(argv[v], "-no-", 4) == 0 &&
                memcmp(argv[v]+4, options[i].name, optlen) == 0) {
                opt_idx = i;
                bool_val = VK_FALSE;
                break;
            }
        }
        if (opt_idx == -1) {
            fprintf(stderr, "*** error: %s: option unknown\n\n", argv[v]);
            print_help = 1;
            break;
        }
        if (options[opt_idx].ptr_integer) {
            if (v + 1 >= argc) {
                fprintf(stderr, "*** error: -%s option requires integer\n\n",
                    argv[v]);
                print_help = 1;
                break;
            } else {
                *options[opt_idx].ptr_integer = atoi(argv[++v]);
            }
        } else if (options[opt_idx].ptr_boolean) {
            *options[opt_idx].ptr_boolean = bool_val;
        }
        v++;
    }

    if (print_help) {
        fprintf(stderr, "usage: %s [options]\n\n", argv[0]);
        fprintf(stderr, "Options\n\n");
        for (uint32_t i = 0; i < sizeof(options)/sizeof(options[0]); i++) {
            const char * spaces = some_spaces + strlen(options[i].name);
            if (options[i].ptr_boolean) {
                fprintf(stderr, "-[no-]%s %s  enable or disable %s\n",
                    options[i].name, spaces, options[i].name);
            } else if (options[i].ptr_integer) {
                fprintf(stderr, "-%s <int> %s specify %s\n",
                    options[i].name, spaces, options[i].name);
            }
        }
        exit(0);
    }
}

static void gears_init_glfw(gears_app *app)
{
    if (!glfwInit()) {
        panic("glfwInit failed\n");
    }
    if (!glfwVulkanSupported()) {
        panic("glfwVulkanSupported failed\n");
    }
}

static void gears_destroy_window(gears_app *app)
{
    glfwDestroyWindow(app->window);
    app->window = 0;
}

static void gears_create_window(gears_app *app)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwSetErrorCallback(gears_glfw_error);
    app->window = glfwCreateWindow(app->width, app->height, "Vulkan Benchmark",
        NULL, NULL);
    if (!app->window) {
        panic("glfwCreateWindow failed\n");
    }

    glfwSetWindowUserPointer(app->window, app);
    //glfwSetKeyCallback(app->window, gears_glfw_key);
}

static void gears_destroy_instance(gears_app *app)
{
    vkDestroyInstance(app->instance, NULL);
    app->instance = VK_NULL_HANDLE;
}

static int check_layers(VkLayerProperties *instance_layers,
    size_t instance_count, const char **check_layers, size_t check_count)
{
    size_t found = 0;
    for (size_t i = 0; i < check_count; i++) {
        for (size_t j = 0; j < instance_count; j++) {
            if (!strcmp(check_layers[i], instance_layers[j].layerName))
            {
                found++;
                break;
            }
        }
    }
    return found == check_count;
}

#define array_size(array) sizeof(array) / sizeof(array[0])

static void gears_create_instance(gears_app *app)
{
    uint32_t required_extension_count = 0;
    uint32_t instance_count = 0;
    uint32_t validation_layer_count = 0;
    uint32_t enabled_extensions_count = 0;
    uint32_t enabled_layer_count = 0;
    const char **required_extensions = NULL;
    const char *extension_names[64];
    const char *layer_names[64];

    static const char *api_dump_layers[] = {
        "VK_LAYER_LUNARG_api_dump"
    };

    static const char *validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VK_CALL(vkEnumerateInstanceLayerProperties(&instance_count, NULL));
    if (instance_count > 0) {
        VkLayerProperties *instance_layers =
                malloc(sizeof (VkLayerProperties) * instance_count);
        VK_CALL(vkEnumerateInstanceLayerProperties(&instance_count,
                instance_layers));
        if (app->api_dump_enable && check_layers(instance_layers,
            instance_count, api_dump_layers, array_size(api_dump_layers))) {
            for (size_t i = 0; i < array_size(api_dump_layers); i++) {
                layer_names[enabled_layer_count++] = api_dump_layers[i];
            }
        }
        if (app->validation_enable && check_layers(instance_layers,
            instance_count, validation_layers, array_size(validation_layers))) {
            for (size_t i = 0; i < array_size(validation_layers); i++) {
                layer_names[enabled_layer_count++] = validation_layers[i];
            }
        }
        free(instance_layers);
    }

    required_extensions =
        glfwGetRequiredInstanceExtensions(&required_extension_count);
    if (!required_extensions) {
        panic("glfwGetRequiredInstanceExtensions failed\n");
    }
    for (size_t i = 0; i < required_extension_count; i++) {
        extension_names[enabled_extensions_count++] = required_extensions[i];
        assert(enabled_extensions_count < 64);
    }

    const VkApplicationInfo ai = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = app->name,
        .applicationVersion = 0,
        .pEngineName = app->name,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_1,
    };
    const VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &ai,
        .enabledLayerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const *)layer_names,
        .enabledExtensionCount = enabled_extensions_count,
        .ppEnabledExtensionNames = (const char *const *)extension_names,
    };
    VK_CALL(vkCreateInstance(&ici, NULL, &app->instance));
}

static void gears_destroy_surface(gears_app *app)
{
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
    app->surface = VK_NULL_HANDLE;
}

static void gears_create_surface(gears_app *app)
{
    VK_CALL(glfwCreateWindowSurface
        (app->instance, app->window, NULL, &app->surface));
}

static void gears_find_physical_device(gears_app *app)
{
    uint32_t physical_device_count = 1;
    VkResult err;

    /* get first matching physical devices avoiding call to get count */
    err = vkEnumeratePhysicalDevices(app->instance, &physical_device_count,
        &app->physdev);
    if (err != VK_SUCCESS && err != VK_INCOMPLETE) {
        panic("vkEnumeratePhysicalDevices failed: err=%d\n", err);
    }
    if (physical_device_count == 0) {
        panic("vkEnumeratePhysicalDevices: no devices found\n");
    }
}

static void gears_find_physical_device_queue(gears_app *app)
{
    uint32_t queue_count = 0;
    VkQueueFamilyProperties *qfamily_props = NULL;

    /* get physical device queue properties */
    vkGetPhysicalDeviceQueueFamilyProperties(app->physdev, &queue_count, NULL);
    assert(queue_count >= 1);
    qfamily_props = malloc(sizeof(VkQueueFamilyProperties) * queue_count);
    assert(qfamily_props != NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(app->physdev, &queue_count,
                                             qfamily_props);
    assert(queue_count >= 1);

    /* find physical device queue index */
    app->queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_count; i++) {
        VkBool32 qsupports_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(app->physdev, i, app->surface,
                                             &qsupports_present);
        if (qsupports_present &&
            (qfamily_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0)
        {
            app->queue_family_index = i;
            break;
        }
    }
    if (app->queue_family_index == UINT32_MAX) {
        panic("vkGetPhysicalDeviceSurfaceSupportKHR: incompatible queue\n");
    }
    free(qfamily_props);
}

static void gears_destroy_logical_device(gears_app *app)
{
    vkDestroyDevice(app->device, NULL);
    app->device = VK_NULL_HANDLE;
}

static void gears_create_logical_device(gears_app *app)
{
    float queue_priorities[1] = { 0 };
    const char* extension_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME
    };

    /* create logical device */
    const VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = app->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities
    };
    VkPhysicalDeviceFeatures features;
    memset(&features, 0, sizeof(features));
    const VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extension_names,
        .pEnabledFeatures = &features,
    };
    VK_CALL(vkCreateDevice(app->physdev, &dci, NULL, &app->device));
    vkGetDeviceQueue(app->device, app->queue_family_index, 0, &app->queue);
}

static void gears_find_surface_format(gears_app *app)
{
    uint32_t format_count = 0;
    VkSurfaceFormatKHR *surface_formats = NULL;

    /* get physical device formats */
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR
        (app->physdev, app->surface, &format_count, NULL));
    assert(format_count > 0);
    surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    assert(surface_formats != NULL);
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR
        (app->physdev, app->surface, &format_count, surface_formats));
    assert(format_count > 0);

    /* find a format and color space, preferring VK_FORMAT_B8G8R8A8_UNORM */
    /* if the format is undefined, use VK_FORMAT_B8G8R8A8_UNORM */
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
         app->format = VK_FORMAT_B8G8R8A8_UNORM;
         app->color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    } else {
        uint32_t format_idx = 0;
        for (uint32_t i = 0; i < format_count; i++) {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
                format_idx = i;
                break;
            }
        }
        app->format = surface_formats[format_idx].format;
        app->color_space = surface_formats[format_idx].colorSpace;
    }
}

static void gears_destroy_swapchain(gears_app *app)
{
    vkDestroySwapchainKHR(app->device, app->swapchain, NULL);
    app->swapchain = VK_NULL_HANDLE;
}

static void gears_create_swapchain(gears_app *app)
{
    uint32_t min_image_count = app->max_swapchain_images;
    VkSurfaceTransformFlagsKHR surface_transform;

    /* get surface capabilities */
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR
        (app->physdev, app->surface, &app->surface_capabilities));

    glfwGetFramebufferSize(app->window, &app->width, &app->height);
    assert(app->surface_capabilities.currentExtent.width == app->width);
    assert(app->surface_capabilities.currentExtent.height == app->height);

    if (app->surface_capabilities.maxImageCount != 0 &&
        min_image_count > app->surface_capabilities.maxImageCount) {
        min_image_count = app->surface_capabilities.maxImageCount;
    }
    if (min_image_count < app->surface_capabilities.minImageCount) {
        min_image_count = app->surface_capabilities.minImageCount;
    }
    if (app->surface_capabilities.supportedTransforms &
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        surface_transform = app->surface_capabilities.currentTransform;
    }

    /* create swapchain */
    const VkExtent2D swapchain_extent = {
        app->surface_capabilities.currentExtent.width,
        app->surface_capabilities.currentExtent.height
    };
    const VkSwapchainCreateInfoKHR sci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = app->surface,
        .minImageCount = min_image_count,
        .imageFormat = app->format,
        .imageColorSpace = app->color_space,
        .imageExtent = {
            .width = swapchain_extent.width,
            .height = swapchain_extent.height,
        },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = surface_transform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = app->swapchain
    };
    VK_CALL(vkCreateSwapchainKHR
        (app->device, &sci, NULL, &app->swapchain));

    /* destroy old swapchain */
    if (sci.oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(app->device, sci.oldSwapchain, NULL);
    }

}

static void gears_destroy_swapchain_buffers(gears_app *app)
{
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
        app->swapchain_buffers[i].image = VK_NULL_HANDLE;
        vkDestroyImageView(app->device, app->swapchain_buffers[i].view, NULL);
    }
    free(app->swapchain_buffers);
    app->swapchain_buffers = NULL;
}

static void gears_create_swapchain_buffers(gears_app *app)
{
    VkImage *swapchain_images = NULL;

    /* Get swapchain images */
    VK_CALL(vkGetSwapchainImagesKHR
        (app->device, app->swapchain, &app->swapchain_image_count, NULL));
    swapchain_images = malloc(sizeof(VkImage) * app->swapchain_image_count);
    assert(swapchain_images != NULL);
    VK_CALL(vkGetSwapchainImagesKHR
        (app->device, app->swapchain, &app->swapchain_image_count,
         swapchain_images));

    /* Create image views for swapchain images */
    app->swapchain_buffers = malloc(sizeof(gears_swapchain_buffer) *
        app->swapchain_image_count);
    memset(app->swapchain_buffers, 0, sizeof(gears_swapchain_buffer) *
        app->swapchain_image_count);
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
         VkImageViewCreateInfo cav = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = app->swapchain_buffers[i].image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = app->format,
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
                .layerCount = 1
            }
        };
        VK_CALL(vkCreateImageView
            (app->device, &cav, NULL, &app->swapchain_buffers[i].view));
    }
    free(swapchain_images);
}

static void gears_create_image(gears_app *app, gears_image_buffer *imbuf,
    VkFormat format, VkSampleCountFlags sample_count,
    VkImageUsageFlags usage, VkImageAspectFlagBits aspectMask)
{
    VkPhysicalDeviceMemoryProperties memory_props;
    VkMemoryRequirements memory_reqs;

    imbuf->format = format;

    const VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = imbuf->format,
        .extent = { app->width, app->height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = sample_count,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VK_CALL(vkCreateImage
        (app->device, &ici, NULL, &imbuf->image));

    vkGetPhysicalDeviceMemoryProperties(app->physdev, &memory_props);
    vkGetImageMemoryRequirements(app->device, imbuf->image,
        &memory_reqs);

    const VkMemoryAllocateInfo mai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memory_reqs.size,
        .memoryTypeIndex = memory_type_index(&memory_reqs, 0, &memory_props)
    };
    VK_CALL(vkAllocateMemory
        (app->device, &mai, NULL, &imbuf->memory));
    VK_CALL(vkBindImageMemory
        (app->device, imbuf->image, imbuf->memory, 0));

    const VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = imbuf->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imbuf->format,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };
    VK_CALL(vkCreateImageView
        (app->device, &ivci, NULL, &imbuf->view));
}

static void gears_destroy_image(gears_app *app, gears_image_buffer *imbuf)
{
    vkDestroyImageView(app->device, imbuf->view, NULL);
    imbuf->view = VK_NULL_HANDLE;
    vkDestroyImage(app->device, imbuf->image, NULL);
    imbuf->image = VK_NULL_HANDLE;
    vkFreeMemory(app->device, imbuf->memory, NULL);
    imbuf->memory = VK_NULL_HANDLE;
}


static uint32_t max_sample_count(VkSampleCountFlags counts)
{
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
    return VK_SAMPLE_COUNT_1_BIT;
}

static void gears_destroy_resolve_buffer(gears_app *app)
{
    if (app->resolve_buffer.image != VK_NULL_HANDLE) {
        gears_destroy_image(app, &app->resolve_buffer);
    }
}

static void gears_create_resolve_buffer(gears_app *app)
{
    vkGetPhysicalDeviceProperties(app->physdev, &app->physdev_props);

    /* set the maximum sample count if multisampling is enabled */
    if (app->multisampling_enable) {
        app->sample_count = max_sample_count(
            app->physdev_props.limits.framebufferColorSampleCounts &
            app->physdev_props.limits.framebufferDepthSampleCounts);
        if (app->sample_count > app->max_samples) {
            app->sample_count = app->max_samples;
        }
    } else {
        app->sample_count = VK_SAMPLE_COUNT_1_BIT;
    }

    /* disable multisampling if only one sample */
    if (app->sample_count == VK_SAMPLE_COUNT_1_BIT) {
        app->multisampling_enable = VK_FALSE;
        return;
    }

    gears_create_image(app, &app->resolve_buffer, app->format,
        app->sample_count, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
}

static void gears_destroy_depth_buffer(gears_app *app)
{
    gears_destroy_image(app, &app->depth_buffer);
}

static void gears_create_depth_buffer(gears_app *app)
{
    gears_create_image(app, &app->depth_buffer, VK_FORMAT_D32_SFLOAT,
        app->sample_count, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT);
}

static void gears_destroy_command_pool(gears_app *app)
{
    vkDestroyCommandPool(app->device, app->cmd_pool, NULL);
    app->cmd_pool = VK_NULL_HANDLE;
}

static void gears_create_command_pool(gears_app *app)
{
    const VkCommandPoolCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = app->queue_family_index,
    };
    VK_CALL(vkCreateCommandPool(app->device, &cpci, NULL,  &app->cmd_pool));
}

static void gears_destroy_command_buffers(gears_app *app)
{
    vkFreeCommandBuffers(app->device, app->cmd_pool, app->swapchain_image_count,
        app->cmd_buffers);
    free(app->cmd_buffers);
    app->cmd_buffers = NULL;
}

static void gears_create_command_buffers(gears_app *app)
{
    const VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = app->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = app->swapchain_image_count,
    };
    app->cmd_buffers = malloc(sizeof(VkCommandBuffer) * app->swapchain_image_count);
    assert(app->cmd_buffers != NULL);
    VK_CALL(vkAllocateCommandBuffers(app->device, &cbai, app->cmd_buffers));
}

static void gears_destroy_semaphores(gears_app *app)
{
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
        vkDestroySemaphore(app->device, app->image_available_semaphores[i], NULL);
        vkDestroySemaphore(app->device, app->render_complete_semaphores[i], NULL);
    }
    free(app->image_available_semaphores);
    free(app->render_complete_semaphores);
    app->image_available_semaphores = NULL;
    app->render_complete_semaphores = NULL;
}

static void gears_create_semaphores(gears_app *app)
{
    VkSemaphoreCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };
    app->image_available_semaphores = malloc(sizeof(VkSemaphore) * app->swapchain_image_count);
    app->render_complete_semaphores = malloc(sizeof(VkSemaphore) * app->swapchain_image_count);
    assert(app->image_available_semaphores != NULL);
    assert(app->render_complete_semaphores != NULL);
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
        VK_CALL(vkCreateSemaphore(app->device, &sci, NULL, &app->image_available_semaphores[i]));
        VK_CALL(vkCreateSemaphore(app->device, &sci, NULL, &app->render_complete_semaphores[i]));
    }
}

static void gears_destroy_fences(gears_app *app)
{
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
        vkDestroyFence(app->device, app->swapchain_fences[i], NULL);
    }
    free(app->swapchain_fences);
    app->swapchain_fences = NULL;
}

static void gears_create_fences(gears_app *app)
{
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    app->swapchain_fences = malloc(sizeof(VkFence) * app->swapchain_image_count);
    assert(app->swapchain_fences != NULL);
    for (uint32_t i = 0; i < app->swapchain_image_count; i++)
    {
        VK_CALL(vkCreateFence(app->device, &fci, NULL, &app->swapchain_fences[i]));
    }
}

static void gears_destroy_render_pass(gears_app *app)
{
    vkDestroyRenderPass(app->device, app->render_pass, NULL);
    app->render_pass = VK_NULL_HANDLE;
}

static void gears_create_render_pass(gears_app *app)
{
   const VkAttachmentDescription attachments[3] = {
        {
            .format = app->format,
            .samples = app->sample_count,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = app->multisampling_enable
                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .format = app->depth_buffer.format,
            .samples = app->sample_count,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
        {
            .format = app->format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };
    const VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference resolve_ref = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
        .pResolveAttachments = app->multisampling_enable ? &resolve_ref : NULL,
        .pDepthStencilAttachment = &depth_ref,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .attachmentCount = app->multisampling_enable ? 3 : 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };

    VK_CALL(vkCreateRenderPass(app->device, &render_pass_create_info,
        NULL, &app->render_pass));
}

static void gears_destroy_frame_buffers(gears_app *app)
{
    for (uint32_t i = 0; i < app->swapchain_image_count; i++) {
        vkDestroyFramebuffer(app->device,
            app->swapchain_buffers[i].framebuffer, NULL);
        app->swapchain_buffers[i].framebuffer = VK_NULL_HANDLE;
    }
}

static void gears_create_frame_buffers(gears_app *app)
{
    VkImageView attachments[3] = {
        app->resolve_buffer.view, app->depth_buffer.view, VK_NULL_HANDLE
    };

    const VkFramebufferCreateInfo framebuffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = app->render_pass,
        .attachmentCount = app->multisampling_enable ? 3 : 2,
        .pAttachments = attachments,
        .width = app->width,
        .height = app->height,
        .layers = 1,
    };

    uint32_t color_idx = app->multisampling_enable ? 2 : 0;
    for (uint32_t i = 0; i < app->swapchain_image_count; i++) {
        attachments[color_idx] = app->swapchain_buffers[i].view;
        VK_CALL(vkCreateFramebuffer(app->device,
            &framebuffer_create_info, NULL,
            &app->swapchain_buffers[i].framebuffer));
    }
}

static VkShaderModule gears_create_shader_from_memory(gears_app *app,
    const void *code, size_t size)
{
    VkShaderModule module;
    const VkShaderModuleCreateInfo module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = size,
        .pCode = code
    };
    VK_CALL(vkCreateShaderModule(app->device, &module_create_info, NULL,
        &module));
    return module;
}

static VkShaderModule gears_create_shader_from_file(gears_app *app,
    const char *filename)
{
    FILE *f;
    struct stat statbuf;
    char *buf;
    size_t nread;
    VkShaderModule module;

    if ((f = fopen(filename, "r")) == NULL) {
        panic("gears_create_shader_from_file: open: %s: %s",
            filename, strerror(errno));
    }
    if (fstat(fileno(f), &statbuf) < 0) {
        panic("gears_create_shader_from_file: stat: %s: %s",
            filename, strerror(errno));
    }
    buf = malloc(statbuf.st_size);
    assert(buf != NULL);
    if ((nread = fread(buf, 1, statbuf.st_size, f)) != (unsigned long)statbuf.st_size) {
        panic("gears_create_shader_from_file: fread: %s: expected %zu got %zu\n",
            filename, statbuf.st_size, nread);
    }

    module = gears_create_shader_from_memory(app, buf, statbuf.st_size);
    free(buf);

    return module;
}

static void gears_destroy_shaders(gears_app *app)
{
    vkDestroyShaderModule(app->device, app->vert_shader, NULL);
    app->vert_shader = VK_NULL_HANDLE;
    vkDestroyShaderModule(app->device, app->frag_shader, NULL);
    app->frag_shader = VK_NULL_HANDLE;
}

static void gears_create_shaders(gears_app *app)
{
    app->vert_shader = gears_create_shader_from_file(app, vert_shader_filename);
    app->frag_shader = gears_create_shader_from_file(app, frag_shader_filename);
}

static void gears_destroy_desc_pool(gears_app *app)
{
    vkDestroyDescriptorPool(app->device, app->desc_pool, NULL);
    app->desc_pool = VK_NULL_HANDLE;
}

static void gears_create_desc_pool(gears_app *app)
{
    const VkDescriptorPoolSize desc_pool_sizes[1] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 15,
        },
    };
    VkDescriptorPoolCreateInfo desc_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 16,
        .poolSizeCount = 1,
        .pPoolSizes = desc_pool_sizes
    };
    VK_CALL(vkCreateDescriptorPool(app->device,
        &desc_pool_create_info, NULL, &app->desc_pool));
}

static void gears_destroy_desc_set_layouts(gears_app *app)
{
    vkDestroyDescriptorSetLayout(app->device, app->desc_set_layout, NULL);
    app->desc_set_layout = VK_NULL_HANDLE;
}

static void gears_create_desc_set_layouts(gears_app *app)
{
    VkDescriptorSetLayoutBinding desc_set_layout_binding[1] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        },
    };
    VkDescriptorSetLayoutCreateInfo desc_set_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = desc_set_layout_binding,
    };
    VK_CALL(vkCreateDescriptorSetLayout(app->device,
        &desc_set_create_info, NULL, &app->desc_set_layout));
}

static void gears_destroy_desc_sets(gears_app *app)
{
    free(app->desc_sets);
    app->desc_set_count = 0;
    app->desc_sets = NULL;
}

static void gears_create_desc_sets(gears_app *app)
{
    VkDescriptorSetLayout *layouts;

    app->desc_set_count = app->swapchain_image_count * 3;

    layouts = malloc(sizeof(VkDescriptorSetLayout) * app->desc_set_count);
    assert(layouts != NULL);
    for (size_t i = 0; i < app->desc_set_count; i++) {
        layouts[i] = app->desc_set_layout;
    }
    app->desc_sets = malloc(sizeof(VkDescriptorSet) * app->desc_set_count);
    assert(app->desc_sets != NULL);
    VkDescriptorSetAllocateInfo ds_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = app->desc_pool,
        .descriptorSetCount = app->desc_set_count,
        .pSetLayouts = layouts
    };
    VK_CALL(vkAllocateDescriptorSets(app->device, &ds_alloc_info, app->desc_sets));
    free(layouts);
}

static void gears_destroy_pipeline_layout(gears_app *app)
{
    vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
    app->pipeline_layout = VK_NULL_HANDLE;
}

static void gears_create_pipeline_layout(gears_app *app)
{
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &app->desc_set_layout,
    };
    VK_CALL(vkCreatePipelineLayout(app->device,
        &pipeline_layout_create_info, NULL, &app->pipeline_layout));
}

static void gears_destroy_pipeline(gears_app *app)
{
    vkDestroyPipelineCache(app->device, app->pipeline_cache, NULL);
    app->pipeline_cache = VK_NULL_HANDLE;
    vkDestroyPipeline(app->device, app->pipeline, NULL);
    app->pipeline = VK_NULL_HANDLE;
}

static void gears_create_pipeline(gears_app *app)
{
    const VkVertexInputBindingDescription vb[1] = {
        {
            .binding = 0,
            .stride = sizeof(gears_vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
    };
    const VkVertexInputAttributeDescription va[4] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(gears_vertex,pos),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(gears_vertex,norm),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(gears_vertex,col),
        }
    };
    const VkPipelineVertexInputStateCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = vb,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = va,
    };
    const VkPipelineInputAssemblyStateCreateInfo ia = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    const VkPipelineRasterizationStateCreateInfo rs = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    const VkPipelineColorBlendAttachmentState as = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo cb = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = 1,
        .pAttachments = &as,
        .blendConstants = { 0, 0, 0, 0 },
    };
    const VkPipelineViewportStateCreateInfo vp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = NULL,
        .scissorCount = 1,
        .pScissors = NULL,
    };
    const VkPipelineDepthStencilStateCreateInfo ds = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .back.failOp = VK_STENCIL_OP_KEEP,
        .back.passOp = VK_STENCIL_OP_KEEP,
        .back.compareOp = VK_COMPARE_OP_ALWAYS,
        .front.failOp = VK_STENCIL_OP_KEEP,
        .front.passOp = VK_STENCIL_OP_KEEP,
        .front.compareOp = VK_COMPARE_OP_ALWAYS,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };
    const VkPipelineMultisampleStateCreateInfo ms = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .rasterizationSamples = app->sample_count,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    const VkPipelineShaderStageCreateInfo ss[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = app->vert_shader,
            .pName = "main",
            .pSpecializationInfo = NULL,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = app->frag_shader,
            .pName = "main",
            .pSpecializationInfo = NULL,
        }
    };
    const VkDynamicState se[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    const VkPipelineDynamicStateCreateInfo st = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = 2,
        .pDynamicStates = se,
    };
    const VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount = 2,
        .pStages = ss,
        .pVertexInputState = &vi,
        .pInputAssemblyState = &ia,
        .pTessellationState = NULL,
        .pViewportState = &vp,
        .pRasterizationState = &rs,
        .pMultisampleState = &ms,
        .pDepthStencilState = &ds,
        .pColorBlendState = &cb,
        .pDynamicState = &st,
        .layout = app->pipeline_layout,
        .renderPass = app->render_pass,
    };
    const VkPipelineCacheCreateInfo pipeline_cache_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    };

    VK_CALL(vkCreatePipelineCache(app->device,
        &pipeline_cache_create_info, NULL, &app->pipeline_cache));
    VK_CALL(vkCreateGraphicsPipelines(app->device, app->pipeline_cache, 1,
        &pipeline_create_info, NULL, &app->pipeline));
}

static inline void normalize2f(float v[2])
{
    // vec2 r; vec2_norm(r, v); v = r;
    float len = sqrtf(v[0]*v[0] + v[1]*v[1]);
    v[0] /= len;
    v[1] /= len;
}

/*
 * Create a gear wheel.
 *
 *   Input:  inner_radius - radius of hole at center
 *                  outer_radius - radius at center of teeth
 *                  width - width of gear teeth - number of teeth
 *                  tooth_depth - depth of tooth
 */

#define vertex_3f(x,y,z) \
    gears_buffer_add_vertex(vb, (gears_vertex){{x,y,z}, norm, uv, col })
#define normal_3f(x,y,z) norm = (vec3f){x,y,z}

static void
gear(gears_buffer *vb, gears_buffer *ib,
        float inner_radius, float outer_radius, float width,
        int teeth, float tooth_depth, vec4f col)
{
    int i;
    float r0, r1, r2;
    float angle, da;
    float u, v, len;
    float tmp[2];
    vec3f norm;
    vec2f uv;
    uint idx;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.f;
    r2 = outer_radius + tooth_depth / 2.f;
    da = 2.f*(float) M_PI / teeth / 4.f;
    uv = (vec2f){ 0,0 };

    normal_3f(0.f, 0.f, 1.f);

    /* draw front face */
    idx = gears_buffer_count(vb);
    for (i = 0; i <= teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        vertex_3f(r0*cosf(angle), r0*sinf(angle), width*0.5f);
        vertex_3f(r1*cosf(angle), r1*sinf(angle), width*0.5f);
        if (i < teeth) {
            vertex_3f(r0*cosf(angle), r0*sinf(angle), width*0.5f);
            vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), width*0.5f);
        }
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quad_strip, teeth*2, idx);

    /* draw front sides of teeth */
    da = 2.f*(float) M_PI / teeth / 4.f;
    idx = gears_buffer_count(vb);
    for (i = 0; i < teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        vertex_3f(r1*cosf(angle), r1*sinf(angle), width*0.5f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), width*0.5f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), width*0.5f);
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), width*0.5f);
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quads, teeth, idx);

    normal_3f(0.0, 0.0, -1.0);

    /* draw back face */
    idx = gears_buffer_count(vb);
    for (i = 0; i <= teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        vertex_3f(r1*cosf(angle), r1*sinf(angle), -width*0.5f);
        vertex_3f(r0*cosf(angle), r0*sinf(angle), -width*0.5f);
        if (i < teeth) {
            vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), -width*0.5f);
            vertex_3f(r0*cosf(angle), r0*sinf(angle), -width*0.5f);
        }
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quad_strip, teeth*2, idx);

    /* draw back sides of teeth */
    da = 2.f*(float) M_PI / teeth / 4.f;
    idx = gears_buffer_count(vb);
    for (i = 0; i < teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), -width*0.5f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), -width*0.5f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), -width*0.5f);
        vertex_3f(r1*cosf(angle), r1*sinf(angle), -width*0.5f);
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quads, teeth, idx);

    /* draw outward faces of teeth */
    idx = gears_buffer_count(vb);
    for (i = 0; i < teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        tmp[0] = r2*cosf(angle+1*da) - r1*cosf(angle);
        tmp[1] = r2*sinf(angle+1*da) - r1*sinf(angle);
        normalize2f(tmp);
        normal_3f(tmp[1], -tmp[0], 0.f);
        vertex_3f(r1*cosf(angle), r1*sinf(angle), width*0.5f);
        vertex_3f(r1*cosf(angle), r1*sinf(angle), -width*0.5f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), -width*0.5f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), width*0.5f);
        normal_3f(cosf(angle), sinf(angle), 0.f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), width*0.5f);
        vertex_3f(r2*cosf(angle+1*da), r2*sinf(angle+1*da), -width*0.5f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), -width*0.5f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), width*0.5f);
        tmp[0] = r1*cosf(angle+3*da) - r2*cosf(angle+2*da);
        tmp[1] = r1*sinf(angle+3*da) - r2*sinf(angle+2*da);
        normalize2f(tmp);
        normal_3f(tmp[1], -tmp[0], 0.f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), width*0.5f);
        vertex_3f(r2*cosf(angle+2*da), r2*sinf(angle+2*da), -width*0.5f);
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), -width*0.5f);
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), width*0.5f);
        normal_3f(cosf(angle), sinf(angle), 0.f);
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), width*0.5f);
        vertex_3f(r1*cosf(angle+3*da), r1*sinf(angle+3*da), -width*0.5f);
        vertex_3f(r1*cosf(angle+4*da), r1*sinf(angle+4*da), -width*0.5f);
        vertex_3f(r1*cosf(angle+4*da), r1*sinf(angle+4*da), width*0.5f);
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quads, teeth*4, idx);

    /* draw inside radius cylinder */
    idx = gears_buffer_count(vb);
    for (i = 0; i <= teeth; i++) {
        angle = i*2.f*(float) M_PI / teeth;
        normal_3f(-cosf(angle), -sinf(angle), 0.f);
        vertex_3f(r0*cosf(angle), r0*sinf(angle), -width*0.5f);
        vertex_3f(r0*cosf(angle), r0*sinf(angle), width*0.5f);
    }
    gears_buffer_add_indices_primitves(ib, gears_topology_quad_strip, teeth, idx);
}

static void gears_destroy_vertex_buffers(gears_app *app)
{
    for (size_t g = 0; g < 3; g++) {
        gears_buffer_destroy(app, &app->gear[g].vb);
        gears_buffer_destroy(app, &app->gear[g].ib);
    }
}

static void gears_create_vertex_buffers(gears_app *app)
{
    static const vec4f red = {0.8f, 0.1f, 0.f, 1.f};
    static const vec4f green = {0.f, 0.8f, 0.2f, 1.f};
    static const vec4f blue = {0.2f, 0.2f, 1.f, 1.f};

    for (size_t g = 0; g < 3; g++) {
        gears_buffer_init(&app->gear[g].vb, sizeof(gears_vertex),
            GEARS_VERTEX_INITIAL_COUNT);
        gears_buffer_init(&app->gear[g].ib, sizeof(uint32_t),
            GEARS_INDEX_INITIAL_COUNT);
    }

    gear(&app->gear[0].vb, &app->gear[0].ib, 1.f, 4.f, 1.f, 20, 0.7f, red);
    gear(&app->gear[1].vb, &app->gear[1].ib, 0.5f, 2.f, 2.f, 10, 0.7f, green);
    gear(&app->gear[2].vb, &app->gear[2].ib, 1.3f, 2.f, 0.5f, 10, 0.7f, blue);

    for (size_t g = 0; g < 3; g++) {
        gears_buffer_freeze(app, &app->gear[g].vb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        gears_buffer_freeze(app, &app->gear[g].ib, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
}

static void gears_destroy_uniform_buffers(gears_app *app)
{
    for (size_t g = 0; g < 3; g++) {
        for (uint j = 0; j < app->swapchain_image_count; j++) {
            vkDestroyBuffer(app->device, app->gear[g].ub.mem[j].buffer, NULL);
            app->gear[g].ub.mem[j].buffer = VK_NULL_HANDLE;
            vkFreeMemory(app->device, app->gear[g].ub.mem[j].memory, NULL);
            app->gear[g].ub.mem[j].memory = VK_NULL_HANDLE;
        }
    }
}

static void gears_update_uniform_buffer(gears_app *app, size_t j)
{
    mat4x4 m[3], v, p;

    const vec3 lightpos = { 5.f, 5.f, 10.f };

    const float h = (float) app->height / (float) app->width;

    mat4x4_frustum(p, -1.0, 1.0, -h, h, 5.0, 60.0);

    /* create gear model and view matrices */
    mat4x4_translate(v, 0.0, 0.0, app->view_dist);
    mat4x4_rotate(v, v, 1.0, 0.0, 0.0, (app->rotation.rotx / 180) * M_PI);
    mat4x4_rotate(v, v, 0.0, 1.0, 0.0, (app->rotation.roty / 180) * M_PI);
    mat4x4_rotate(v, v, 0.0, 0.0, 1.0, (app->rotation.rotz / 180) * M_PI);

    mat4x4_translate(m[0], -3.0, -2.0, 0.0);
    mat4x4_rotate_Z(m[0], m[0], (app->rotation.angle / 180) * M_PI);

    mat4x4_translate(m[1], 3.1f, -2.f, 0.f);
    mat4x4_rotate_Z(m[1], m[1], ((-2.f * app->rotation.angle - 9.f) / 180) * M_PI);

    mat4x4_translate(m[2], -3.1f, 4.2f, 0.f);
    mat4x4_rotate_Z(m[2], m[2], ((-2.f * app->rotation.angle - 25.f) / 180) * M_PI);

    for (size_t g = 0; g < 3; g++) {
        memcpy(app->gear[g].ub.data.model, m[g], sizeof(m[g]));
        memcpy(app->gear[g].ub.data.view, v, sizeof(v));
        memcpy(app->gear[g].ub.data.projection, p, sizeof(p));
        memcpy(app->gear[g].ub.data.lightpos, lightpos, sizeof(lightpos));

        void* data;
        VK_CALL(vkMapMemory(app->device, app->gear[g].ub.mem[j].memory, 0,
            sizeof(gears_uniform_buffer), 0, &data));
        memcpy(data, &app->gear[g].ub.data, sizeof(gears_uniform_buffer));
        vkUnmapMemory(app->device, app->gear[g].ub.mem[j].memory);

        VkDescriptorBufferInfo uniform_info = {
            .buffer = app->gear[g].ub.mem[j].buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        };
        VkWriteDescriptorSet wds = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = app->desc_sets[j * 3 + g],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = &uniform_info,
            .pTexelBufferView = NULL,
        };
        vkUpdateDescriptorSets(app->device, 1, &wds, 0, NULL);
    }
}

static void gears_create_uniform_buffer(gears_app *app)
{
    for (size_t g = 0; g < 3; g++) {
        app->gear[g].ub.mem = malloc(sizeof(gears_uniform_memory) *
            app->swapchain_image_count);
        assert(app->gear[g].ub.mem != NULL);
        for (uint j = 0; j < app->swapchain_image_count; j++) {
            gears_buffer_alloc(app, &app->gear[g].ub.mem[j].buffer,
                &app->gear[g].ub.mem[j].memory,
                sizeof(gears_uniform_buffer),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        }
    }
    for (uint j = 0; j < app->swapchain_image_count; j++) {
        gears_update_uniform_buffer(app, j);
    }
}

static void gears_reset_command_buffers(gears_app *app)
{
    for (size_t j = 0; j < app->swapchain_image_count; j++) {
        VK_CALL(vkWaitForFences(app->device, 1,
            &app->swapchain_fences[j], VK_TRUE, UINT64_MAX));
        VK_CALL(vkResetCommandBuffer(app->cmd_buffers[j],
            VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
    }
}

static void gears_record_command_buffers(gears_app *app, size_t j)
{
    VkClearValue clear_values[] = {
        { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
        { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
    };

    VkRenderPassBeginInfo pass = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = app->render_pass,
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent = app->surface_capabilities.currentExtent,
        .clearValueCount = 3,
        .pClearValues = clear_values,
    };

    VkRect2D scissor = {
        .extent = app->surface_capabilities.currentExtent
    };

    VkViewport viewport = {
        .x = 0,
        .y = (float)app->surface_capabilities.currentExtent.height,
        .height = -(float)app->surface_capabilities.currentExtent.height,
        .width = (float)app->surface_capabilities.currentExtent.width,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkCommandBufferBeginInfo cbuf_info = {0};
    cbuf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(vkResetCommandBuffer(app->cmd_buffers[j], 0));
    VK_CALL(vkBeginCommandBuffer(app->cmd_buffers[j], &cbuf_info));

    pass.framebuffer = app->swapchain_buffers[j].framebuffer;
    vkCmdBeginRenderPass(app->cmd_buffers[j], &pass,
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(app->cmd_buffers[j], 0, 1, &viewport);
    vkCmdSetScissor(app->cmd_buffers[j], 0, 1, &scissor);
    vkCmdBindPipeline(app->cmd_buffers[j],
        VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline);
    for (size_t g = 0; g < 3; g++) {
        VkDeviceSize offset = 0;
        vkCmdBindDescriptorSets(app->cmd_buffers[j],
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline_layout, 0, 1,
            app->desc_sets + j * 3 + g, 0, NULL);
        vkCmdBindVertexBuffers(app->cmd_buffers[j],
            0, 1, &app->gear[g].vb.buffer, &offset);
        vkCmdBindIndexBuffer(app->cmd_buffers[j],
            app->gear[g].ib.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(app->cmd_buffers[j],
            app->gear[g].ib.count, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(app->cmd_buffers[j]);

    VK_CALL(vkEndCommandBuffer(app->cmd_buffers[j]));
}

static void gears_init(gears_app *app, const int argc, const char **argv)
{
    gears_init_app(app, argv[0]);
    //FIXME gears_parse_options(app, argc, argv);
    gears_init_glfw(app);
    gears_create_window(app);
    gears_create_instance(app);
    gears_create_surface(app);
    gears_find_physical_device(app);
    gears_find_physical_device_queue(app);
    gears_create_logical_device(app);
    gears_find_surface_format(app);
    gears_create_swapchain(app);
    gears_create_swapchain_buffers(app);
    gears_create_resolve_buffer(app);
    gears_create_depth_buffer(app);
    gears_create_command_pool(app);
    gears_create_command_buffers(app);
    gears_create_semaphores(app);
    gears_create_fences(app);
    gears_create_render_pass(app);
    gears_create_frame_buffers(app);
    gears_create_shaders(app);
    gears_create_desc_pool(app);
    gears_create_desc_set_layouts(app);
    gears_create_desc_sets(app);
    gears_create_pipeline_layout(app);
    gears_create_pipeline(app);
    gears_create_vertex_buffers(app);
    gears_create_uniform_buffer(app);
}

static void gears_cleanup(gears_app *app)
{
    gears_reset_command_buffers(app);
    gears_destroy_uniform_buffers(app);
    gears_destroy_vertex_buffers(app);
    gears_destroy_pipeline(app);
    gears_destroy_pipeline_layout(app);
    gears_destroy_desc_sets(app);
    gears_destroy_desc_set_layouts(app);
    gears_destroy_desc_pool(app);
    gears_destroy_shaders(app);
    gears_destroy_frame_buffers(app);
    gears_destroy_render_pass(app);
    gears_destroy_fences(app);
    gears_destroy_semaphores(app);
    gears_destroy_command_buffers(app);
    gears_destroy_command_pool(app);
    gears_destroy_depth_buffer(app);
    gears_destroy_resolve_buffer(app);
    gears_destroy_swapchain_buffers(app);
    gears_destroy_swapchain(app);
    gears_destroy_logical_device(app);
    gears_destroy_surface(app);
    gears_destroy_instance(app);
    gears_destroy_window(app);
}

static void gears_recreate_swapchain(gears_app *app)
{
    gears_reset_command_buffers(app);
    gears_destroy_pipeline(app);
    gears_destroy_frame_buffers(app);
    gears_destroy_render_pass(app);
    gears_destroy_depth_buffer(app);
    gears_destroy_resolve_buffer(app);
    gears_destroy_swapchain_buffers(app);
    gears_destroy_swapchain(app);
    gears_create_swapchain(app);
    gears_create_swapchain_buffers(app);
    gears_create_resolve_buffer(app);
    gears_create_depth_buffer(app);
    gears_create_render_pass(app);
    gears_create_frame_buffers(app);
    gears_create_pipeline(app);
}

static void gears_print_info_verbose(gears_app *app)
{
    printf("device.name:           %s\n",
        app->physdev_props.deviceName);
    printf("vulkan.version:        %u.%u\n",
        VK_VERSION_MAJOR(app->physdev_props.apiVersion),
        VK_VERSION_MINOR(app->physdev_props.apiVersion));
    printf("multisampling:         %s\n",
        app->multisampling_enable ? "enabled" : "disabled");
    if (app->multisampling_enable) {
        printf("samples:               %d\n",
            app->sample_count);
    }
    printf("swapchain.image.count: %d\n",
        app->swapchain_image_count);
}

static void gears_run(gears_app *app)
{
    VkResult err;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .pWaitDstStageMask = &wait_stage,
        .pWaitSemaphores = NULL,
        .waitSemaphoreCount = 1,
        .pSignalSemaphores = NULL,
        .signalSemaphoreCount = 1,
        .commandBufferCount = 1,
    };

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = NULL,
        .swapchainCount = 1,
        .pSwapchains = &app->swapchain,
        .pImageIndices = NULL,
        .pResults = NULL,
    };

    if (app->verbose_enable) {
        gears_print_info_verbose(app);
    }

    uint32_t j = 0, k;
    float fps=0;
    double dt=0,t=current_time();
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
        fps++;
        k = j;
        VK_CALL(vkAcquireNextImageKHR(app->device,
            app->swapchain, UINT64_MAX,
            app->image_available_semaphores[k],
            app->swapchain_fences[j], &j));

        if (app->animation_enable) {
            app->rotation.angle = 100.f * (float) glfwGetTime();
            gears_update_uniform_buffer(app, j);
        }
        gears_record_command_buffers(app, j);

        submit_info.pCommandBuffers = &app->cmd_buffers[j];
        submit_info.pWaitSemaphores = &app->image_available_semaphores[k];
        submit_info.pSignalSemaphores = &app->render_complete_semaphores[j];
        VK_CALL(vkResetFences(app->device, 1,
            &app->swapchain_fences[j]));
        VK_CALL(vkQueueSubmit(app->queue, 1, &submit_info,
            app->swapchain_fences[j]));

        present_info.pImageIndices = &j;
        present_info.pWaitSemaphores = &app->render_complete_semaphores[j];
        err = vkQueuePresentKHR(app->queue, &present_info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            glfwGetFramebufferSize(app->window, &app->width, &app->height);
            gears_recreate_swapchain(app);
        }
        else if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR) {
            panic("vkQueuePresentKHR failed: err=%d\n", err);
        }
        VK_CALL(vkWaitForFences(app->device, 1,
            &app->swapchain_fences[j], VK_TRUE, UINT64_MAX));
	
	if(current_time()-dt>3){
	    if(dt!=0) {
	        dt=current_time();
	        printf("Ver=%d, Result:%f\n",VERSION,fps/(dt-t));
  	        fflush(stdout);
	        glfwSetWindowShouldClose(app->window, GLFW_TRUE);
	    } else {fps=0;dt=current_time();}
	}
    }
}

int main(const int argc, const char **argv)
{
    gears_app app;

    gears_init(&app, argc, argv);
    gears_run(&app);
    gears_cleanup(&app);

    return 0;
}
