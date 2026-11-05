#include <mln/test/util.hpp>
#include <mln/test/stub_file_source.hpp>
#include <mln/test/map_adapter.hpp>

#include <mln/gfx/headless_frontend.hpp>
#include <mln/map/camera.hpp>
#include <mln/map/map_options.hpp>
#include <mln/map/mode.hpp>
#include <mln/renderer/renderer.hpp>
#include <mln/style/layers/line_layer.hpp>
#include <mln/style/sources/geojson_source.hpp>
#include <mln/style/style.hpp>
#include <mln/util/image.hpp>
#include <mln/util/run_loop.hpp>

#include <mapbox/geojson.hpp>

#include <memory>
#include <string>

using namespace mln;

namespace {

// A red line running north-south a little west of the map center, over flat terrain at sea level,
// seen straight down at zoom 16.
constexpr const char* lineGeoJSON =
    R"({"type": "Feature", "properties": {}, "geometry": {"type": "LineString", "coordinates": [[9.9995, 45.99], [9.9995, 46.01]]}})";

std::string terrainStyle(bool withLine) {
    std::string style = R"({"version": 8, "sources": {
      "dem": {"type": "raster-dem", "tiles": ["http://example.com/dem/{z}-{x}-{y}.png"], "encoding": "mapbox",
              "maxzoom": 11, "tileSize": 256})";
    if (withLine) style += std::string(R"(, "line": {"type": "geojson", "data": )") + lineGeoJSON + "}";
    style += R"(}, "terrain": {"source": "dem", "exaggeration": 1.0},
      "layers": [{"id": "bg", "type": "background", "paint": {"background-color": "white"}})";
    if (withLine) {
        style +=
            R"(, {"id": "line", "type": "line", "source": "line", "paint": {"line-color": "red", "line-width": 16}})";
    }
    return style + "]}";
}

// A flat DEM tile in Terrain-RGB: height = (R * 65536 + G * 256 + B) * 0.1 - 10000
std::string flatDEMTile(uint8_t r, uint8_t g, uint8_t b) {
    PremultipliedImage image({256, 256});
    for (uint32_t i = 0; i < 256 * 256; ++i) {
        uint8_t* px = image.data.get() + i * 4;
        px[0] = r;
        px[1] = g;
        px[2] = b;
        px[3] = 255;
    }
    return encodePNG(image);
}

// Sea level in Terrain-RGB: (0 + 10000) / 0.1 = 100000 = 1 * 65536 + 134 * 256 + 160
std::string seaLevelDEMTile() {
    return flatDEMTile(1, 134, 160);
}

// Red pixels in the band where the line is (about 47 px west of the center, 16 px wide).
int redInLineBand(const PremultipliedImage& image) {
    int red = 0;
    for (uint32_t y = 16; y < 240; ++y) {
        for (uint32_t x = 70; x < 92; ++x) {
            const uint8_t* px = image.data.get() + (y * image.size.width + x) * 4;
            red += px[0] > 150 && px[1] < 90 && px[2] < 90 ? 1 : 0;
        }
    }
    return red;
}

class TerrainStyleChangeTest {
public:
    explicit TerrainStyleChangeTest(TerrainLoadMode loadMode)
        : fileSource(std::make_shared<StubFileSource>(ResourceOptions::Default(), ClientOptions())),
          dem(seaLevelDEMTile()),
          frontend({256, 256}, 1),
          map(frontend,
              MapObserver::nullObserver(),
              fileSource,
              MapOptions().withMapMode(MapMode::Static).withSize(frontend.getSize())) {
        fileSource->tileResponse = [this](const Resource&) {
            Response res;
            res.data = std::make_shared<std::string>(dem);
            return res;
        };
        map.setTerrainLoadMode(loadMode);
        map.jumpTo(CameraOptions().withCenter(LatLng{46.0, 10.0}).withZoom(16.0).withPitch(0.0));
    }

    int renderRedInLineBand() {
        PremultipliedImage image;
        for (int i = 0; i < 4; ++i) {
            loop.runOnce();
            image = frontend.render(map).image;
        }
        return redInLineBand(image);
    }

    void addLine() {
        auto source = std::make_unique<style::GeoJSONSource>("line");
        source->setGeoJSON(mapbox::geojson::parse(lineGeoJSON));
        map.getStyle().addSource(std::move(source));
        auto layer = std::make_unique<style::LineLayer>("line", "line");
        layer->setLineColor(Color::red());
        layer->setLineWidth(16.0f);
        map.getStyle().addLayer(std::move(layer));
    }

    util::RunLoop loop;
    std::shared_ptr<StubFileSource> fileSource;
    std::string dem;
    HeadlessFrontend frontend;
    MapAdapter map;
};

} // namespace

// An app adds its route once the map is up. The drapes rendered before it must take it in: the
// fork used to drop every drape target when a layer or source was added, and the route then never
// reached the terrain.
TEST(TerrainDrape, ALineAddedAfterTheFirstFrameIsDraped) {
    for (const auto loadMode : {TerrainLoadMode::Quality, TerrainLoadMode::Balanced}) {
        int expected = 0;
        {
            TerrainStyleChangeTest fromTheStart(loadMode);
            fromTheStart.map.getStyle().loadJSON(terrainStyle(true));
            expected = fromTheStart.renderRedInLineBand();
        }
        ASSERT_GT(expected, 1000) << "the line drapes when the style has it from the start";

        TerrainStyleChangeTest addedLater(loadMode);
        addedLater.map.getStyle().loadJSON(terrainStyle(false));
        EXPECT_EQ(0, addedLater.renderRedInLineBand());
        addedLater.addLine();
        const int red = addedLater.renderRedInLineBand();
        EXPECT_GT(red, expected * 9 / 10) << (loadMode == TerrainLoadMode::Quality ? "Quality" : "Balanced") << ": "
                                          << red << " of " << expected << " red pixels";
    }
}

// A new style over terrain replaces what the drapes showed.
TEST(TerrainDrape, AStyleSwitchReplacesTheDrape) {
    TerrainStyleChangeTest test(TerrainLoadMode::Balanced);
    test.map.getStyle().loadJSON(terrainStyle(false));
    EXPECT_EQ(0, test.renderRedInLineBand());
    test.map.getStyle().loadJSON(terrainStyle(true));
    EXPECT_GT(test.renderRedInLineBand(), 1000);
}

// The engine side of GL JS map.queryTerrainElevation (MLNMapView elevationAtCoordinate:): the
// exaggerated height of the loaded DEM under a coordinate, and nothing without terrain.
TEST(TerrainElevation, QueryReturnsTheExaggeratedHeightOfTheLoadedDEM) {
    TerrainStyleChangeTest test(TerrainLoadMode::Quality);
    // 100 m: (100 + 10000) / 0.1 = 101000 = 1 * 65536 + 138 * 256 + 136
    test.dem = flatDEMTile(1, 138, 136);
    std::string style = terrainStyle(false);
    const std::string from = R"("exaggeration": 1.0)";
    style.replace(style.find(from), from.size(), R"("exaggeration": 1.5)");
    test.map.getStyle().loadJSON(style);
    test.renderRedInLineBand();

    Renderer* renderer = test.frontend.getRenderer();
    ASSERT_NE(nullptr, renderer);
    const auto elevation = renderer->queryTerrainElevation(LatLng{46.0, 10.0});
    ASSERT_TRUE(elevation.has_value());
    EXPECT_NEAR(150.0, *elevation, 0.5);

    // Far outside the loaded DEM tiles: not loaded, so no answer rather than sea level
    EXPECT_FALSE(renderer->queryTerrainElevation(LatLng{-40.0, -120.0}).has_value());

    // Terrain off: no answer
    test.map.getStyle().loadJSON(R"({"version": 8, "sources": {}, "layers": []})");
    test.renderRedInLineBand();
    EXPECT_FALSE(test.frontend.getRenderer()->queryTerrainElevation(LatLng{46.0, 10.0}).has_value());
}
