
/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/scene/game_object.hpp>

#include "../../gal/src/vulkan/render_pass.hpp"

#include "gltf_loader.hpp"
#include "renderer.hpp"

using namespace Aura;

#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>
#include <vulkan/vulkan.h>

void configure_pipeline_geometry(
  VkGraphicsPipelineCreateInfo& pipeline,
  Geometry const* geometry
) {
  // TODO: find a way to not leak memory while also keeping the code sane.
  auto bindings = new std::vector<VkVertexInputBindingDescription>{};
  auto attributes = new std::vector<VkVertexInputAttributeDescription>{};

  for (auto& buffer : geometry->get_vertex_buffers()) {
    //auto& layout = buffer.layout();
    auto binding = u32(bindings->size());

    bindings->push_back(VkVertexInputBindingDescription{
      .binding = binding,
      .stride = u32(buffer->stride()),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    });
  }

  for (auto const& attribute : geometry->get_attributes()) {
    attributes->push_back(VkVertexInputAttributeDescription{
      .location = u32(attribute.location),
      .binding = u32(attribute.buffer),
      .format = Renderer::GetVkFormatFromAttribute(attribute),
      .offset = u32(attribute.offset)
    });
  }

  pipeline.pVertexInputState = new VkPipelineVertexInputStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = u32(bindings->size()),
    .pVertexBindingDescriptions = bindings->data(),
    .vertexAttributeDescriptionCount = u32(attributes->size()),
    .pVertexAttributeDescriptions = attributes->data()
  };

  pipeline.pInputAssemblyState = new VkPipelineInputAssemblyStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };
}

auto create_pipeline(
  VkDevice device,
  VkShaderModule shader_vert,
  VkShaderModule shader_frag,
  VkRenderPass render_pass,
  VkPipelineLayout pipeline_layout,
  Geometry const* geometry
) -> VkPipeline {
  auto pipeline = VkPipeline{};

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
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .depthBiasEnable = VK_FALSE,
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

  auto depth_stencil_info = VkPipelineDepthStencilStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {},
    .minDepthBounds = 0,
    .maxDepthBounds = 0
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
    .blendConstants = {0, 0, 0, 0}
  };

  auto pipeline_info = VkGraphicsPipelineCreateInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = 2,
    .pStages = pipeline_stages,
    .pTessellationState = nullptr,
    .pViewportState = &viewport_info,
    .pRasterizationState = &rasterization_info,
    .pMultisampleState = &multisample_info,
    .pDepthStencilState = &depth_stencil_info,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = nullptr,
    .layout = pipeline_layout,
    .renderPass = render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0
  };

  configure_pipeline_geometry(pipeline_info, geometry);

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
    Assert(false, "Vulkan: failed to create graphics pipeline :(");
  }

  return pipeline;
}


struct GeometryCacheEntry {
  VkPipeline pipeline;
  std::unique_ptr<Buffer> ibo;
  std::vector<std::shared_ptr<Buffer>> vbos;
};

std::unordered_map<Geometry const*, GeometryCacheEntry> geometry_cache;

void upload_geometry(
  VkPhysicalDevice physical_device,
  VkDevice device,
  std::shared_ptr<RenderDevice> render_device,
  VkShaderModule shader_vert,
  VkShaderModule shader_frag,
  VkRenderPass render_pass,
  VkPipelineLayout pipeline_layout,
  Geometry const* geometry
) {
  auto match = geometry_cache.find(geometry);

  if (match == geometry_cache.end()) {
    auto entry = GeometryCacheEntry{};

    entry.pipeline = create_pipeline(device, shader_vert, shader_frag, render_pass, pipeline_layout, geometry);

    // TODO: write ArrayBuffer constructor (and maybe a cast operator) which accepts a std::vector
    // Also maybe support an ArrayView that doesn't allow modification to the underlying data.

    auto& index_buffer = geometry->get_index_buffer();

    entry.ibo = render_device->CreateBufferWithData(Buffer::Usage::IndexBuffer, index_buffer->view<u8>());

    for (auto& buffer : geometry->get_vertex_buffers()) {
      auto vbo = render_device->CreateBufferWithData(Buffer::Usage::VertexBuffer, buffer->view<u8>());

      entry.vbos.push_back(std::move(vbo));
    }

    geometry_cache[geometry] = std::move(entry);
  }
}

void draw_geometry(
  std::unique_ptr<CommandBuffer>& command_buffer,
  VkPipelineLayout pipeline_layout,
  VkDescriptorSet descriptor_set,
  Geometry const* geometry
) {
  auto& entry = geometry_cache[geometry];
  auto const& index_buffer = geometry->get_index_buffer();

  auto command_buffer_ = (VkCommandBuffer)command_buffer->Handle();

  command_buffer->BindGraphicsPipeline(entry.pipeline);
  command_buffer->BindIndexBuffer(entry.ibo, index_buffer->data_type());
  command_buffer->BindVertexBuffers(entry.vbos);
  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

  switch (index_buffer->data_type()) {
    case IndexDataType::UInt16:
      command_buffer->DrawIndexed(index_buffer->size() / sizeof(u16));
      break;
    case IndexDataType::UInt32:
      command_buffer->DrawIndexed(index_buffer->size() / sizeof(u32));
      break;
  }
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

#include "shader/output.vert.h"
#include "shader/output.frag.h"

u32 queue_family_graphics;
u32 queue_family_transfer;

// TODO get_instance_layers() and get_device_layers() are almost the same.

auto get_instance_layers() -> std::vector<char const*> {
  const std::vector<char const*> kDesiredInstanceLayers {
    //"VK_LAYER_KHRONOS_validation"
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
    //"VK_LAYER_KHRONOS_validation"
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

static float vertices[] = {
  -1, -1,  3,   1, 0, 0,  0, 0,
   1, -1,  3,   0, 1, 0,  1, 0,
   1,  1,  3,   0, 0, 1,  1, 1,
  -1,  1,  3,   1, 1, 0,  0, 1
};

static u16 indices[] = {
  0, 1, 2,
  2, 3, 0
};

static Geometry fullscreen_quad;

struct ScreenRenderer {
  ScreenRenderer(VkPhysicalDevice physical_device, VkDevice device)
    : physical_device(physical_device), device(device) {}

  void Initialize(std::shared_ptr<RenderDevice> render_device) {
    this->render_device = render_device;
    CreatePipelineLayoutAndBindGroup();
    CreateUniformBuffer();
    CreateSampler();
    CreateShaderModules();
  }

  void Render(
    std::unique_ptr<CommandBuffer>& command_buffer,
    std::unique_ptr<RenderTarget>& render_target,
    std::unique_ptr<RenderPass>& render_pass,
    std::shared_ptr<GPUTexture>& texture
  ) {
    bind_group->Bind(1, texture, sampler);

    render_pass->SetClearColor(0, 1, 0, 0, 1);

    command_buffer->BeginRenderPass(render_target, render_pass);

    upload_geometry(
      physical_device,
      device,
      render_device,
      (VkShaderModule)shader_vert->Handle(),
      (VkShaderModule)shader_frag->Handle(),
      ((VulkanRenderPass*)render_pass.get())->Handle(),
      (VkPipelineLayout)pipeline_layout->Handle(),
      &fullscreen_quad
    );

    draw_geometry(
      command_buffer,
      (VkPipelineLayout)pipeline_layout->Handle(),
      (VkDescriptorSet)bind_group->Handle(),
      &fullscreen_quad
    );

    command_buffer->EndRenderPass();
  }

private:
  void CreatePipelineLayoutAndBindGroup() {
    bind_group_layout = render_device->CreateBindGroupLayout({
      {
        .binding = 0,
        .type = BindGroupLayout::Entry::Type::UniformBuffer
      },
      {
        .binding = 1,
        .type = BindGroupLayout::Entry::Type::ImageWithSampler
      }
      });
    bind_group = bind_group_layout->Instantiate();

    pipeline_layout = render_device->CreatePipelineLayout({ bind_group_layout });
  }

  // TODO: get rid of this once we reworked the shader
  void CreateUniformBuffer() {
    auto transform = Matrix4::perspective_vk(45 / 180.0 * 3.141592, 1600.0 / 900, 0.01, 100.0);

    ubo = render_device->CreateBufferWithData(
      Aura::Buffer::Usage::UniformBuffer,
      &transform,
      sizeof(transform)
    );
    bind_group->Bind(0, ubo, BindGroupLayout::Entry::Type::UniformBuffer);
  }

  // TODO: specify a sampler inside the shader if possible
  void CreateSampler() {
    sampler = render_device->CreateSampler({
      .mag_filter = Sampler::FilterMode::Nearest,
      .min_filter = Sampler::FilterMode::Nearest
    });
  }

  void CreateShaderModules() {
    shader_vert = render_device->CreateShaderModule(output_vert, sizeof(output_vert));
    shader_frag = render_device->CreateShaderModule(output_frag, sizeof(output_frag));
  }

  VkPhysicalDevice physical_device;
  VkDevice device;
  std::shared_ptr<RenderDevice> render_device;

  // TODO: clean this up a bit...
  std::unique_ptr<PipelineLayout> pipeline_layout;
  std::shared_ptr<BindGroupLayout> bind_group_layout;
  std::unique_ptr<BindGroup> bind_group;
  std::unique_ptr<Buffer> ubo;
  std::unique_ptr<Sampler> sampler;
  std::unique_ptr<ShaderModule> shader_vert;
  std::unique_ptr<ShaderModule> shader_frag;
};

struct Application {
  Application(
    VkInstance instance,
    VkPhysicalDevice physical_device,
    VkDevice device,
    VkSwapchainKHR swapchain
  )   : instance(instance)
      , physical_device(physical_device)
      , device(device)
      , swapchain(swapchain)
      , screen_renderer(physical_device, device) {
  }

  void Initialize() {
    CreateRenderDevice();
    CreateSwapChainRenderTargets();
    screen_renderer.Initialize(render_device);
    renderer.Initialize(physical_device, device, render_device);
  }

  void Render(
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
    GameObject* scene,
    u32 image_id
  ) {
    renderer.Render(command_buffers, scene);

    screen_renderer.Render(
      command_buffers[1],
      render_targets[image_id],
      render_pass,
      renderer.color_texture
    );
  }

//private:
  void CreateRenderDevice() {
    render_device = CreateVulkanRenderDevice({
      .instance = instance,
      .physical_device = physical_device,
      .device = device
    });
  }

  void CreateSwapChainRenderTargets() {
    u32 swapchain_image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);

    auto swapchain_images = std::vector<VkImage>{};
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

    for (auto swapchain_image : swapchain_images) {
      auto texture = (std::shared_ptr<GPUTexture>)render_device->CreateTexture2DFromSwapchainImage(
        1600,
        900,
        GPUTexture::Format::B8G8R8A8_SRGB,
        (void*)swapchain_image
      );

      auto render_target = render_device->CreateRenderTarget({ texture });

      render_targets.push_back(std::move(render_target));
    }

    render_pass = render_targets[0]->CreateRenderPass({ RenderPass::Descriptor{
      //.load_op = RenderPass::LoadOp::DontCare,
      .layout_src = GPUTexture::Layout::Undefined,
      .layout_dst = GPUTexture::Layout::PresentSrc
    }});
  }

  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkSwapchainKHR swapchain;
  std::shared_ptr<RenderDevice> render_device;
  std::vector<std::unique_ptr<RenderTarget>> render_targets;
  std::unique_ptr<RenderPass> render_pass;

  ScreenRenderer screen_renderer;
  Renderer renderer;
};

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  // Create fullscreen quad geometry
  auto fs_quad_ibo = std::make_shared<IndexBuffer>(IndexDataType::UInt16, std::vector<u8>{(u8*)indices, (u8*)indices + sizeof(indices)});
  auto fs_quad_vbo = std::make_shared<VertexBuffer>(sizeof(float) * 8, std::vector<u8>((u8*)vertices, (u8*)vertices + sizeof(vertices)));
  fullscreen_quad.set_index_buffer(fs_quad_ibo);
  fullscreen_quad.add_vertex_buffer(fs_quad_vbo);
  fullscreen_quad.add_attribute({
    .location = 0,
    .buffer = 0,
    .data_type = VertexDataType::Float32,
    .components = 3,
    .normalized = false,
    .offset = 0
  });
  fullscreen_quad.add_attribute({
    .location = 1,
    .buffer = 0,
    .data_type = VertexDataType::Float32,
    .components = 2,
    .normalized = false,
    .offset = sizeof(float) * 6
  });

  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Aurora",
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

  auto app = Application{instance, physical_device, device, swapchain};

  app.Initialize();

  auto& render_device = app.render_device;

  auto command_pool = render_device->CreateCommandPool(
    queue_family_graphics,
    CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer
  );

  auto command_buffers = std::array<std::unique_ptr<CommandBuffer>, 2>{};
  command_buffers[0] = render_device->CreateCommandBuffer(command_pool);
  command_buffers[1] = render_device->CreateCommandBuffer(command_pool);

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

  auto event = SDL_Event{};
  auto scene = new GameObject{};
  //scene->add_child(GLTFLoader{}.parse("DamagedHelmet/DamagedHelmet.gltf"));
  scene->add_child(GLTFLoader{}.parse("Sponza/Sponza.gltf"));
  //scene->add_child(GLTFLoader{}.parse("porsche/porsche.gltf"));

  auto behemoth = GLTFLoader{}.parse("behemoth/behemoth.gltf");
  scene->add_child(behemoth);

  // test 1000 damaged helmets at once
  //auto helmet = GLTFLoader{}.parse("DamagedHelmet/DamagedHelmet.gltf")->children()[0];
  //auto& geometry = helmet->get_component<Mesh>()->geometry;
  //auto& material = helmet->get_component<Mesh>()->material;
  //for (int x = 0; x < 10; x++) {
  //  for (int y = 0; y < 10; y++) {
  //    for (int z = 0; z < 10; z++) {
  //      auto object = new GameObject{};
  //      object->add_component<Mesh>(geometry, material);
  //      object->transform().position().x() = x * 1.5;
  //      object->transform().position().y() = y * 1.5;
  //      object->transform().position().z() = z * 1.5;
  //      scene->add_child(object);
  //    }
  //  }
  //}

  auto camera = new GameObject{};
  camera->add_component<PerspectiveCamera>();
  camera->transform().position().z() = 3;
  scene->add_child(camera);
  scene->add_component<Scene>(camera);

  float x = 0;
  float y = 0;
  float z = 0;

  while (true) {
    auto state = SDL_GetKeyboardState(nullptr);
    auto const& camera_local = camera->transform().local();

    if (state[SDL_SCANCODE_W]) {
      camera->transform().position() -= camera_local[2].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_S]) {
      camera->transform().position() += camera_local[2].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_A]) {
      camera->transform().position() -= camera_local[0].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_D]) {
      camera->transform().position() += camera_local[0].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_UP])    x += 0.01;
    if (state[SDL_SCANCODE_DOWN])  x -= 0.01;
    if (state[SDL_SCANCODE_LEFT])  y += 0.01;
    if (state[SDL_SCANCODE_RIGHT]) y -= 0.01;
    if (state[SDL_SCANCODE_M])     z -= 0.01;
    if (state[SDL_SCANCODE_N])     z += 0.01;

    camera->transform().rotation().set_euler(x, y, z);

    if (state[SDL_SCANCODE_O] && behemoth != nullptr) {
      scene->remove_child(behemoth);
      delete behemoth;
      behemoth = nullptr;
    }

    u32 swapchain_image_id;

    vkResetFences(device, 1, &fence);
    vkAcquireNextImageKHR(device, swapchain, u64(-1), VK_NULL_HANDLE, fence, &swapchain_image_id);
    vkWaitForFences(device, 1, &fence, VK_TRUE, u64(-1));

    command_buffers[0]->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffers[1]->Begin(CommandBuffer::OneTimeSubmit::Yes);

    app.Render(command_buffers, scene, swapchain_image_id);

    command_buffers[0]->End();
    command_buffers[1]->End();

    VkCommandBuffer command_buffer_handles[2]{
      (VkCommandBuffer)command_buffers[0]->Handle(),
      (VkCommandBuffer)command_buffers[1]->Handle()
    };
    const auto submit_info = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 2,
      .pCommandBuffers = command_buffer_handles,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };

    vkResetFences(device, 1, &fence);
    vkQueueSubmit(queue_graphics, 1, &submit_info, fence);
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
