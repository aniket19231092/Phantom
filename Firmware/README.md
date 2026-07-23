# Bike Computer Firmware

Starting firmware for the ESP32-S3-based bike computer: GPX route browsing,
map + route overlay display, ride recording (speed/avg speed/distance), and
rotary encoder navigation.

## Required libraries (Arduino Library Manager)

- **TinyGPSPlus** (Mikal Hart) - NMEA parsing
- **Adafruit GFX Library** - text/shape drawing primitives, used by the
  Sharp Memory LCD driver

## Board settings (Arduino IDE)

- Board: `ESP32S3 Dev Module` (or your custom board definition once made)
- USB Mode: **Hardware CDC and JTAG** (matches native USB D+/D- wiring)
- PSRAM: **Disabled** (this targets the `-N8` module variant, no PSRAM)
- Flash Size: 8MB

## SD card folder layout

```
/routes/
    morning_loop.gpx
    river_trail.gpx
    ...
/tiles/
    16/
        11241/
            26348.bin
            26349.bin
            ...
        11242/
            ...
    17/
        ...
```

## Map tile format (what your PC-side prep script must produce)

Each tile file is a **raw 1-bit-per-pixel monochrome bitmap**, no header:

- `TILE_SIZE x TILE_SIZE` pixels (currently 200x200, see `Config.h`)
- Row-major, MSB-first bit packing (bit 7 of byte 0 = pixel (0,0), etc.)
- File size must be exactly `(TILE_SIZE * TILE_SIZE) / 8` bytes = 5000 bytes
- 1 = white, 0 = black (matches the Sharp Memory LCD's native pixel format)
- Path: `/tiles/{zoom}/{tileX}/{tileY}.bin`
- Tile numbering follows the standard Web Mercator "slippy map" scheme
  (same as OpenStreetMap tile servers), just with a 200px tile size instead
  of the usual 256px. Standard formulas:

  ```
  n = 2^zoom
  tileX = floor((lon + 180) / 360 * n)
  tileY = floor((1 - ln(tan(lat_rad) + 1/cos(lat_rad)) / pi) / 2 * n)
  ```

  A PC-side script (not included here) would typically: pull source map
  tiles/vector data for your riding area, render each to a `TILE_SIZE x
  TILE_SIZE` monochrome image (e.g. via Pillow in Python, thresholding to
  1-bit), and write it out in this raw packed format at each zoom level you
  want to support (`MIN_ZOOM` to `MAX_ZOOM` in `Config.h`).

- Missing tiles (not on the card) render as blank/white rather than
  crashing - useful for partial map coverage of just your usual riding area.

## GPX parsing notes

The parser is a lightweight line-based scanner, not a full XML parser - it
looks for `<trkpt lat="..." lon="...">`, `<wpt lat="..." lon="...">`, and
`<name>` tags. This works with typical GPX 1.1 output (Strava, RideWithGPS,
GPX Studio, etc.) but has not been exhaustively tested against every GPX
variant - if a specific exported file fails to load, check whether its tags
span multiple lines in an unusual way.

Long routes are automatically decimated (every Nth point kept, coarsening
further if needed) to fit within `MAX_TRACK_POINTS` (1500) so very long GPX
files don't exhaust RAM - the overall route shape is preserved even if some
fine detail is thinned out.

## Encoder / button behavior implemented

- **Home screen**: file list from `/routes`, CW = move right/down, CCW =
  move left/up, press = load route and open map screen.
- **Map screen**: three-button bar (Back, Start/Stop, Zoom), same CW/CCW
  cycling, press = activate. Back is removed from the navigation cycle
  entirely while a ride is being recorded. Start/Stop toggles GPS ride
  recording (distance, avg speed, current speed). Zoom press enters a
  sub-mode where the encoder controls zoom level instead of button
  navigation; pressing again returns to normal navigation with the zoom
  level retained.
- Position arrow (heading-pointed) only appears once recording has started,
  per spec. GPS heading is frozen below ~1 m/s to avoid jitter when
  stopped/slow (see `GPS_STOPPED_THRESHOLD_MPS` in `Config.h`).

