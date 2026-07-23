#pragma once
/*
 * SDStorage.h
 *
 * SD card access over SDMMC (4-bit mode). Handles:
 *   - Listing .gpx files in ROUTES_DIR for the file-browser screen
 *   - Lightweight streaming GPX parser (track points + waypoints)
 *   - Loading pre-rendered map tiles (see Config.h for tile format spec)
 *
 * The GPX parser is intentionally NOT a full XML parser - GPX files have a
 * very predictable flat structure, so this does simple substring search per
 * line rather than pulling in a heavy XML library. Works with GPX 1.1 output
 * from most common tools (Strava, RideWithGPS, GPX Studio, etc.) but has not
 * been tested against every GPX variant in the wild - if a specific file
 * fails to parse, check whether its <trkpt>/<wpt> tags and lat/lon
 * attributes span multiple lines in an unusual way.
 */

#include <Arduino.h>
#include <vector>
#include "Config.h"

struct RoutePoint {
  float lat;
  float lon;
};

struct Waypoint {
  float lat;
  float lon;
  String name;
};

struct GPXRoute {
  String fileName;
  std::vector<RoutePoint> track;
  std::vector<Waypoint> waypoints;
};

// Cap how many track points we'll hold in RAM at once. Long routes get
// decimated (every Nth point kept) rather than truncated, so the whole
// route shape is still represented on screen.
#define MAX_TRACK_POINTS  1500

class SDStorage {
public:
  bool begin();
  bool cardPresent();

  // Returns list of .gpx filenames found in ROUTES_DIR (name only, no path)
  std::vector<String> listRoutes();

  // Parses ROUTES_DIR/filename into a GPXRoute. Returns false on failure.
  bool loadRoute(const String &fileName, GPXRoute &out);

  // Loads a single map tile into the provided buffer (must be TILE_BYTES
  // long). Returns false if the tile file doesn't exist on the card
  // (caller should draw a blank/placeholder tile in that case).
  bool loadTile(int zoom, long tileX, long tileY, uint8_t *buf);

private:
  bool _cardOk = false;
};
