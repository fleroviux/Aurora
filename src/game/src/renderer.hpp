/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <array>
#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/any_ptr.hpp>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "geometry_cache.hpp"

namespace Aura {

struct Renderer {
  void Initialize(
    VkPhysicalDevice physical_device,
    VkDevice device,
    std::shared_ptr<RenderDevice> render_device
  );

  void Render(
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
    GameObject* scene
  );

//private:
  using ProgramKey = std::pair<std::type_index, u32>;

  struct Renderable {
    GameObject* object;
    Mesh* mesh;
    float z;
  };

  void RenderObject(
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
    GameObject* object,
    Mesh* mesh
  );

  bool IsObjectInsideCameraFrustum(Matrix4 const& modelview, std::shared_ptr<Geometry> const& geometry);

  void CompileShaderProgram(AnyPtr<Material> material);

  void UploadTexture(
    VkCommandBuffer command_buffer,
    std::shared_ptr<Texture>& texture
  );

  void UploadTextureCube(
    VkCommandBuffer command_buffer,
    std::array<std::shared_ptr<Texture>, 6>& textures
  );

  void TransitionImageLayout(
    VkCommandBuffer command_buffer,
    AnyPtr<GPUTexture> texture,
    GPUTexture::Layout old_layout,
    GPUTexture::Layout new_layout,
    u32 base_mip = 0,
    u32 mip_count = 1
  );

  void CreateExampleCubeMap(VkCommandBuffer command_buffer);

  void CreateCameraUniformBlock();
  void UpdateCamera(GameObject* camera);

  void CreateRenderTarget();

  void CreateBindGroupAndPipelineLayout();

  auto CreatePipeline(
    AnyPtr<Geometry> geometry,
    std::unique_ptr<PipelineLayout>& pipeline_layout,
    std::unique_ptr<ShaderModule>& shader_vert,
    std::unique_ptr<ShaderModule>& shader_frag
  ) -> VkPipeline;

  static void BuildPipelineGeometryInfo(
    Geometry* geometry,
    std::vector<VkVertexInputBindingDescription>& bindings,
    std::vector<VkVertexInputAttributeDescription>& attributes
  );

  // TODO: make this code more sensible.
  static auto GetVkFormatFromAttribute(
    Geometry::Attribute const& attribute
  ) -> VkFormat;

  VkPhysicalDevice physical_device;
  VkDevice device;
  std::shared_ptr<RenderDevice> render_device;

  std::shared_ptr<GPUTexture> color_texture;
  std::shared_ptr<GPUTexture> depth_texture;
  std::unique_ptr<RenderTarget> render_target;
  std::unique_ptr<RenderPass> render_pass;

  std::shared_ptr<BindGroupLayout> bind_group_layout;
  std::unique_ptr<PipelineLayout> pipeline_layout;

  struct CameraData {
    UniformBlock data;
    Matrix4* projection;
    Matrix4* view;
    std::unique_ptr<Buffer> ubo;

    Frustum const* frustum;
  } camera_data;

  struct ProgramData {
    std::unique_ptr<ShaderModule> shader_vert;
    std::unique_ptr<ShaderModule> shader_frag;
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
    VkPipeline pipeline;
  };

  bool uploaded_example_cubemap = false;
  Texture* cubemap_handle = nullptr;

  std::unordered_map<Material*, std::unique_ptr<Buffer>> material_ubo;
  std::unordered_map<ProgramKey, ProgramData, pair_hash> program_cache;
  std::unordered_map<Texture*, TextureData> texture_cache;
  std::unordered_map<GameObject*, ObjectData> object_cache;

  std::unique_ptr<GeometryCache> geo_cache;
};

} // namespace Aura
