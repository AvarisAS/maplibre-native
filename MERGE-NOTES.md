# Avaris fork: merge notes

Branch `avaris/main` in AvarisAS/maplibre-native is the source of the iOS build that
AvarisAS/maplibre-native-distribution ships over SPM. The goal is native MapLibre with 3D terrain
for the MarineCatch apps.

## Sources

| What | Ref | SHA |
| --- | --- | --- |
| Base | `mapriot/maplibre-native` `mapriot/terrain` | `2230ede875c8aaa9bb29eca89ce02e74b1558d48` |
| Merged | `maplibre/maplibre-native` `feature/terrain-3d` (PR #4190) | `157e1fed56d6e55a6d24f582a21d0a2ce4080666` |
| Result | `avaris/main` merge commit | `75cedcafd64d8bd04a510fe0d0f3c123e2f7f4d3` |

Why mapriot/terrain is the base: on 2026-11-05 it was 93 commits ahead of #4190 (Metal performance
fixes, used in a shipping iOS app) and only 15 behind, so we merged those 15 rather than give up the 93.

### Conflict resolutions (terrain-3d into mapriot/terrain)

1. `TERRAIN.md`: took upstream's text. The heatmap item had been settled upstream ("elevated, not
   draped, as in gl-js"); mapriot still listed it as an open question. Documentation only.
2. `test/CMakeLists.txt`: kept mapriot's `test/renderer/terrain_runtime_layer.test.cpp` entry.
   Upstream's side simply lacked that file; nothing was removed.

## Not merged: globe (PR #4533) and sky (PR #4534)

MarineCatch does not need globe: the app's zoom floor is 6.4, and globe only differs visibly from
Mercator below about z5. Sky was tried on its own.

**Globe**, head `bec8a12440d0c8a3bd320f151e1b16de39285705` (the `ios-v2026.11.2-globe` tag is
`a8e58afd6753722be718037efd189302b84ba48b`). Trial-merged into `avaris/main`: 140 conflicted files,
310 hunks. The conflicts change behaviour; they are not just overlapping text:

- Every vertex shader, on GL, Metal, Vulkan and WebGPU (plus the generated `include/mln/shaders/*`):
  terrain writes `apply_drape_transform(u_matrix * pos, …)` and globe writes `projectTile(…)`.
  Neither side composes them.
- `transform_state`, `tile_cover`, `render_orchestrator`, `paint_parameters`, symbol placement and
  collision (`placement.cpp` alone has 12 hunks).
- Neither branch renders terrain on a globe. TERRAIN.md ("Convergence with the globe PR",
  2026-09-07) records that the seam is undecided upstream ("whichever branch lands first owns that
  seam") and that unifying the tile-cover loops is deferred. Picking a side per hunk would compile
  but render wrongly with both enabled, so we did not do it.

**Sky**, head `f7b4c293349b3a823c9caae184be99c16444c8c9`. It does not contain #4533, but it carries a
merge of upstream `main` that overlaps the same code. Trial merge into `avaris/main`: 147 files,
335 hunks, with the same shader and transform overlap. Not clean, so not included.

**When to revisit:** once #4190 or #4533 lands on maplibre `main`, the other side adapts to it, and
this fork can merge `main`.

## Release

- Version: `platform/ios/VERSION` says `7.0.0`; we publish `7.0.0-avaris.N`.
- First release `7.0.0-avaris.1` was built by hand on macOS 27.2 / Xcode 27.2, using the same command
  as `.github/workflows/avaris-ios-release.yml`.
- Release builds come from that workflow. Push the tag `ios-v7.0.0-avaris.N` to trigger it. The
  workflow needs the secret `DISTRIBUTION_TOKEN` (contents:write on the distribution repo).
- The zip includes dSYMs (~180 MB), like upstream's.

## Verification

- An `swiftc`-built simulator app (outside this repo) linked against the released XCFramework renders
  `terrain` from a terrarium `raster-dem` with exaggeration 1.5, pitched over Lofoten (68.2N 14.5E),
  on an iPhone 18 Pro simulator (iOS 27.2).
- The repo's own terrain unit and render tests have not been run on this fork yet.
