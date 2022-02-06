/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/scene/game_object.hpp>
#include <unordered_map>

namespace Aura {

struct Renderer {
  void Initialize(
    VkPhysicalDevice physical_device,
    VkDevice device,
    std::shared_ptr<RenderDevice> render_device
  );

  void Render(
    std::array<VkCommandBuffer, 2>& command_buffers,
    GameObject* scene
  );

//private:
  void RenderObject(
    std::array<VkCommandBuffer, 2>& command_buffers,
    GameObject* object,
    Mesh* mesh
  );

  auto GetTexture(
    VkCommandBuffer command_buffer,
    std::shared_ptr<Texture>& texture
  ) -> std::unique_ptr<GPUTexture>&;

  void TransitionImageLayout(
    VkCommandBuffer command_buffer,
    AnyPtr<GPUTexture> texture,
    GPUTexture::Layout old_layout,
    GPUTexture::Layout new_layout
  );

  void CreateCameraUniformBlock();
  void UpdateCameraUniformBlock(GameObject* camera);

  void CreateRenderTarget();

  auto CreatePipeline(
    std::shared_ptr<Geometry>& geometry,
    std::unique_ptr<PipelineLayout>& pipeline_layout,
    std::unique_ptr<ShaderModule>& shader_vert,
    std::unique_ptr<ShaderModule>& shader_frag
  ) -> VkPipeline;

  static void BuildPipelineGeometryInfo(
    std::shared_ptr<Geometry>& geometry,
    std::vector<VkVertexInputBindingDescription>& bindings,
    std::vector<VkVertexInputAttributeDescription>& attributes
  );

  // TODO: make this code more sensible.
  static auto GetVkFormatFromAttribute(
    VertexBufferLayout::Attribute const& attribute
  ) -> VkFormat;

  VkPhysicalDevice physical_device;
  VkDevice device;
  std::shared_ptr<RenderDevice> render_device;

  std::shared_ptr<GPUTexture> color_texture;
  std::shared_ptr<GPUTexture> depth_texture;
  std::unique_ptr<RenderTarget> render_target;
  std::unique_ptr<RenderPass> render_pass;

  struct CameraData {
    UniformBlock data;
    Matrix4* projection;
    Matrix4* view;
    std::unique_ptr<Buffer> ubo;
  } camera_data;

  struct TextureData {
    bool valid = false;
    std::unique_ptr<GPUTexture> texture;
    std::unique_ptr<Sampler> sampler;
    std::unique_ptr<Buffer> buffer;
  };

  struct ObjectData {
    bool valid = false;

    // TODO: move this stuff to the appropriate places.
    std::shared_ptr<BindGroupLayout> bind_group_layout;
    std::unique_ptr<PipelineLayout> pipeline_layout;
    std::unique_ptr<BindGroup> bind_group;
    std::unique_ptr<ShaderModule> shader_vert;
    std::unique_ptr<ShaderModule> shader_frag;
    std::unique_ptr<Buffer> ubo;
    std::unique_ptr<Buffer> ibo;
    std::vector<std::unique_ptr<Buffer>> vbos;
    VkPipeline pipeline;
  };

  std::unordered_map<Texture*, TextureData> texture_cache;
  std::unordered_map<GameObject*, ObjectData> object_cache;
};

} // namespace Aura
