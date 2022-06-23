/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/render_engine.hpp>
#include <aurora/renderer/render_pipeline_base.hpp>

#include "cache/geometry_cache.hpp"
#include "forward/forward_render_pipeline.hpp"

namespace Aura {

struct RenderEngine final : RenderEngineBase {
  RenderEngine(RenderEngineOptions const& options)
      : render_device(options.render_device) {
    CreateSharedCaches();
    CreateRenderPipeline();
  }

  void Render(
    GameObject* scene,
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
  ) override {
    // TODO: verify that scene component exists and camera is non-null.
    auto camera = scene->get_component<Scene>()->camera;

    render_pipeline->Render(scene, camera, command_buffers);
  }

  auto GetOutputTexture() -> GPUTexture* override {
    return render_pipeline->GetOutputTexture();
  }

private:
  void CreateSharedCaches() {
    geometry_cache = std::make_shared<GeometryCache>(render_device);
  }

  void CreateRenderPipeline() {
    render_pipeline = std::make_unique<ForwardRenderPipeline>(render_device, geometry_cache);
  }

  std::shared_ptr<RenderDevice> render_device;
  std::shared_ptr<GeometryCache> geometry_cache;
  std::unique_ptr<RenderPipelineBase> render_pipeline;
};

auto CreateRenderEngine(
  RenderEngineOptions const& options
) -> std::unique_ptr<RenderEngineBase> {
  return std::make_unique<RenderEngine>(options);
}

} // namespace Aura
