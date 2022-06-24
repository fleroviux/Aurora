/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/render_device.hpp>
#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/material.hpp>
#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "cache/geometry_cache.hpp"
#include "render_pipeline_base.hpp"

namespace Aura {

struct ForwardRenderPipeline final : RenderPipelineBase {
  ForwardRenderPipeline(
    std::shared_ptr<RenderDevice> render_device,
    std::shared_ptr<GeometryCache> geometry_cache
  );

  void Render(
    GameObject* scene,
    GameObject* camera,
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
  ) override;

  auto GetOutputTexture() -> GPUTexture* override;

private:
  using ProgramKey = std::pair<std::type_index, u32>;

  struct Renderable {
    GameObject* object;
    Mesh* mesh;
    float z;
  };

  void CreateCameraUniformBlock();
  void CreateRenderTarget();
  void CreateBindGroupAndPipelineLayout();
  auto CreatePipeline(
    AnyPtr<Geometry> geometry,
    AnyPtr<Material> material,
    std::shared_ptr<PipelineLayout>& pipeline_layout,
    std::shared_ptr<ShaderModule>& shader_vert,
    std::shared_ptr<ShaderModule>& shader_frag
  ) -> std::unique_ptr<GraphicsPipeline>;

  void CreateExampleCubeMap(VkCommandBuffer command_buffer);

  void RenderObject(
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
    GameObject* object,
    Mesh* mesh
  );

  bool IsObjectInsideCameraFrustum(Matrix4 const& modelview, std::shared_ptr<Geometry> const& geometry);

  void UpdateCamera(GameObject* camera);

  void CompileShaderProgram(AnyPtr<Material> material);

  void UploadTexture(
    VkCommandBuffer command_buffer,
    std::shared_ptr<Texture>& texture
  );

  void UploadTextureCube(
    VkCommandBuffer command_buffer,
    std::array<std::shared_ptr<Texture>, 6>& textures
  );

  // TODO: move the following methods to the graphics backend:

  void GenerateMipMaps(VkCommandBuffer command_buffer, AnyPtr<GPUTexture> texture);

  void TransitionImageLayout(
    VkCommandBuffer command_buffer,
    AnyPtr<GPUTexture> texture,
    GPUTexture::Layout old_layout,
    GPUTexture::Layout new_layout,
    u32 base_mip = 0,
    u32 mip_count = 1
  );

  struct CameraData {
    UniformBlock data;
    Matrix4* projection;
    Matrix4* view;
    std::unique_ptr<Buffer> ubo;

    Frustum const* frustum;
  } camera_data;

  std::shared_ptr<RenderDevice> render_device;

  // Caches
  struct ProgramData {
    std::shared_ptr<ShaderModule> shader_vert;
    std::shared_ptr<ShaderModule> shader_frag;
  };
  struct TextureData {
    std::unique_ptr<GPUTexture> texture;
    std::unique_ptr<Sampler> sampler;
    std::unique_ptr<Buffer> buffer;
  };
  struct ObjectData {
    bool valid = false;

    // TODO: move this stuff to the appropriate places.
    std::unique_ptr<BindGroup> bind_group;
    std::unique_ptr<Buffer> ubo;
    std::unique_ptr<GraphicsPipeline> pipeline;
  };
  std::shared_ptr<GeometryCache> geometry_cache;
  std::unordered_map<ProgramKey, ProgramData, pair_hash> program_cache;
  std::unordered_map<Texture*, TextureData> texture_cache;
  std::unordered_map<GameObject*, ObjectData> object_cache;
  std::unordered_map<Material*, std::unique_ptr<Buffer>> material_ubo;

  // Render target and pass
  std::shared_ptr<GPUTexture> color_texture;
  std::shared_ptr<GPUTexture> depth_texture;
  std::shared_ptr<GPUTexture> albedo_texture;
  std::shared_ptr<GPUTexture> normal_texture;
  std::unique_ptr<RenderTarget> render_target;
  std::shared_ptr<RenderPass> render_pass;

  // Bind group and pipeline layout
  std::shared_ptr<BindGroupLayout> bind_group_layout;
  std::shared_ptr<PipelineLayout> pipeline_layout;

  // Example cubemap
  bool uploaded_example_cubemap = false;
  Texture* cubemap_handle = nullptr;
};

} // namespace Aura
