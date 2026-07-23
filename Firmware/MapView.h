#pragma once
/*
 * MapView.h
 *
 * Renders pre-rendered raster tiles (see Config.h TILE_* for the exact
 * on-disk format your PC-side prep script must produce) plus the GPX route
 * overlay, waypoint markers, and a heading-pointed position arrow.
 *
 * Uses standard Web Mercator "slippy map" tile math (the same scheme
 * OpenStreetMap / most tile servers use), just with TILE_SIZE=200 instead of
 * the usual 256 to better fit this display and RAM budget.
 */

#include <Arduino.h>
#include "Config.h"
#include "SharpMemLCD.h"
#include "SDStorage.h"
#include "GPSModule.h"

class MapView {
public:
  void begin(SDStorage *storage);

  void setRoute(const GPXRoute *route);   // may be nullptr for "no route loaded"
  void centerOn(double lat, double lon);

  void setZoom(int zoom);
  void zoomIn();
  void zoomOut();
  int getZoom() const { return _zoom; }

  // Renders the current view into the display's framebuffer (does not call
  // refresh() itself - caller decides when to push to the panel).
  void render(SharpMemLCD &display, const GPSFix &fix, bool showPositionArrow);

private:
  SDStorage *_storage = nullptr;
  const GPXRoute *_route = nullptr;

  double _centerLat = 0;
  double _centerLon = 0;
  int _zoom = DEFAULT_ZOOM;

  // Web Mercator projection: returns pixel coordinates in the "world pixel"
  // space at the current zoom level (continuous, not tile-quantized).
  void latLonToWorldPixel(double lat, double lon, int zoom, double &px, double &py) const;

  void drawTiles(SharpMemLCD &display, double centerPx, double centerPy);
  void drawRoute(SharpMemLCD &display, double centerPx, double centerPy);
  void drawWaypoints(SharpMemLCD &display, double centerPx, double centerPy);
  void drawPositionArrow(SharpMemLCD &display, float headingDeg);
};
