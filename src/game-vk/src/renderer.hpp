/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/scene/game_object.hpp>
#include <unordered_map>

namespace Aura {

struct Renderer {
  void Initialize(
    VkPhysicalDevice physical_device,
    VkDevice device,
    std::shared_ptr<RenderDevice> render_device
  );

  void Render(VkCommandBuffer command_buffer, GameObject* scene);

//private:
  void RenderObject(
    VkCommandBuffer command_buffer,
    GameObject* object,
    Mesh* mesh
  );
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

  std::unordered_map<GameObject*, ObjectData> object_cache;
};

} // namespace Aura
