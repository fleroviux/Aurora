
// Copyright (C) 2022 fleroviux. All rights reserved.

#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/render_engine.hpp>

#include "cache/geometry_cache.hpp"
#include "effect/ssr/ssr_effect.hpp"
#include "forward/forward_render_pipeline.hpp"
#include "render_pipeline_base.hpp"

namespace Aura {

struct RenderEngine final : RenderEngineBase {
  RenderEngine(RenderEngineOptions const& options)
      : render_device(options.render_device) {
    CreateSharedCaches();
    CreateRenderPipeline();
    CreateRenderTarget();
    CreatePostEffects();
  }

  void Render(
    GameObject* scene,
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
  ) override {
    // TODO: verify that scene component exists and camera is non-null.
    auto camera = scene->get_component<Scene>()->camera;

    render_pipeline->Render(scene, camera, command_buffers);

    auto color_texture = render_pipeline->GetColorTexture();
    auto depth_texture = render_pipeline->GetDepthTexture();
    auto normal_texture = render_pipeline->GetNormalTexture();

    ssr_effect->Render(camera, command_buffers[1], render_texture, render_target, color_texture, depth_texture, normal_texture);
  }

  auto GetOutputTexture() -> GPUTexture* override {
    // TODO: return our render texture
    //return render_pipeline->GetOutputTexture();
    return render_texture.get();
  }

private:
  void CreateSharedCaches() {
    geometry_cache = std::make_shared<GeometryCache>(render_device);
  }

  void CreateRenderPipeline() {
    render_pipeline = std::make_unique<ForwardRenderPipeline>(render_device, geometry_cache);
  }

  void CreateRenderTarget() {
    render_texture = render_device->CreateTexture2D(
      3200, 1800, GPUTexture::Format::B8G8R8A8_SRGB, GPUTexture::Usage::ColorAttachment | GPUTexture::Usage::Sampled);

    render_target = render_device->CreateRenderTarget({render_texture});
  }

  void CreatePostEffects() {
    ssr_effect = std::make_unique<SSREffect>(render_device);
  }

  std::shared_ptr<RenderDevice> render_device;
  std::shared_ptr<GeometryCache> geometry_cache;
  std::unique_ptr<RenderPipelineBase> render_pipeline;

  std::shared_ptr<GPUTexture> render_texture;
  std::unique_ptr<RenderTarget> render_target;

  std::unique_ptr<SSREffect> ssr_effect;
};

auto CreateRenderEngine(
  RenderEngineOptions const& options
) -> std::unique_ptr<RenderEngineBase> {
  return std::make_unique<RenderEngine>(options);
}

} // namespace Aura
