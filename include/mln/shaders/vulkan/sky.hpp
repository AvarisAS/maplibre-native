#pragma once

#include <mln/shaders/shader_source.hpp>
#include <mln/shaders/vulkan/shader_program.hpp>

namespace mln {
namespace shaders {

constexpr auto skyShaderPrelude = R"(

#define idSkyPropsUBO layerUBOStartId

)";

template <>
struct ShaderSource<BuiltIn::SkyShader, gfx::Backend::Type::Vulkan> {
    static constexpr const char* name = "SkyShader";
    static const std::array<AttributeInfo, 1> attributes;
    static constexpr std::array<AttributeInfo, 0> instanceAttributes{};
    static const std::array<TextureInfo, 0> textures;
    static constexpr auto prelude = skyShaderPrelude;

    static constexpr auto vertex = R"(
layout(location = 0) in ivec2 in_position;
layout(location = 0) out vec2 frag_pos;

void main() {
    frag_pos = vec2(in_position);
    gl_Position = vec4(frag_pos, 1.0, 1.0);
    applySurfaceTransform();
}
)";

    static constexpr auto fragment = R"(
layout(location = 0) in vec2 frag_pos;
layout(location = 0) out vec4 out_color;

layout(set = LAYER_SET_INDEX, binding = idSkyPropsUBO) uniform SkyPropsUBO {
    vec4 sky_color;
    vec4 horizon_color;
    vec2 horizon;
    vec2 horizon_normal;
    vec2 viewport_size;
    float sky_horizon_blend;
    float sky_blend;
} sky;

void main() {
    vec2 pixel = (frag_pos * 0.5 + 0.5) * sky.viewport_size;
    float distance_to_horizon = dot(pixel - sky.horizon, sky.horizon_normal);
    vec4 color = vec4(0.0);
    if (distance_to_horizon > 0.0) {
        if (sky.sky_horizon_blend > 0.0 && distance_to_horizon < sky.sky_horizon_blend) {
            float blend = 1.0 - distance_to_horizon / sky.sky_horizon_blend;
            color = mix(sky.sky_color, sky.horizon_color, blend * blend);
        } else {
            color = sky.sky_color;
        }
    }
    out_color = color * (1.0 - sky.sky_blend);
}
)";
};

} // namespace shaders
} // namespace mln
