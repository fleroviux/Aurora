
/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/gal/backend/vulkan.hpp>
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

auto get_vk_format_from_attribute(
  VertexBufferLayout::Attribute const& attribute
) -> VkFormat {
  const auto components = attribute.components;
  const bool normalized = attribute.normalized;

  switch (attribute.data_type) {
    case VertexDataType::SInt8: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R8_SNORM;
        if (components == 2) return VK_FORMAT_R8G8_SNORM;
        if (components == 3) return VK_FORMAT_R8G8B8_SNORM;
        if (components == 4) return VK_FORMAT_R8G8B8A8_SNORM;
      } else {
        if (components == 1) return VK_FORMAT_R8_SINT;
        if (components == 2) return VK_FORMAT_R8G8_SINT;
        if (components == 3) return VK_FORMAT_R8G8B8_SINT;
        if (components == 4) return VK_FORMAT_R8G8B8A8_SINT;
      } 
      break;
    }
    case VertexDataType::UInt8: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R8_UNORM;
        if (components == 2) return VK_FORMAT_R8G8_UNORM;
        if (components == 3) return VK_FORMAT_R8G8B8_UNORM;
        if (components == 4) return VK_FORMAT_R8G8B8A8_UNORM;
      } else {
        if (components == 1) return VK_FORMAT_R8_UINT;
        if (components == 2) return VK_FORMAT_R8G8_UINT;
        if (components == 3) return VK_FORMAT_R8G8B8_UINT;
        if (components == 4) return VK_FORMAT_R8G8B8A8_UINT;
      }
      break;
    }
    case VertexDataType::SInt16: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R16_SNORM;
        if (components == 2) return VK_FORMAT_R16G16_SNORM;
        if (components == 3) return VK_FORMAT_R16G16B16_SNORM;
        if (components == 4) return VK_FORMAT_R16G16B16A16_SNORM;
      } else {
        if (components == 1) return VK_FORMAT_R16_SINT;
        if (components == 2) return VK_FORMAT_R16G16_SINT;
        if (components == 3) return VK_FORMAT_R16G16B16_SINT;
        if (components == 4) return VK_FORMAT_R16G16B16A16_SINT;
      }
      break;
    }
    case VertexDataType::UInt16: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R16_UNORM;
        if (components == 2) return VK_FORMAT_R16G16_UNORM;
        if (components == 3) return VK_FORMAT_R16G16B16_UNORM;
        if (components == 4) return VK_FORMAT_R16G16B16A16_UNORM;
      } else {
        if (components == 1) return VK_FORMAT_R16_UINT;
        if (components == 2) return VK_FORMAT_R16G16_UINT;
        if (components == 3) return VK_FORMAT_R16G16B16_UINT;
        if (components == 4) return VK_FORMAT_R16G16B16A16_UINT;
      }
      break;
    }
    case VertexDataType::Float16: {
      if (components == 1) return VK_FORMAT_R16_SFLOAT;
      if (components == 2) return VK_FORMAT_R16G16_SFLOAT;
      if (components == 3) return VK_FORMAT_R16G16B16_SFLOAT;
      if (components == 4) return VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    }
    case VertexDataType::Float32: {
      if (components == 1) return VK_FORMAT_R32_SFLOAT;
      if (components == 2) return VK_FORMAT_R32G32_SFLOAT;
      if (components == 3) return VK_FORMAT_R32G32B32_SFLOAT;
      if (components == 4) return VK_FORMAT_R32G32B32A32_SFLOAT;
      break;
    }
  }

  Assert(false, "Vulkan: failed to map vertex attribute to VkFormat");
}

void configure_pipeline_geometry(
  VkGraphicsPipelineCreateInfo& pipeline,
  Geometry const* geometry
) {
  // TODO: find a way to not leak memory while also keeping the code sane.
  auto bindings = new std::vector<VkVertexInputBindingDescription>{};
  auto attributes = new std::vector<VkVertexInputAttributeDescription>{};

  for (auto& buffer : geometry->buffers) {
    auto& layout = buffer.layout();
    auto binding = u32(bindings->size());

    bindings->push_back(VkVertexInputBindingDescription{
      .binding = binding,
      .stride = u32(layout.stride),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    });

    for (auto& attribute : layout.attributes) {
      attributes->push_back(VkVertexInputAttributeDescription{
        .location = u32(attribute.index),
        .binding = binding,
        .format = get_vk_format_from_attribute(attribute),
        .offset = u32(attribute.offset)
      });
    }
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
    .blendConstants = {0, 0, 0, 0} // probably don't need to set this unless needed.
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
    // TODO: set this to make e.g. the viewport and scissor test dynamic.
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
  std::vector<std::unique_ptr<Buffer>> vbos;

  std::vector<VkBuffer> vk_vbos;
  std::vector<VkDeviceSize> vk_vbo_offs;
};

std::unordered_map<Geometry const*, GeometryCacheEntry> geometry_cache;

void upload_geometry(
  VkPhysicalDevice physical_device,
  VkDevice device,
  std::unique_ptr<RenderDevice>& render_device,
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

    auto& index_buffer = geometry->index_buffer;

    entry.ibo = render_device->CreateBufferWithData(Buffer::Usage::IndexBuffer, index_buffer.view<u8>());

    for (auto& buffer : geometry->buffers) {
      auto vbo = render_device->CreateBufferWithData(Buffer::Usage::VertexBuffer, buffer.view<u8>());

      entry.vk_vbos.push_back((VkBuffer)vbo->Handle());
      entry.vbos.push_back(std::move(vbo));
      entry.vk_vbo_offs.push_back(0);
    }

    geometry_cache[geometry] = std::move(entry);
  }
}

void draw_geometry(
  VkCommandBuffer command_buffer,
  Geometry const* geometry
) {
  auto const& entry = geometry_cache[geometry];
  auto const& index_buffer = geometry->index_buffer;

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entry.pipeline);
  vkCmdBindVertexBuffers(command_buffer, 0, entry.vk_vbos.size(), entry.vk_vbos.data(), entry.vk_vbo_offs.data());

  switch (index_buffer.data_type()) {
    case IndexDataType::UInt16:
      vkCmdBindIndexBuffer(command_buffer, (VkBuffer)entry.ibo->Handle(), 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed(command_buffer, index_buffer.size()/sizeof(u16), 1, 0, 0, 0);
      break;
    case IndexDataType::UInt32:
      vkCmdBindIndexBuffer(command_buffer, (VkBuffer)entry.ibo->Handle(), 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, index_buffer.size()/sizeof(u32), 1, 0, 0, 0);
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

const auto triangle = Geometry{
  IndexBuffer{
    IndexDataType::UInt16,
    std::vector<u8>{
      (u8*)indices,
      (u8*)indices + sizeof(indices)
    }
  },
  std::vector<VertexBuffer>{VertexBuffer{
    VertexBufferLayout{
      .stride = sizeof(float) * 6,
      .attributes = std::vector<VertexBufferLayout::Attribute>{
        {
          .index = 0,
          .data_type = VertexDataType::Float32,
          .components = 3,
          .normalized = false,
          .offset = 0
        },
        {
          .index = 1,
          .data_type = VertexDataType::Float32,
          .components = 3,
          .normalized = false,
          .offset = sizeof(float) * 3
        }
      }
    },
    std::vector<u8>{
      (u8*)vertices,
      (u8*)vertices + sizeof(vertices)
    }
  }}
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

  auto render_device = CreateVulkanRenderDevice({
    .instance = instance,
    .physical_device = physical_device,
    .device = device
    });

  VkRenderPass render_pass;

  {
    auto attachments = std::vector<VkAttachmentDescription>{
      // Color Attachment
      {
        .flags = 0,
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      },
      // Depth Attachment
      {
        .flags = 0,
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      }
    };

    auto refs = std::vector<VkAttachmentReference>{
      // Color Attachment reference
      {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      },
      // Depth Attachment reference
      {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      }
    };

    auto render_pass_subpass = VkSubpassDescription{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &refs[0],
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = &refs[1],
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr
    };

    auto render_pass_info = VkRenderPassCreateInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = (u32)attachments.size(),
      .pAttachments = attachments.data(),
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

  auto textures = std::vector<std::unique_ptr<GPUTexture>>{};
  auto render_targets = std::vector<std::unique_ptr<RenderTarget>>{};

  auto depth_texture = render_device->CreateTexture2D(
    1600, 900, GPUTexture::Format::DEPTH_F32, GPUTexture::Usage::DepthStencilAttachment);

  {
    u32 swapchain_image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);

    auto swapchain_images = std::vector<VkImage>{};
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

    for (auto swapchain_image : swapchain_images) {
      auto texture = render_device->CreateTexture2DFromSwapchainImage(
        1600,
        900,
        GPUTexture::Format::B8G8R8B8_SRGB,
        (void*)swapchain_image
      );

      auto render_target = render_device->CreateRenderTarget({ texture.get() }, depth_texture.get());

      render_targets.push_back(std::move(render_target));
      textures.push_back(std::move(texture)); // keep texture alive
    }
  }

  auto descriptor_set_layout = VkDescriptorSetLayout{};

  // Create descriptor set layout
  {
    auto descriptor_set_layout_binding = VkDescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1, // number of "locations" in the binding?
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr
    };

    auto descriptor_set_layout_info = VkDescriptorSetLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = &descriptor_set_layout_binding
    };

    if (vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to create descriptor set layout");
    }
  }

  auto pipeline_layout = VkPipelineLayout{};

  // Create pipeline layout info
  {
    auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr
    };

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to create pipeline layout :(");
    }
  }

  auto descriptor_set = VkDescriptorSet{};

  // Create descriptor pool and descriptor set
  {
    // TODO: how do people typically handle descriptor pools in their engines?
    auto descriptor_pool = VkDescriptorPool{};
    auto descriptor_pool_sizes = std::vector<VkDescriptorPoolSize>{
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1024
      }
    };
    auto descriptor_pool_info = VkDescriptorPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .maxSets = 1024,
      .poolSizeCount = (u32)descriptor_pool_sizes.size(),
      .pPoolSizes = descriptor_pool_sizes.data()
    };

    if (vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to create descriptor pool");
    }

    auto descriptor_set_info = VkDescriptorSetAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptor_set_layout
    };

    if (vkAllocateDescriptorSets(device, &descriptor_set_info, &descriptor_set) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to allocate descriptor set");
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

  auto shader_vert_ = render_device->CreateShaderModule(triangle_vert, sizeof(triangle_vert));
  auto shader_frag_ = render_device->CreateShaderModule(triangle_frag, sizeof(triangle_frag));
  auto shader_vert = (VkShaderModule)shader_vert_->Handle();
  auto shader_frag = (VkShaderModule)shader_frag_->Handle();

  auto event = SDL_Event{};

  while (true) {
    u32 swapchain_image_id;

    // TODO: wait for this? why would we need to? Vsync?
    vkResetFences(device, 1, &fence);
    vkAcquireNextImageKHR(device, swapchain, u64(-1), VK_NULL_HANDLE, fence, &swapchain_image_id);
    vkWaitForFences(device, 1, &fence, VK_TRUE, u64(-1));

    auto& render_target = render_targets[swapchain_image_id];

    const auto clear_values = std::vector<VkClearValue>{
      {
        .color = VkClearColorValue{
          .float32 = { 0.01, 0.01, 0.01, 1 }
        }
      },
      {
        .depthStencil = VkClearDepthStencilValue{
          .depth = 1
        }
      }
    };

    auto render_pass_begin_info = VkRenderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = render_pass,
      .framebuffer = (VkFramebuffer)render_target->handle(),
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
      .clearValueCount = (u32)clear_values.size(),
      .pClearValues = clear_values.data()
    };

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    {
      // TODO: is it a good idea to upload the geometry while recording the command buffer?
      upload_geometry(physical_device, device, render_device, shader_vert, shader_frag, render_pass, pipeline_layout, &triangle);
      draw_geometry(command_buffer, &triangle);
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
