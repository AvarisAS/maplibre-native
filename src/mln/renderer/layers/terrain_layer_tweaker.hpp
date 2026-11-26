#pragma once

#include <mln/util/noncopyable.hpp>
#include <mln/shaders/shader_defines.hpp>

#include <memory>
#include <string>

namespace mln {

class RenderTerrain;
class LayerGroupBase;
class PaintParameters;

namespace gfx {
class UniformBuffer;
using UniformBufferPtr = std::shared_ptr<UniformBuffer>;
} // namespace gfx

/**
 * Terrain layer specific tweaker - updates UBOs for terrain rendering
 * Note: This is NOT a LayerTweaker because terrain is not a regular layer
 */
class TerrainLayerTweaker : util::noncopyable {
public:
    explicit TerrainLayerTweaker(const RenderTerrain* terrain_)
        : terrain(terrain_) {}

    ~TerrainLayerTweaker() = default;

    /// `depthPass`: the group is the one drawn into the offscreen depth target that symbols read,
    /// which keeps the usual depth while the main pass is reversed (PaintParameters::reversedDepth).
    void execute(LayerGroupBase&, const PaintParameters&, bool depthPass = false);

protected:
#if MLN_UBO_CONSOLIDATION
    // One per group: their matrices differ when the main pass has a reversed depth.
    gfx::UniformBufferPtr drawableUniformBuffer;
    gfx::UniformBufferPtr depthDrawableUniformBuffer;
#endif

    const RenderTerrain* terrain = nullptr;
};

} // namespace mln
