
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
    std::shared_ptr<RenderPass>& render_pass, 
    std::shared_ptr<GPUTexture>& texture
  ) {
    bind_group->Bind(1, texture, sampler);

    render_pass->SetClearColor(0, 1, 0, 0, 1);

    if (pipeline == VK_NULL_HANDLE) {
      CreateGraphicsPipeline(render_pass);
    }

    command_buffer->BeginRenderPass(render_target, render_pass);
    command_buffer->BindGraphicsPipeline(pipeline);
    command_buffer->BindGraphicsBindGroup(0, pipeline_layout, bind_group);
    command_buffer->Draw(3);
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

  void CreateUniformBuffer() {
    // TODO: get rid of this once we reworked the shader
    auto transform = Matrix4::perspective_vk(45 / 180.0 * 3.141592, 1600.0 / 900, 0.01, 100.0);

    ubo = render_device->CreateBufferWithData(
      Aura::Buffer::Usage::UniformBuffer,
      &transform,
      sizeof(transform)
    );
    bind_group->Bind(0, ubo, BindGroupLayout::Entry::Type::UniformBuffer);
  }

  void CreateSampler() {
    // TODO: specify a sampler inside the shader if possible
    sampler = render_device->CreateSampler({
      .mag_filter = Sampler::FilterMode::Linear,
      .min_filter = Sampler::FilterMode::Linear
    });
  }

  void CreateShaderModules() {
    shader_vert = render_device->CreateShaderModule(output_vert, sizeof(output_vert));
    shader_frag = render_device->CreateShaderModule(output_frag, sizeof(output_frag));
  }

  void CreateGraphicsPipeline(std::shared_ptr<RenderPass>& render_pass) {
    auto pipeline_builder = render_device->CreateGraphicsPipelineBuilder();

    pipeline_builder->SetViewport(0, 0, 1600, 900);
    pipeline_builder->SetShaderModule(PipelineStage::VertexShader, shader_vert);
    pipeline_builder->SetShaderModule(PipelineStage::FragmentShader, shader_frag);
    pipeline_builder->SetPipelineLayout(pipeline_layout);
    pipeline_builder->SetRenderPass(render_pass);

    pipeline = pipeline_builder->Build();
  }

  VkPhysicalDevice physical_device;
  VkDevice device;
  std::shared_ptr<RenderDevice> render_device;

  std::shared_ptr<PipelineLayout> pipeline_layout;
  std::shared_ptr<BindGroupLayout> bind_group_layout;
  std::unique_ptr<BindGroup> bind_group;
  std::unique_ptr<Buffer> ubo;
  std::unique_ptr<Sampler> sampler;
  std::shared_ptr<ShaderModule> shader_vert;
  std::shared_ptr<ShaderModule> shader_frag;
  std::unique_ptr<GraphicsPipeline> pipeline;
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

  void Initialize1() {
    CreateRenderDevice();
  }

  void Initialize2() {
    CreateSwapChainRenderTargets();
    screen_renderer.Initialize(render_device);
    renderer = std::make_unique<Renderer>(render_device);
  }

  void Render(
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
    GameObject* scene,
    u32 image_id
  ) {
    renderer->Render(command_buffers, scene);

    screen_renderer.Render(
      command_buffers[1],
      render_targets[image_id],
      render_pass,
      renderer->color_texture
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
  std::shared_ptr<RenderPass> render_pass;

  ScreenRenderer screen_renderer;
  std::unique_ptr<Renderer> renderer;
};

struct GlassMaterial final : Material {
  GlassMaterial() : Material(std::vector<std::string>{}) {
    auto layout = UniformBlockLayout{};
    layout.add<Matrix4>("model");
    uniforms = UniformBlock{layout};

    blend_state.enable = true;
    blend_state.src_color_factor = BlendFactor::One;
    blend_state.src_alpha_factor = BlendFactor::Zero;
    blend_state.dst_color_factor = BlendFactor::Src1Color;
    blend_state.dst_alpha_factor = BlendFactor::One;

    side() = Side::Both;
  }

  auto get_vert_shader() -> char const* override {
    return R"(
  #version 450

  layout (location = 0) in vec3 a_position;
  layout (location = 1) in vec3 a_normal;

  layout (binding = 0, std140) uniform Camera {
    mat4 u_projection;
    mat4 u_view;
  };

  layout (binding = 1, std140) uniform Material {
    mat4 u_model;
  };

  layout(location = 0) out vec3 v_world_position;
  layout(location = 1) out vec3 v_world_normal;
  layout(location = 2) out vec3 v_view_position;

  void main() {
    vec4 world_position = u_model * vec4(a_position, 1.0);
    vec4 view_position = u_view * world_position;

    // TODO: handle non-uniform scale in normal matrix.
    v_world_position = world_position.xyz;
    v_view_position = view_position.xyz;
    v_world_normal = normalize((u_model * vec4(a_normal, 0.0)).xyz);

    gl_Position = u_projection * view_position;
  }
)";
  }

  auto get_frag_shader() -> char const* override {
    return R"(
  #version 450

  layout (location = 0, index = 0) out vec4 frag_color_add;
  layout (location = 0, index = 1) out vec4 frag_color_multiply;

  layout(location = 0) in vec3 v_world_position;
  layout(location = 1) in vec3 v_world_normal;
  layout(location = 2) in vec3 v_view_position;

  layout (binding = 0, std140) uniform Camera {
    mat4 u_projection;
    mat4 u_view;
  };

  layout (binding = 34) uniform samplerCube u_env_map;

  float FresnelSchlick(float f0, float n_dot_v) {
    return f0 + (1.0 - f0) * pow(1.0 - n_dot_v, 5.0);
  }

  void main() {
    vec3 view_dir = -normalize(v_view_position);
    view_dir = (vec4(view_dir, 0.0) * u_view).xyz;

    vec3 world_normal = v_world_normal * (gl_FrontFacing ? 1.0 : -1.0);

    vec3 reflect_dir = reflect(-view_dir, world_normal);
    float n_dot_v = dot(view_dir, world_normal);
    vec4 reflection = texture(u_env_map, reflect_dir) * FresnelSchlick(0.0425, n_dot_v);

    frag_color_add = reflection;
    frag_color_multiply = vec4(0.25, 0.5, 0.5, 1.0);
  }
)";
  }

  auto get_uniforms() -> UniformBlock & override {
    return uniforms;
  }

  auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> override {
    return ArrayView<std::shared_ptr<Texture>>{nullptr, 0};
  }
private:
  UniformBlock uniforms;
};

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

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

  app.Initialize1();

  auto& render_device = app.render_device;

  auto command_pool = render_device->CreateCommandPool(
    queue_family_graphics,
    CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer
  );

  auto command_buffers = std::array<std::unique_ptr<CommandBuffer>, 2>{};
  command_buffers[0] = render_device->CreateCommandBuffer(command_pool);
  command_buffers[1] = render_device->CreateCommandBuffer(command_pool);
  render_device->SetTransferCommandBuffer(command_buffers[0].get());

  // TODO: remove this atrocious hack.
  command_buffers[0]->Begin(CommandBuffer::OneTimeSubmit::Yes);
  app.Initialize2();

  // TODO: move this closer to the device creation logic?
  auto queue_graphics = VkQueue{};
  vkGetDeviceQueue(device, queue_family_graphics, 0, &queue_graphics);

  auto fence = render_device->CreateFence();

  auto event = SDL_Event{};
  auto scene = new GameObject{};
  //auto helmet = GLTFLoader{}.parse("DamagedHelmet/DamagedHelmet.gltf");
  //scene->add_child(helmet);
  scene->add_child(GLTFLoader{}.parse("Sponza/Sponza.gltf"));

  //auto plane = GLTFLoader{}.parse("plane/plane.gltf");
  //plane->children()[0]->get_component<Mesh>()->material = std::make_shared<GlassMaterial>();
  //plane->transform().rotation().set_euler(M_PI * 0.5, 0.0, M_PI * 0.5);
  //plane->transform().position() = Vector3{-4, 1, 0};
  //scene->add_child(plane);

  auto behemoth = GLTFLoader{}.parse("behemoth/behemoth.gltf");
  scene->add_child(behemoth);

  //auto vegetation = GLTFLoader{}.parse("pachira_aquatica_01_4k.gltf/pachira_aquatica_01_4k.gltf");
  //scene->add_child(vegetation);

  //// test 1000 damaged helmets at once
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

    /*if (state[SDL_SCANCODE_O] && behemoth != nullptr) {
      scene->remove_child(behemoth);
      delete behemoth;
      behemoth = nullptr;
    }*/

    u32 swapchain_image_id;

    fence->Reset();
    vkAcquireNextImageKHR(device, swapchain, ~0ULL, VK_NULL_HANDLE, (VkFence)fence->Handle(), &swapchain_image_id);
    fence->Wait();

    //command_buffers[0]->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffers[1]->Begin(CommandBuffer::OneTimeSubmit::Yes);

    app.Render(command_buffers, scene, swapchain_image_id);

    command_buffers[0]->End();
    command_buffers[1]->End();

    std::array<CommandBuffer*, 2> command_bufs{command_buffers[0].get(), command_buffers[1].get()};

    fence->Reset();
    render_device->GraphicsQueue()->Submit(command_bufs, fence);
    fence->Wait();

    command_buffers[0]->Begin(CommandBuffer::OneTimeSubmit::Yes);

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
