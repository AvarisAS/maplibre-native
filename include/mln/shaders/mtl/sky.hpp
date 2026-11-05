#pragma once

#include <mln/shaders/mtl/shader_program.hpp>
#include <mln/shaders/shader_defines.hpp>
#include <mln/shaders/shader_source.hpp>

namespace mln {
namespace shaders {

constexpr auto skyShaderPrelude = R"(

enum {
    idSkyPropsUBO = drawableReservedUBOCount,
    skyUBOCount
};

struct alignas(16) SkyPropsUBO {
    float4 sky_color;
    float4 horizon_color;
    float2 horizon;
    float2 horizon_normal;
    float2 viewport_size;
    float sky_horizon_blend;
    float sky_blend;
};

)";

template <>
struct ShaderSource<BuiltIn::SkyShader, gfx::Backend::Type::Metal> {
    static constexpr auto name = "SkyShader";
    static constexpr auto vertexMainFunction = "vertexMain";
    static constexpr auto fragmentMainFunction = "fragmentMain";

    static const std::array<AttributeInfo, 1> attributes;
    static constexpr std::array<AttributeInfo, 0> instanceAttributes{};
    static const std::array<TextureInfo, 0> textures;

    static constexpr auto prelude = skyShaderPrelude;
    static constexpr auto source = R"(

struct SkyVertexStage {
    short2 position [[attribute(0)]];
};

struct SkyFragmentStage {
    float4 position [[position, invariant]];
    float2 pos;
};

SkyFragmentStage vertex vertexMain(SkyVertexStage in [[stage_in]]) {
    const float2 pos = float2(in.position);
    return {
        .position = float4(pos, 1.0, 1.0),
        .pos = pos,
    };
}

half4 fragment fragmentMain(SkyFragmentStage in [[stage_in]],
                            device const SkyPropsUBO& sky [[buffer(idSkyPropsUBO)]]) {
    const float2 pixel = (in.pos * 0.5 + 0.5) * sky.viewport_size;
    const float distanceToHorizon = dot(pixel - sky.horizon, sky.horizon_normal);
    float4 color = float4(0.0);

    if (distanceToHorizon > 0.0) {
        if (sky.sky_horizon_blend > 0.0 && distanceToHorizon < sky.sky_horizon_blend) {
            const float blend = 1.0 - distanceToHorizon / sky.sky_horizon_blend;
            color = mix(sky.sky_color, sky.horizon_color, blend * blend);
        } else {
            color = sky.sky_color;
        }
    }

    return half4(color * (1.0 - sky.sky_blend));
}
)";
};

} // namespace shaders
} // namespace mln
