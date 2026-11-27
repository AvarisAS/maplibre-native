package org.maplibre.android.testapp.activity.style

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import org.maplibre.android.camera.CameraPosition
import org.maplibre.android.constants.MapLibreConstants
import org.maplibre.android.geometry.LatLng
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.Style
import org.maplibre.android.style.layers.CircleLayer
import org.maplibre.android.style.layers.FillLayer
import org.maplibre.android.style.layers.Property
import org.maplibre.android.testapp.R

/**
 * Avaris: checks the sea-surface invariants (MarineCatch docs/beslutninger/2026-11-26-vannflata-er-en-invariant.md) on Android: a sea surface at 0 m (`fill-elevation-reference: sea`) over 3D terrain near Reine, Lofoten, with sea-level circles and a fill-extrusion on land.
 */
class AvarisSeaSurfaceActivity : AppCompatActivity() {
    private lateinit var mapView: MapView

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_fill_extrusion_layer)
        mapView = findViewById(R.id.mapView)
        mapView.onCreate(savedInstanceState)

        val lat = intent.getDoubleExtra("lat", 67.932)
        val lon = intent.getDoubleExtra("lon", 13.089)
        val zoom = intent.getDoubleExtra("zoom", 12.0)
        val pitch = intent.getDoubleExtra("pitch", 60.0)
        val bearing = intent.getDoubleExtra("bearing", 200.0)
        val terrain = intent.getBooleanExtra("terrain", true)
        val sea = intent.getBooleanExtra("sea", true)
        val circles = intent.getBooleanExtra("circles", true)

        mapView.getMapAsync { map ->
            // Let the tilt gesture reach the full supported pitch over 3D terrain
            map.setMaxPitchPreference(MapLibreConstants.MAXIMUM_PITCH_LIMIT.toDouble())
            map.cameraPosition = CameraPosition.Builder()
                .target(LatLng(lat, lon))
                .zoom(zoom)
                .tilt(pitch)
                .bearing(bearing)
                .build()
            map.setStyle(Style.Builder().fromJson(styleJson(terrain, sea, circles))) { style ->
                (style.getLayer("sea-marks") as? CircleLayer)?.let { layer ->
                    check(layer.circleElevationReference.value == Property.CIRCLE_ELEVATION_REFERENCE_SEA) {
                        "sea-marks circle-elevation-reference is not sea"
                    }
                }
                (style.getLayer("sea-surface") as? FillLayer)?.let { layer ->
                    check(layer.fillElevationReference.value == Property.FILL_ELEVATION_REFERENCE_SEA) {
                        "sea-surface fill-elevation-reference is not sea"
                    }
                }
                check(
                    style.setSkyProperties(
                        mapOf(
                            "sky-color" to "#88c6fc",
                            "horizon-color" to "#ffffff",
                            "fog-color" to "#ffffff",
                            "fog-ground-blend" to 0.5f,
                            "sky-horizon-blend" to 0.8f,
                        )
                    )
                ) { "valid sky properties were rejected" }
                check(style.hasSky()) { "style has no sky after setSkyProperties" }
                check(!style.setSkyProperties(mapOf("no-such-property" to 1f))) {
                    "unknown sky property was accepted"
                }
            }
        }
    }

    override fun onStart() {
        super.onStart()
        mapView.onStart()
    }

    override fun onResume() {
        super.onResume()
        mapView.onResume()
    }

    override fun onPause() {
        super.onPause()
        mapView.onPause()
    }

    override fun onStop() {
        super.onStop()
        mapView.onStop()
    }

    public override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        mapView.onSaveInstanceState(outState)
    }

    override fun onLowMemory() {
        super.onLowMemory()
        mapView.onLowMemory()
    }

    public override fun onDestroy() {
        super.onDestroy()
        mapView.onDestroy()
    }

    companion object {
        private const val DEM_SOURCE = """
            "dem": {
              "type": "raster-dem",
              "url": "https://tiles.mapterhorn.com/tilejson.json",
              "encoding": "terrarium",
              "attribution": "<a href=\"https://mapterhorn.com\">Mapterhorn</a>"
            }
        """

        private const val SEA_SOURCE = """
            "sea": {
              "type": "geojson",
              "data": {
                "type": "Feature",
                "properties": {},
                "geometry": {
                  "type": "Polygon",
                  "coordinates": [[[12.80,67.80],[13.40,67.80],[13.40,68.05],[12.80,68.05],[12.80,67.80]]]
                }
              }
            }
        """

        private const val MARKS_SOURCE = """
            "marks": {
              "type": "geojson",
              "data": {
                "type": "FeatureCollection",
                "features": [
                  {"type": "Feature", "properties": {}, "geometry": {"type": "Point", "coordinates": [13.075,67.925]}},
                  {"type": "Feature", "properties": {}, "geometry": {"type": "Point", "coordinates": [13.10,67.94]}},
                  {"type": "Feature", "properties": {}, "geometry": {"type": "Point", "coordinates": [13.06,67.935]}}
                ]
              }
            }
        """

        private const val BLOCK_SOURCE = """
            "block": {
              "type": "geojson",
              "data": {
                "type": "Feature",
                "properties": {},
                "geometry": {
                  "type": "Polygon",
                  "coordinates": [[[13.092,67.932],[13.096,67.932],[13.096,67.934],[13.092,67.934],[13.092,67.932]]]
                }
              }
            }
        """

        private const val BACKGROUND_LAYER = """
            {"id":"background","type":"background","paint":{"background-color":"#7f9f6a"}}
        """

        private const val HILLSHADE_LAYER = """
            {"id":"hillshade","type":"hillshade","source":"dem","paint":{"hillshade-exaggeration":0.5}}
        """

        private const val SEA_SURFACE_LAYER = """
            {"id":"sea-surface","type":"fill","source":"sea","layout":{"fill-elevation-reference":"sea"},"paint":{"fill-color":"#1f7a8c","fill-opacity":0.75}}
        """

        private const val BLOCK_LAYER = """
            {"id":"block","type":"fill-extrusion","source":"block","paint":{"fill-extrusion-color":"#c0392b","fill-extrusion-height":60,"fill-extrusion-base":0}}
        """

        private const val SEA_MARKS_LAYER = """
            {"id":"sea-marks","type":"circle","source":"marks","layout":{"circle-elevation-reference":"sea"},"paint":{"circle-radius":10,"circle-color":"#ffcc00","circle-stroke-width":2,"circle-stroke-color":"#000000","circle-pitch-alignment":"map"}}
        """

        /** Builds the style JSON; the flags only add or leave out the terrain property and two layers. */
        fun styleJson(terrain: Boolean, sea: Boolean, circles: Boolean): String {
            val terrainProperty = if (terrain) {
                ""","terrain":{"source":"dem","exaggeration":1.0}"""
            } else {
                ""
            }
            val layers = buildList {
                add(BACKGROUND_LAYER.trim())
                add(HILLSHADE_LAYER.trim())
                if (sea) add(SEA_SURFACE_LAYER.trim())
                add(BLOCK_LAYER.trim())
                if (circles) add(SEA_MARKS_LAYER.trim())
            }.joinToString(",\n")
            val sources = listOf(DEM_SOURCE, SEA_SOURCE, MARKS_SOURCE, BLOCK_SOURCE)
                .joinToString(",\n") { it.trim() }
            return """
                {
                  "version": 8,
                  "glyphs": "https://demotiles.maplibre.org/font/{fontstack}/{range}.pbf",
                  "sources": {
                    $sources
                  },
                  "layers": [
                    $layers
                  ]$terrainProperty
                }
            """.trimIndent()
        }
    }
}
