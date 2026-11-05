#pragma once

#include <mln/renderer/layer_group.hpp>
#include <mln/renderer/render_sky.hpp>
#include <mln/shaders/shader_program_base.hpp>
#include <mln/util/size.hpp>

#include <optional>

namespace mln {

class TransformState;

namespace gfx {
class Context;
class ShaderRegistry;
} // namespace gfx

/// Owns the source-independent fullscreen passes used by the root `sky` style property.
///
/// The planar sky is rendered before style layers. (The globe atmosphere of upstream #4534 is not
/// ported: this fork has no globe.)
class SkyPass {
public:
    void update(gfx::ShaderRegistry&,
                gfx::Context&,
                const TransformState&,
                const Size& viewportSize,
                float pixelRatio,
                const std::optional<EvaluatedSky>&);

    LayerGroupBase* getSkyLayerGroup() const { return skyLayerGroup.get(); }

private:
    LayerGroupPtr skyLayerGroup;
    gfx::ShaderProgramBasePtr skyShader;
};

} // namespace mln
