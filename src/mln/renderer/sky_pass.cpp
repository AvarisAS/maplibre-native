#include <mln/renderer/sky_pass.hpp>

#include <mln/gfx/color_mode.hpp>
#include <mln/gfx/context.hpp>
#include <mln/gfx/cull_face_mode.hpp>
#include <mln/gfx/drawable_builder.hpp>
#include <mln/gfx/shader_registry.hpp>
#include <mln/map/transform_state.hpp>
#include <mln/renderer/render_pass.hpp>
#include <mln/shaders/shader_defines.hpp>
#include <mln/shaders/sky_ubo.hpp>
#include <mln/style/sky_impl.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>

namespace mln {

namespace {

constexpr std::string_view SkyShaderName = "SkyShader";

LayerGroupPtr makeFullscreenLayerGroup(gfx::Context& context,
                                       const gfx::ShaderProgramBasePtr& shader,
                                       std::string_view name,
                                       std::size_t vertexAttribute,
                                       bool is3D,
                                       gfx::DepthMaskType depthMask) {
    if (!shader) {
        return nullptr;
    }

    auto layerGroup = context.createLayerGroup(0, 1, std::string(name));
    if (!layerGroup) {
        return nullptr;
    }

    auto builder = context.createDrawableBuilder(std::string(name));
    builder->setShader(shader);
    builder->setRenderPass(RenderPass::Translucent);
    builder->setIs3D(is3D);
    builder->setDepthType(depthMask);
    builder->setEnableDepth(true);
    builder->setEnableStencil(false);
    builder->setColorMode(gfx::ColorMode::alphaBlended());
    builder->setCullFaceMode(gfx::CullFaceMode::disabled());
    builder->setVertexAttrId(vertexAttribute);
    builder->addQuad(-1, -1, 1, 1);
    builder->flush(context);

    for (auto& drawable : builder->clearDrawables()) {
        layerGroup->addDrawable(std::move(drawable));
    }
    return layerGroup;
}

} // namespace

void SkyPass::update(gfx::ShaderRegistry& shaders,
                     gfx::Context& context,
                     const TransformState& state,
                     const Size& viewportSize,
                     float pixelRatio,
                     const std::optional<EvaluatedSky>& evaluated) {
    if (!evaluated || viewportSize.isEmpty()) {
        if (skyLayerGroup) {
            skyLayerGroup->clearDrawables();
        }
        return;
    }

    // No globe on this line: the sky is always fully shown (GL JS's sky-blend is 0 in Mercator).
    constexpr double projectionTransition = 0.0;
    {
        if (!skyShader) {
            skyShader = context.getGenericShader(shaders, std::string(SkyShaderName));
        }
        if (!skyLayerGroup || skyLayerGroup->empty()) {
            skyLayerGroup = makeFullscreenLayerGroup(
                context, skyShader, "sky", shaders::idSkyPosVertexAttribute, false, gfx::DepthMaskType::ReadWrite);
        }

        if (skyLayerGroup) {
            constexpr double maxHorizonAngle = 89.25 * std::numbers::pi / 180.0;
            const double pitch = state.getPitch();
            const double horizonDistance = state.getCameraToCenterDistance() * pixelRatio *
                                           std::min(std::tan(std::numbers::pi / 2.0 - pitch) * 0.85,
                                                    std::tan(maxHorizonAngle - pitch));
            const double roll = state.getRoll();
            const std::array<float, 2> normal = {
                {static_cast<float>(-std::sin(roll)), static_cast<float>(std::cos(roll))}};
            const std::array<float, 2> horizon = {
                {static_cast<float>(viewportSize.width * 0.5 + normal[0] * horizonDistance),
                 static_cast<float>(viewportSize.height * 0.5 + normal[1] * horizonDistance)}};

            const shaders::SkyPropsUBO props = {
                .sky_color = evaluated->get<style::SkyColor>(),
                .horizon_color = evaluated->get<style::SkyHorizonColor>(),
                .horizon = horizon,
                .horizon_normal = normal,
                .viewport_size = {{static_cast<float>(viewportSize.width), static_cast<float>(viewportSize.height)}},
                .sky_horizon_blend = evaluated->get<style::SkyHorizonBlend>() * viewportSize.height * 0.5f,
                .sky_blend = static_cast<float>(projectionTransition),
            };
            skyLayerGroup->mutableUniformBuffers().createOrUpdate(shaders::idSkyPropsUBO, &props, context);
        }
    }
}

} // namespace mln
