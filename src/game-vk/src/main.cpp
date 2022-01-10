
/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/renderer/component/mesh.hpp>
#include <aurora/scene/game_object.hpp>

using namespace Aura;

auto create_example_scene() -> GameObject* {
  auto scene = new GameObject{};

  const u16 plane_indices[] = {
    0, 1, 2,
    2, 3, 0
  };

  const float plane_vertices[] = {
  // POSITION   UV         COLOR
    -1, +1, 2,  0.0, 0.0,  1.0, 0.0, 0.0,
    +1, +1, 2,  1.0, 0.0,  0.0, 1.0, 0.0,
    +1, -1, 2,  1.0, 1.0,  0.0, 0.0, 1.0,
    -1, -1, 2,  0.0, 1.0,  1.0, 0.0, 1.0
  };

  auto index_buffer = IndexBuffer{IndexDataType::UInt16, 6};
  std::memcpy(
    index_buffer.data(), plane_indices, sizeof(plane_indices));

  auto vertex_buffer_layout = VertexBufferLayout{
    .stride = sizeof(float) * 8,
    .attributes = {{
      .index = 0,
      .data_type = VertexDataType::Float32,
      .components = 3,
      .normalized = false,
      .offset = 0
    }, {
      .index = 2,
      .data_type = VertexDataType::Float32,
      .components = 2,
      .normalized = false,
      .offset = sizeof(float) * 3
    }, {
      .index = 3,
      .data_type = VertexDataType::Float32,
      .components = 3,
      .normalized = false,
      .offset = sizeof(float) * 5
    }}
  };
  auto vertex_buffer = VertexBuffer{vertex_buffer_layout, 4};
  std::memcpy(
    vertex_buffer.data(), plane_vertices, sizeof(plane_vertices));

  auto geometry = std::make_shared<Geometry>(index_buffer);
  geometry->buffers.push_back(std::move(vertex_buffer));

  auto material = std::make_shared<PbrMaterial>();

  auto plane = new GameObject{"Plane0"};
  plane->add_component<Mesh>(geometry, material);
  scene->add_child(plane);
  return scene;
}

// ---------------------------------------------------
// legacy level one 

#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>
#include <vulkan/vulkan.h>

auto find_memory_type_index(
  VkPhysicalDevice physical_device,
  u32 memory_type_bits,
  VkMemoryPropertyFlags property_flags
) -> u32 {
  auto memory_properties = VkPhysicalDeviceMemoryProperties{};

  // TODO: this should only be read out once during device creation.
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
    if (memory_type_bits & (1 << i)) {
      auto property_flags_have = memory_properties.memoryTypes[i].propertyFlags;
      if ((property_flags_have & property_flags) == property_flags) {
        return i;
      }
    }
  }

  Assert(false, "Vulkan: find_memory_type_index() failed to find suitable memory type.");
}

auto create_buffer_with_data(
  VkPhysicalDevice physical_device,
  VkDevice device,
  ArrayView<u8> const& data,
  bool index
) -> VkBuffer {
  auto buffer = VkBuffer{};
  auto memory = VkDeviceMemory{};
  auto memory_requirements = VkMemoryRequirements{};

  auto usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  if (index) {
    usage = (VkBufferUsageFlagBits)(usage | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  } else {
    usage = (VkBufferUsageFlagBits)(usage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  }

  auto buffer_info = VkBufferCreateInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = data.size(),
    .usage = usage,
    // Hmm check this, we might use this buffer with multiple queues
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr
  };

  if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to create a buffer :(");
  }

  vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

  auto allocate_info = VkMemoryAllocateInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = find_memory_type_index(
      physical_device,
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    )
  };

  if (vkAllocateMemory(device, &allocate_info, nullptr, &memory) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to allocate buffer memory :(");
  }

  if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to bind memory to buffer :(");
  }

  void* host_buffer;

  if (vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &host_buffer) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to map buffer to host memory :(");
  }

  std::memcpy(host_buffer, data.data(), data.size());

  auto mapped_memory_range = VkMappedMemoryRange{
    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
    .pNext = nullptr,
    .memory = memory,
    .offset = 0,
    .size = VK_WHOLE_SIZE
  };

  if (vkFlushMappedMemoryRanges(device, 1, &mapped_memory_range) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to flush mapped memory range :(");
  }

  vkUnmapMemory(device, memory);
  return buffer;
}

// ---------------------------------------------------
// legacy level two 

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vector>

#ifdef main
#undef main
#endif

using u32 = std::uint32_t;
using u64 = std::uint64_t;

float vertices[] = {
  -1, -1,  0,   1, 0, 0,
   1, -1,  0,   0, 1, 0,
   0,  1,  0,   0, 0, 1
};

u16 indices[] = {
  0, 1, 2
};

// glslangValidator -S vert -V100 --vn triangle_vert -o triangle.vert.h triangle.vert.glsl
// glslangValidator -S frag -V100 --vn triangle_frag -o triangle.frag.h triangle.frag.glsl
#include "triangle.vert.h"
#include "triangle.frag.h"

u32 queue_family_graphics;
u32 queue_family_transfer;

// TODO get_instance_layers() and get_device_layers() are almost the same.

auto get_instance_layers() -> std::vector<char const*> {
  const std::vector<char const*> kDesiredInstanceLayers {
    "VK_LAYER_KHRONOS_validation"
  };

  auto result = std::vector<char const*>{};

  u32 instance_layer_count;
  vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);

  auto instance_layers = std::vector<VkLayerProperties>{};
  instance_layers.resize(instance_layer_count);
  vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data());

  for (auto layer_name : kDesiredInstanceLayers) {
    const auto predicate = [&](VkLayerProperties& instance_layer) {
      return std::strcmp(layer_name, instance_layer.layerName) == 0;
    };

    auto match = std::find_if(
      instance_layers.begin(), instance_layers.end(), predicate);

    if (match != instance_layers.end()) {
      result.push_back(layer_name);
    }
  }

  return result;
}

auto get_device_layers(VkPhysicalDevice physical_device) -> std::vector<char const*> {
  const std::vector<char const*> kDesiredDeviceLayers {
    "VK_LAYER_KHRONOS_validation"
  };

  auto result = std::vector<char const*>{};

  u32 device_layer_count;
  vkEnumerateDeviceLayerProperties(physical_device, &device_layer_count, nullptr);

  auto device_layers = std::vector<VkLayerProperties>{};
  device_layers.resize(device_layer_count);
  vkEnumerateDeviceLayerProperties(physical_device, &device_layer_count, device_layers.data());

  for (auto layer_name : kDesiredDeviceLayers) {
    const auto predicate = [&](VkLayerProperties& device_layer) {
      return std::strcmp(layer_name, device_layer.layerName) == 0;
    };

    auto match = std::find_if(
      device_layers.begin(), device_layers.end(), predicate);

    if (match != device_layers.end()) {
      result.push_back(layer_name);
    }
  }

  return result;
}

auto create_instance(
  std::vector<char const*> const& extensions
) -> VkInstance {
  auto instance = VkInstance{};

  auto app = VkApplicationInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = "Aurora VR3",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Aurora VR3",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_MAKE_VERSION(1, 2, 189)  
  };
  
  auto layers = get_instance_layers();

  auto info = VkInstanceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .pApplicationInfo = &app,
    .enabledLayerCount = static_cast<u32>(layers.size()),
    .ppEnabledLayerNames = layers.data(),
    .enabledExtensionCount = static_cast<u32>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data()    
  };

  if (vkCreateInstance(&info, nullptr, &instance) == VK_SUCCESS) {
    return instance;
  }

  return VK_NULL_HANDLE;
}

auto pick_physical_device(VkInstance instance) -> VkPhysicalDevice {
  u32 physical_device_count;
  vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

  auto physical_devices = std::vector<VkPhysicalDevice>{};
  physical_devices.resize(physical_device_count);
  vkEnumeratePhysicalDevices(
    instance, &physical_device_count, physical_devices.data());

  auto physical_device_props = VkPhysicalDeviceProperties{};

  VkPhysicalDevice fallback_gpu = VK_NULL_HANDLE;

  /* TODO: implement a more robust logic to pick the best device.
   * At the moment we pick the first discrete GPU we discover.
   * If no discrete GPU is available we fallback to any integrated GPU.
   */
  for (auto physical_device : physical_devices) {
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);

    auto device_type = physical_device_props.deviceType;

    if (device_type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return physical_device;
    }

    if (device_type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      fallback_gpu = physical_device;
    }
  }

  return fallback_gpu;
}

auto create_logical_device(VkInstance instance, VkPhysicalDevice physical_device) -> VkDevice {
  // Let use hope that any GPU has VK_KHR_swapchain...
  const std::vector<char const*> kDeviceExtensions {
    "VK_KHR_swapchain",
#ifdef __APPLE__
    "VK_KHR_portability_subset"
#endif
  };

  auto physical_device_props = VkPhysicalDeviceProperties{};
  vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);
  std::printf("Physical Device: %s\n", physical_device_props.deviceName);

  auto layers = get_device_layers(physical_device);

  // Just enable all availble device features for now.
  auto features = VkPhysicalDeviceFeatures{};
  vkGetPhysicalDeviceFeatures(physical_device, &features);

  auto queue_create_info = std::vector<VkDeviceQueueCreateInfo>{};

  // TODO: move this somewhere outside...
  // Also do a more intelligent/robust queue allocation?
  {
    u32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    auto queue_families = std::vector<VkQueueFamilyProperties>{};
    queue_families.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    bool have_graphics_queue = false;
    bool have_transfer_queue = false;

    for (u32 i = 0; i < queue_family_count; i++) {
      auto& queue_family = queue_families[i];

      bool support_graphics = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
      bool support_transfer = queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT;

      if (support_graphics || support_transfer) {
        const float priority = 0.0;

        queue_create_info.push_back({
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = i,
          .queueCount = 1,
          .pQueuePriorities = &priority
        });
      }

      if (support_graphics) {
        have_graphics_queue = true;
        queue_family_graphics = i;
      }

      if (support_transfer) {
        have_transfer_queue = true;
        queue_family_transfer = i;
      }
    }

    if (!have_graphics_queue) {
      std::puts("Failed to create a graphics queue :("); 
      return VK_NULL_HANDLE;
    }

    if (!have_transfer_queue) {
      std::puts("Failed to create a transfer queue :(");
      return VK_NULL_HANDLE;
    }
  }

  auto device_create_info = VkDeviceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueCreateInfoCount = static_cast<u32>(queue_create_info.size()),
    .pQueueCreateInfos = queue_create_info.data(),
    .enabledLayerCount = static_cast<u32>(layers.size()),
    .ppEnabledLayerNames = layers.data(),
    .enabledExtensionCount = static_cast<u32>(kDeviceExtensions.size()),
    .ppEnabledExtensionNames = kDeviceExtensions.data(),
    .pEnabledFeatures = &features
  };

  VkDevice device;
  VkResult result = vkCreateDevice(physical_device, &device_create_info, nullptr, &device);

  if (result == VK_SUCCESS) {
    return device;
  }

  return VK_NULL_HANDLE;
}

auto create_swapchain(VkDevice device, VkSurfaceKHR surface) -> VkSwapchainKHR {
  // TODO
  auto extent = VkExtent2D{
    .width = 1600,
    .height = 900
  };

  // TODO: make sure that at least presentMode, imageFormat and imageColorSpace are supported.
  auto info = VkSwapchainCreateInfoKHR{
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .surface = surface,
    .minImageCount = 2,
    .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
    .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  VkSwapchainKHR swapchain;

  if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) == VK_SUCCESS) {
    return swapchain;
  }

  return VK_NULL_HANDLE;
}

auto sdl_create_instance(SDL_Window* window) -> VkInstance {
  u32 extension_count;
  SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr);

  std::vector<char const*> extensions;
  extensions.resize(extension_count);
  SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data());

  return create_instance(extensions);
}

auto sdl_create_surface(
  SDL_Window* window,
  VkInstance instance,
  VkPhysicalDevice physical_device
) -> VkSurfaceKHR {
  VkSurfaceKHR surface;

  if (SDL_Vulkan_CreateSurface(window, instance, &surface)) {
    // Apparently this is necessary so that we can create a swapchain. Why?
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_graphics, surface, &supported);
    if (supported == VK_FALSE) {
      return VK_NULL_HANDLE;
    }
    return surface;
  }

  return VK_NULL_HANDLE;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Aurora VR3",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    1600,
    900,
    SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI
  );

  auto instance = sdl_create_instance(window);

  if (instance == VK_NULL_HANDLE) {
    std::puts("Failed to create a Vulkan instance :(");
    return -1;
  }

  auto physical_device = pick_physical_device(instance);

  if (physical_device == VK_NULL_HANDLE) {
    std::puts("Failed to pick a physical device :(");
    return -1;
  }

  auto device = create_logical_device(instance, physical_device);

  if (device == VK_NULL_HANDLE) {
    std::puts("Failed to create a Vulkan device :(");
    return -1;
  }

  auto surface = sdl_create_surface(window, instance, physical_device);

  if (surface == VK_NULL_HANDLE) {
    std::puts("Failed to create a Vulkan surface :(");
    return -1;
  }

  auto swapchain = create_swapchain(device, surface);

  if (swapchain == VK_NULL_HANDLE) {
    std::puts("Failed to create a Vulkan swapchain :(");
    return -1;
  }

  VkRenderPass render_pass;
  
  {
    auto render_pass_attachment = VkAttachmentDescription{
      .flags = 0,
      .format = VK_FORMAT_B8G8R8A8_SRGB, // same as specified in swapchain
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    auto render_pass_attachment_ref = VkAttachmentReference{
      .attachment = 0, // index into VkRenderPassCreateInfo.pAttachments
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    auto render_pass_subpass = VkSubpassDescription{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &render_pass_attachment_ref,
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr
    };

    auto render_pass_info = VkRenderPassCreateInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = 1,
      .pAttachments = &render_pass_attachment,
      .subpassCount = 1,
      .pSubpasses = &render_pass_subpass,
      .dependencyCount = 0,
      .pDependencies = nullptr
    };
    
    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
      std::puts("Failed to create the render pass :(");
      return -1;
    }
  }

  auto swapchain_frame_buffers = std::vector<VkFramebuffer>{};

  {
    u32 swapchain_image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);

    auto swapchain_images = std::vector<VkImage>{};
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

    auto image_view_info = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_B8G8R8A8_SRGB, // same as specified in swapchain
      .components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A
      },
      .subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    auto frame_buffer_info = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = render_pass,
      .attachmentCount = 1,
      .width = 1600,
      .height = 900,
      .layers = 1
    };

    for (auto swapchain_image : swapchain_images) {
      VkImageView image_view;
      VkFramebuffer frame_buffer;

      image_view_info.image = swapchain_image;

      if (vkCreateImageView(device, &image_view_info, nullptr, &image_view) != VK_SUCCESS) {
        std::puts("Failed to create an image view for the swapchain :(");
        return -1;
      }

      frame_buffer_info.pAttachments = &image_view;

      if (vkCreateFramebuffer(device, &frame_buffer_info, nullptr, &frame_buffer) != VK_SUCCESS) {
        std::puts("Failed to create a frame buffer for the swapchain :(");
        return -1;
      }

      swapchain_frame_buffers.push_back(frame_buffer);
    }
  }

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
  auto command_buffer_begin_info = VkCommandBufferBeginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    // For now we'll re-record the command buffer every time it is submitted.
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // This is only necessary for secondary command buffers
    .pInheritanceInfo = nullptr
  };
  auto command_buffer_submit_info = VkSubmitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .pWaitDstStageMask = nullptr,
    .commandBufferCount = 1,
    .pCommandBuffers = &command_buffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = nullptr
  };

  { 
    auto command_pool_info = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family_graphics
    };

    if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS) {
      std::puts("Failed to a create a command pool :(");
      return -1;
    }

    auto command_buffer_info = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &command_buffer_info, &command_buffer) != VK_SUCCESS) {
      std::puts("Failed to create a command buffer :(");
      return -1;
    }
  }

  // TODO: move this closer to the device creation logic?
  auto queue_graphics = VkQueue{};
  vkGetDeviceQueue(device, queue_family_graphics, 0, &queue_graphics);

  auto queue_transfer = VkQueue{};
  vkGetDeviceQueue(device, queue_family_transfer, 0, &queue_transfer);

  auto fence = VkFence{};
  
  {
    auto fence_info = VkFenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
    };

    if (vkCreateFence(device, &fence_info, nullptr, &fence) != VK_SUCCESS) {
      std::puts("Failed to create a fence :(");
      return -1;
    }
  }

  auto buffer_vtx = create_buffer_with_data(
    physical_device, device, ArrayView{(u8*)vertices, sizeof(vertices)}, false);

  auto buffer_idx = create_buffer_with_data(
    physical_device, device, ArrayView{(u8*)indices, sizeof(indices)}, true);

  auto shader_vert = VkShaderModule{};
  auto shader_frag = VkShaderModule{};

  // Shader compilation
  {
    auto shader_module_info_vert = VkShaderModuleCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = sizeof(triangle_vert),
      .pCode = triangle_vert
    };

    if (vkCreateShaderModule(device, &shader_module_info_vert, nullptr, &shader_vert) != VK_SUCCESS) {
      std::puts("Failed to compile vertex shader :(");
      return -1;
    }

    auto shader_module_info_frag = VkShaderModuleCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = sizeof(triangle_frag),
      .pCode = triangle_frag
    };

    if (vkCreateShaderModule(device, &shader_module_info_frag, nullptr, &shader_frag) != VK_SUCCESS) {
      std::puts("Failed to compile fragment shader :(");
      return -1;
    }
  }

  auto pipeline_layout = VkPipelineLayout{};

  // Create pipeline layout
  {
    auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr
    };

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
      std::puts("Failed to create pipeline layout :(");
      return -1;
    }
  }

  auto pipeline = VkPipeline{};

  // Create pipeline
  {
    VkPipelineShaderStageCreateInfo pipeline_stages[] = {
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader_vert,
        .pName = "main",
        .pSpecializationInfo = nullptr
      },
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader_frag,
        .pName = "main",
        .pSpecializationInfo = nullptr
      }
    };

    auto vertex_binding_info = VkVertexInputBindingDescription{
      .binding = 0,
      .stride = sizeof(float) * 6,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertex_attribute_infos[] {
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = sizeof(float) * 3
      }
    };

    auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_binding_info,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = vertex_attribute_infos
    };

    auto input_assembly_info = VkPipelineInputAssemblyStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
    };

    auto viewport = VkViewport{
      .x = 0,
      .y = 0,
      .width = 1600,
      .height = 900,
      .minDepth = 0,
      .maxDepth = 1
    };

    auto scissor = VkRect2D{
      .offset = VkOffset2D{
        .x = 0,
        .y = 0
      },
      .extent = VkExtent2D{
        .width = 1600,
        .height = 900
      }
    };

    auto viewport_info = VkPipelineViewportStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      // TODO: viewports and scissors are ignored if viewport or scissor state is dynarmic.
      // We probably actually want to have it dynamic because fuck recreating pipelines when the resolution changes.
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor
    };

    auto rasterization_info = VkPipelineRasterizationStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE, // the fuck does this do????
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .depthBiasEnable = VK_FALSE,
      // probably don't need to set this if depth bias is disabled.
      .depthBiasConstantFactor = 0,
      .depthBiasClamp = 0,
      .depthBiasSlopeFactor = 0,
      .lineWidth = 1
    };

    auto multisample_info = VkPipelineMultisampleStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };

    auto color_blend_attachment_info = VkPipelineColorBlendAttachmentState{
      .blendEnable = VK_FALSE,
      .colorWriteMask = 0xF // ?
    };

    auto color_blend_info = VkPipelineColorBlendStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_NO_OP,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment_info,
      .blendConstants = {0, 0, 0, 0} // probably don't need to set this unless needed.
    };

    auto pipeline_info = VkGraphicsPipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = 2,
      .pStages = pipeline_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_info,
      .pRasterizationState = &rasterization_info,
      .pMultisampleState = &multisample_info,
      .pDepthStencilState = nullptr,
      .pColorBlendState = &color_blend_info,
      // TODO: set this to make e.g. the viewport and scissor test dynamic.
      .pDynamicState = nullptr,
      .layout = pipeline_layout,
      .renderPass = render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
      std::puts("Failed to create graphics pipeline :(");
      return -1;
    }
  }

  auto event = SDL_Event{};

  while (true) {
    u32 swapchain_image_id;

    // TODO: wait for this? why would we need to? Vsync?
    vkResetFences(device, 1, &fence);
    vkAcquireNextImageKHR(device, swapchain, u64(-1), VK_NULL_HANDLE, fence, &swapchain_image_id);
    vkWaitForFences(device, 1, &fence, VK_TRUE, u64(-1));

    auto frame_buffer = swapchain_frame_buffers[swapchain_image_id];

    auto clear_value = VkClearValue{
      .color = VkClearColorValue{
        .float32 = { 0.01, 0.01, 0.01, 1}
      }
    };

    auto render_pass_begin_info = VkRenderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = render_pass,
      .framebuffer = frame_buffer,
      .renderArea = VkRect2D{
        .offset = VkOffset2D{
          .x = 0,
          .y = 0
        },
        .extent = VkExtent2D{
          .width = 1600,
          .height = 900
        }
      },
      .clearValueCount = 1,
      .pClearValues = &clear_value
    };

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    {
      auto offset = VkDeviceSize(0);
      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
      vkCmdBindIndexBuffer(command_buffer, buffer_idx, 0, VK_INDEX_TYPE_UINT16);
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer_vtx, &offset);
      vkCmdDrawIndexed(command_buffer, 3, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);
    
    vkResetFences(device, 1, &fence);
    vkQueueSubmit(queue_graphics, 1, &command_buffer_submit_info, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, u64(-1));

    auto result = VkResult{};

    auto present_info = VkPresentInfoKHR{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &swapchain_image_id,
      .pResults = &result
    };

    vkQueuePresentKHR(queue_graphics, &present_info);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        goto done;
      }
    }
  }

done:
  SDL_Quit();
  return 0;
}