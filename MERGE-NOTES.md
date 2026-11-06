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

**Sky, ported by hand (branch `avaris/sky`, 2026-11-05).** Instead of merging #4534, its sky commits
were cherry-picked onto `avaris/main` and cut down to what Mercator needs:

- Taken: the root `sky` style property and its parsing (63142ef6), the sky gradient pass and
  shaders for GL, Metal and Vulkan (69a78074, 6a1edf15, 15eff5d3), the sky unit and render
  tests (6480a034, minus `sky/atmosphere-globe`).
- Dropped: the globe atmosphere (shader, UBO, pass, `GlobeDepthPass`, projection transition),
  because this fork has no globe. `atmosphere-blend` still parses and is ignored. Also dropped:
  #4534's Metal/Vulkan layer-group depth-state changes (only the atmosphere used them; the
  terrain line has its own), and the darwin `MLNSky` API (cbdba6c2). The style JSON is enough
  for MLRN.
- Added, since #4534 has none: the sky's fog on the terrain drape, ported from maplibre-gl-js
  `terrain.vertex/fragment`. It uses `fog-color`, `horizon-color`, `fog-ground-blend` and
  `horizon-fog-blend`, mixes in linear space, and fades in from 60 to 70 degrees of pitch
  (`Sky.calculateFogBlendOpacity`). The fog depth comes from gl-js's fog matrix (near plane at
  sea level under the centre, `TransformState::getFogDepthRange`). It is **Metal only**. GL,
  Vulkan and WebGPU read a UBO prefix and draw no fog.
- Tests (macOS Metal CMake build): `mbgl-test-runner --gtest_filter='*Sky*'` passes 6/6, and the
  render tests `sky/basic` and `sky/zoom-expression` pass. No terrain+fog render test: the
  headless runner draws terrain wrongly with or without sky, which is why upstream ignores
  `terrain/*`. Fog was checked in the simulator against the dashboard instead: the dashboard's
  own style (dark sky), Lofoten 68.12N 14.25E, z11, pitch 65, bearing 200, iPhone 18 Pro / iOS 27.2,
  with the XCFramework from `bazel build --compilation_mode=opt --//:renderer=metal
  //platform/ios:MapLibre.dynamic`. It matches GL JS to the eye: the same haze darkening toward
  the horizon and the same sky band.

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

## Avaris additions after 7.0.0-avaris.1

- `-[MLNMapView elevationAtCoordinate:]` (Swift `elevation(at:)`): the exaggerated terrain height in
  metres under a coordinate, or `nil` without terrain or before a DEM tile covering it has loaded.
  The equivalent of GL JS `map.queryTerrainElevation(lngLat)`, backed by the existing core
  `Renderer::queryTerrainElevation` (`RenderTerrain::getElevationAtLatLng`, deepest loaded DEM
  tile). Main thread only, as the renderer runs there on iOS. Core test:
  `TerrainElevation.QueryReturnsTheExaggeratedHeightOfTheLoadedDEM`
  (`test/renderer/terrain_runtime_layer.test.cpp`; 74/74 `Terrain*` tests pass on macOS Metal).
- Released in `7.0.0-avaris.2` (below).

## 7.0.0-avaris.2 (2026-11-05)

- `avaris/sky` (head `58fc6531`, the hand port of sky, see above) merged into `avaris/main`
  as `6e0e085a`, on top of `elevationAtCoordinate:` (`109ddb2e`).
- Tests (macOS Metal CMake build, `build-macos-metal`): `mbgl-test-runner
  --gtest_filter='*Sky*:*Terrain*'` passes 83/83.
- Built by hand like `.1` (the repository has no `DISTRIBUTION_TOKEN` secret, so the workflow
  cannot publish): the workflow's bazel command with `--embed_label=maplibre_ios_7.0.0-avaris.2`,
  LICENSE.md added to the zip, `swift package compute-checksum`
  `dec020b674eb9f1c65fff0d6f18229f91e5ae258756290c77defe6dd1742f21f`. Release asset and
  Package.swift in AvarisAS/maplibre-native-distribution, tag `7.0.0-avaris.2`. Built from
  `6e0e085a`. As for `.1`, this repository carries no `ios-v*` tag: pushing one starts the
  workflow, which builds for hours and then cannot publish without the secret.

## 7.0.0-avaris.3 (2026-11-05)

- `avaris/far-plane-sky` (head `6ba7c0b2`) merged into `avaris/main`:
  - The far plane (and the fog's far end) comes from the lowest plane, as gl-js
    `_calculateNearFarZIfNeeded`: min of the centre's signed altitude, the lowest height of the
    DEM tile under the centre (`TransformState::setMinElevationForCurrentTile`, reported by the
    renderer via `RendererObserver::onTerrainMinElevationChanged`) and 100 m below the camera.
    The fog's near end is sea level under the centre (gl-js `_fogMatrix`).
  - `-[MLNStyle setSkyProperties:error:]` / `-removeSky`: the sky changed in place, no reload
    (gl-js `map.setSky`).
  - `platform/ios/test/MLNTerrainTests.m` imported `<Mapbox.h>`; `ios_test` builds again.
- Tests: `mbgl-test-runner --gtest_filter='*Sky*:*Terrain*'` 85/85 (macOS Metal);
  `bazel test //platform/ios/test:ios_test --test_filter=MLNStyleTests` 22/22.
- Built by hand like `.2` from `6ba7c0b2` (`--embed_label=maplibre_ios_7.0.0-avaris.3`, LICENSE.md
  in the zip), checksum `8d186490ffe0a3c596cc620390fed2d7dbc6b6569f475419ae1d07f37fd9cdae`.
  Release asset, Package.swift and tag `7.0.0-avaris.3` in AvarisAS/maplibre-native-distribution.
- Seen against GL JS: at z12 and 72° over Vestfjorden the far ground towards the horizon is
  still missing (the clear colour), where GL JS draws it. Most likely the mesh-tile cap
  (`TerrainLoadBudget::maxMeshTiles`, 48 in Quality); GL JS has none. Not changed here.

## 7.0.0-avaris.4 (2026-11-06)

- Merged into `avaris/main`:
  - `avaris/min-elevation-rebuilds-matrices` (`c1d62a2f`): a new lowest plane
    (`setMinElevationForCurrentTile`) rebuilds the transform's matrices, so the far plane
    follows at once. Fixes the white band towards the horizon.
  - `avaris/label-perspective-cutoff` (`d4ec8c44`): no label is placed under GL JS's 0.6
    perspective-ratio cutoff (no peak names far behind a low, pitched camera).
  - `avaris/terrain-horizon-cover` (head `146bc91a`): merged whole, since `146bc91a`
    reverts `bd8509ac`'s cap raise; the net change is only the `[terrain-cover]` counters
    (`render_terrain.cpp/.hpp`). The mesh-tile cap is unchanged.
- Tests (macOS Metal): `mbgl-test-runner` over `*TransformState*:*Transform*:*Terrain*:*Sky*:
  *Collision*:*Placement*:*Symbol*`, 137 pass, 1 disabled; `MainResourceLoader.SetResourceTransform`
  (matched by the filter, network) hangs locally and was left out.
  `bazel test //platform/ios/test:ios_test --test_filter=MLNStyleTests` 22/22.
- Built by hand like `.3` from `46645127` (merges up to `5a75d66d`, `--embed_label=maplibre_ios_7.0.0-avaris.4`, LICENSE.md
  in the zip), checksum `5bbfd4ef257e69525dfa3f3319aa24fb279f0610723e7b14c6c80dde75c43a5c`. Release asset, Package.swift and tag `7.0.0-avaris.4` in
  AvarisAS/maplibre-native-distribution.
