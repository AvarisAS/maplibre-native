#include <mln/test/util.hpp>
#include <mln/test/stub_file_source.hpp>
#include <mln/test/map_adapter.hpp>

#include <mln/gfx/headless_frontend.hpp>
#include <mln/map/camera.hpp>
#include <mln/map/map_options.hpp>
#include <mln/style/style.hpp>
#include <mln/style/layer.hpp>
#include <mln/style/layers/circle_layer.hpp>
#include <mln/style/layers/fill_layer.hpp>
#include <mln/style/layers/line_layer.hpp>
#include <mln/style/layers/symbol_layer.hpp>
#include <mln/util/image.hpp>
#include <mln/util/run_loop.hpp>
#include <mln/util/io.hpp>
#include <cstdlib>

#include <cmath>
#include <memory>
#include <string>

using namespace mln;

// Avaris: `*-elevation-reference`. "ground" (the default) drapes a fill or line on the terrain
// and lifts a circle or symbol by the DEM; "sea" draws it flat at 0 m, depth-tested against the
// terrain, so ground above sea level hides it and a seabed below it does not.

namespace {

// A flat DEM tile at `metres`, Terrain-RGB.
std::string flatDEMTile(double metres) {
    const auto v = static_cast<uint32_t>(std::lround((metres + 10000.0) * 10.0));
    PremultipliedImage image({256, 256});
    for (uint32_t i = 0; i < 256 * 256; ++i) {
        uint8_t* px = image.data.get() + i * 4;
        px[0] = static_cast<uint8_t>((v >> 16) & 0xff);
        px[1] = static_cast<uint8_t>((v >> 8) & 0xff);
        px[2] = static_cast<uint8_t>(v & 0xff);
        px[3] = 255;
    }
    return encodePNG(image);
}

// A red square over the centre, as a fill, an outline line or a circle, on `reference`.
std::string style(const std::string& kind, const std::string& reference) {
    std::string layer;
    if (kind == "fill") {
        layer = R"({"id": "x", "type": "fill", "source": "shape",
                    "layout": {"fill-elevation-reference": ")" +
                reference + R"("}, "paint": {"fill-color": "red"}})";
    } else if (kind == "line") {
        layer = R"({"id": "x", "type": "line", "source": "shape",
                    "layout": {"line-elevation-reference": ")" +
                reference + R"("}, "paint": {"line-color": "red", "line-width": 20}})";
    } else {
        layer = R"({"id": "x", "type": "circle", "source": "shape",
                    "layout": {"circle-elevation-reference": ")" +
                reference + R"("}, "paint": {"circle-color": "red", "circle-radius": 12}})";
    }
    return R"({
  "version": 8,
  "sources": {
    "dem": {"type": "raster-dem", "tiles": ["http://example.com/dem/{z}-{x}-{y}.png"],
            "encoding": "mapbox", "maxzoom": 11, "tileSize": 256},
    "shape": {"type": "geojson", "maxzoom": 16, "data": {"type": "Feature", "properties": {},
      "geometry": {"type": "Polygon", "coordinates": [[[9.99, 45.993], [10.01, 45.993], [10.01, 46.007],
                                                        [9.99, 46.007], [9.99, 45.993]]]}}}
  },
  "terrain": {"source": "dem", "exaggeration": 1.0},
  "layers": [{"id": "bg", "type": "background", "paint": {"background-color": "white"}}, )" +
           layer + R"(]
})";
}

int redPixels(const std::string& kind, const std::string& reference, double groundMetres) {
    util::RunLoop loop;
    auto fileSource = std::make_shared<StubFileSource>(ResourceOptions::Default(), ClientOptions());
    const std::string dem = flatDEMTile(groundMetres);
    fileSource->tileResponse = [&](const Resource&) {
        Response res;
        res.data = std::make_shared<std::string>(dem);
        return res;
    };
    HeadlessFrontend frontend{{256, 256}, 1};
    MapAdapter map(frontend,
                   MapObserver::nullObserver(),
                   fileSource,
                   MapOptions().withMapMode(MapMode::Static).withSize(frontend.getSize()));
    map.getStyle().loadJSON(style(kind, reference));
    map.jumpTo(CameraOptions().withCenter(LatLng{46.0, 10.0}).withZoom(13.0).withPitch(20.0));

    PremultipliedImage image;
    for (int i = 0; i < 4; ++i) {
        loop.runOnce();
        image = frontend.render(map).image;
    }
    if (const char* dir = std::getenv("SEA_LEVEL_DUMP")) {
        util::write_file(std::string(dir) + "/" + kind + "-" + reference + "-" + std::to_string(int(groundMetres)) + ".png",
                         encodePNG(image));
    }
    int red = 0;
    for (uint32_t i = 0; i < image.size.width * image.size.height; ++i) {
        const uint8_t* px = image.data.get() + i * 4;
        if (px[0] > 150 && px[1] < 90 && px[2] < 90) {
            ++red;
        }
    }
    return red;
}

} // namespace

TEST(TerrainSeaLevel, ParsesElevationReference) {
    util::RunLoop loop;
    auto fileSource = std::make_shared<StubFileSource>(ResourceOptions::Default(), ClientOptions());
    HeadlessFrontend frontend{{64, 64}, 1};
    MapAdapter map(frontend, MapObserver::nullObserver(), fileSource, MapOptions().withMapMode(MapMode::Static));
    map.getStyle().loadJSON(R"({"version": 8, "sources": {"s": {"type": "geojson", "data": {"type": "FeatureCollection", "features": []}}},
      "layers": [
        {"id": "f", "type": "fill", "source": "s", "layout": {"fill-elevation-reference": "sea"}},
        {"id": "l", "type": "line", "source": "s", "layout": {"line-elevation-reference": "sea"}},
        {"id": "c", "type": "circle", "source": "s", "layout": {"circle-elevation-reference": "sea"}},
        {"id": "y", "type": "symbol", "source": "s", "layout": {"symbol-elevation-reference": "sea"}},
        {"id": "g", "type": "fill", "source": "s"}
      ]})");
    using style::ElevationReferenceType;
    auto& s = map.getStyle();
    EXPECT_EQ(ElevationReferenceType::Sea,
              static_cast<style::FillLayer*>(s.getLayer("f"))->getFillElevationReference().asConstant());
    EXPECT_EQ(ElevationReferenceType::Sea,
              static_cast<style::LineLayer*>(s.getLayer("l"))->getLineElevationReference().asConstant());
    EXPECT_EQ(ElevationReferenceType::Sea,
              static_cast<style::CircleLayer*>(s.getLayer("c"))->getCircleElevationReference().asConstant());
    EXPECT_EQ(ElevationReferenceType::Sea,
              static_cast<style::SymbolLayer*>(s.getLayer("y"))->getSymbolElevationReference().asConstant());
    EXPECT_TRUE(static_cast<style::FillLayer*>(s.getLayer("g"))->getFillElevationReference().isUndefined());
    EXPECT_EQ(ElevationReferenceType::Ground, style::FillLayer::getDefaultFillElevationReference().asConstant());

    // Serialised back under its own name, as the platform SDKs read it.
    const auto value = s.getLayer("f")->getProperty("fill-elevation-reference");
    EXPECT_EQ("sea", *value.getValue().getString());
}

// Under ground at +300 m a sea-level shape is buried; draped on the ground it shows.
TEST(TerrainSeaLevel, LandHidesWhatIsAtSeaLevel) {
    for (const std::string kind : {"fill", "line", "circle"}) {
        const int ground = redPixels(kind, "ground", 300.0);
        const int sea = redPixels(kind, "sea", 300.0);
        EXPECT_GT(ground, 300) << kind << " on the ground shows";
        EXPECT_LT(sea, ground / 50) << kind << " at sea level under land is hidden";
    }
}

// Over a seabed at -100 m a sea-level shape stands above the ground and shows.
TEST(TerrainSeaLevel, ASeaLevelShapeShowsOverTheSeabed) {
    for (const std::string kind : {"fill", "line", "circle"}) {
        const int sea = redPixels(kind, "sea", -100.0);
        EXPECT_GT(sea, 300) << kind << " at sea level over the seabed shows";
    }
}
