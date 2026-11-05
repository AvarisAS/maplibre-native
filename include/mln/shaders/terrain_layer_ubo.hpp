#pragma once

#include <mln/shaders/layer_ubo.hpp>
#include <mln/util/color.hpp>

namespace mln {
namespace shaders {

struct alignas(16) TerrainDrawableUBO {
    /*  0 */ std::array<float, 4 * 4> matrix;
    /* 64 */ std::array<float, 4> dem_coords; // scale, x offset, y offset into the bound DEM
                                              // tile ({1,0,0,0} unless an ancestor is bound)
    /* 80 */
};
static_assert(sizeof(TerrainDrawableUBO) == 5 * 16);

// One entry per instance of the instanced GL terrain depth pass. The whole array is bound as
// the TerrainDrawableUBO block and indexed by gl_InstanceID in terrain_depth.vertex. Kept
// separate from TerrainDrawableUBO so the shared struct (and the mtl/vulkan/webgpu layouts
// mirroring it) stay untouched. std140: mat4 then two vec4s -> 6*16 bytes.
struct alignas(16) TerrainDepthInstanceUBO {
    /*  0 */ std::array<float, 4 * 4> matrix;
    /* 64 */ std::array<float, 4> dem_coords; // scale, x offset, y offset, dem_dim (in .w)
    /* 80 */ float dem_layer;                 // sampler2DArray layer of this tile's DEM
    /* 84 */ float pad1;
    /* 88 */ float pad2;
    /* 92 */ float pad3;
    /* 96 */
};
static_assert(sizeof(TerrainDepthInstanceUBO) == 6 * 16);

struct alignas(16) TerrainTilePropsUBO {
    /*  0 */ std::array<float, 2> dem_tl;
    /*  8 */ float dem_scale;
    /* 12 */ float pad1;
    /* 16 */
};
static_assert(sizeof(TerrainTilePropsUBO) == 16);

/// Evaluated properties that do not depend on the tile
struct alignas(16) TerrainEvaluatedPropsUBO {
    /*  0 */ std::array<float, 4> unpack; // DEM unpack vector for the source's encoding
    /* 16 */ float exaggeration;
    /* 20 */ float elevation_offset;
    // Sky fog blended into the drape, as maplibre-gl-js terrain.fragment does. The near and far
    // planes give the fog depth (see TransformState::getFogDepthRange); an opacity of 0 is no fog.
    /* 24 */ float fog_near = 0.0f;
    /* 28 */ float fog_far = 1.0f;
    /* 32 */ Color fog_color = Color::white();
    /* 48 */ Color horizon_color = Color::white();
    /* 64 */ float fog_ground_blend = 1.0f;
    /* 68 */ float horizon_fog_blend = 1.0f;
    /* 72 */ float fog_ground_blend_opacity = 0.0f;
    /* 76 */ float pad1 = 0.0f;
    /* 80 */
};
static_assert(sizeof(TerrainEvaluatedPropsUBO) == 80);

} // namespace shaders
} // namespace mln
