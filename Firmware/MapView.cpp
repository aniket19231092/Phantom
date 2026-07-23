#include "MapView.h"
#include <math.h>

void MapView::begin(SDStorage *storage) {
  _storage = storage;
  _zoom = DEFAULT_ZOOM;
}

void MapView::setRoute(const GPXRoute *route) {
  _route = route;
  if (route && !route->track.empty()) {
    centerOn(route->track[0].lat, route->track[0].lon);
  }
}

void MapView::centerOn(double lat, double lon) {
  _centerLat = lat;
  _centerLon = lon;
}

void MapView::setZoom(int zoom) {
  _zoom = constrain(zoom, MIN_ZOOM, MAX_ZOOM);
}

void MapView::zoomIn()  { setZoom(_zoom + 1); }
void MapView::zoomOut() { setZoom(_zoom - 1); }

void MapView::latLonToWorldPixel(double lat, double lon, int zoom, double &px, double &py) const {
  double n = pow(2.0, zoom);
  double latRad = radians(lat);

  px = (lon + 180.0) / 360.0 * (n * TILE_SIZE);
  py = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * (n * TILE_SIZE);
}

void MapView::render(SharpMemLCD &display, const GPSFix &fix, bool showPositionArrow) {
  double centerPx, centerPy;
  latLonToWorldPixel(_centerLat, _centerLon, _zoom, centerPx, centerPy);

  display.fillScreen(1); // clear to white before compositing this frame

  drawTiles(display, centerPx, centerPy);
  drawRoute(display, centerPx, centerPy);
  drawWaypoints(display, centerPx, centerPy);

  if (showPositionArrow && fix.valid) {
    drawPositionArrow(display, fix.headingDeg);
  }
}

void MapView::drawTiles(SharpMemLCD &display, double centerPx, double centerPy) {
  if (!_storage) return;

  // Screen top-left corner in world-pixel space
  double screenOriginPx = centerPx - LCD_WIDTH / 2.0;
  double screenOriginPy = centerPy - LCD_HEIGHT / 2.0;

  long firstTileX = (long)floor(screenOriginPx / TILE_SIZE);
  long firstTileY = (long)floor(screenOriginPy / TILE_SIZE);
  long lastTileX  = (long)floor((screenOriginPx + LCD_WIDTH) / TILE_SIZE);
  long lastTileY  = (long)floor((screenOriginPy + LCD_HEIGHT) / TILE_SIZE);

  static uint8_t tileBuf[TILE_BYTES];

  for (long ty = firstTileY; ty <= lastTileY; ty++) {
    for (long tx = firstTileX; tx <= lastTileX; tx++) {
      int screenX0 = (int)(tx * TILE_SIZE - screenOriginPx);
      int screenY0 = (int)(ty * TILE_SIZE - screenOriginPy);

      bool ok = _storage->loadTile(_zoom, tx, ty, tileBuf);
      if (!ok) {
        // Missing tile - leave it blank (already white from fillScreen)
        continue;
      }

      // NOTE: this blits one pixel at a time via drawPixel for simplicity
      // and to stay correct regardless of screen alignment. If frame rate
      // becomes a bottleneck, replace with a byte-aligned direct framebuffer
      // copy for the common case where screenX0 % 8 == 0.
      for (int row = 0; row < TILE_SIZE; row++) {
        int sy = screenY0 + row;
        if (sy < 0 || sy >= LCD_HEIGHT) continue;

        for (int col = 0; col < TILE_SIZE; col++) {
          int sx = screenX0 + col;
          if (sx < 0 || sx >= LCD_WIDTH) continue;

          int byteIdx = (row * TILE_SIZE + col) / 8;
          int bitIdx  = 7 - ((row * TILE_SIZE + col) % 8);
          bool white = (tileBuf[byteIdx] >> bitIdx) & 0x01;
          display.drawPixel(sx, sy, white ? 1 : 0);
        }
      }
    }
  }
}

void MapView::drawRoute(SharpMemLCD &display, double centerPx, double centerPy) {
  if (!_route || _route->track.size() < 2) return;

  double screenOriginPx = centerPx - LCD_WIDTH / 2.0;
  double screenOriginPy = centerPy - LCD_HEIGHT / 2.0;

  double prevPx, prevPy;
  latLonToWorldPixel(_route->track[0].lat, _route->track[0].lon, _zoom, prevPx, prevPy);

  for (size_t i = 1; i < _route->track.size(); i++) {
    double px, py;
    latLonToWorldPixel(_route->track[i].lat, _route->track[i].lon, _zoom, px, py);

    int x0 = (int)(prevPx - screenOriginPx);
    int y0 = (int)(prevPy - screenOriginPy);
    int x1 = (int)(px - screenOriginPx);
    int y1 = (int)(py - screenOriginPy);

    display.drawLine(x0, y0, x1, y1, 0); // black route line

    prevPx = px;
    prevPy = py;
  }
}

void MapView::drawWaypoints(SharpMemLCD &display, double centerPx, double centerPy) {
  if (!_route) return;

  double screenOriginPx = centerPx - LCD_WIDTH / 2.0;
  double screenOriginPy = centerPy - LCD_HEIGHT / 2.0;

  for (const auto &wpt : _route->waypoints) {
    double px, py;
    latLonToWorldPixel(wpt.lat, wpt.lon, _zoom, px, py);

    int x = (int)(px - screenOriginPx);
    int y = (int)(py - screenOriginPy);

    if (x < -5 || x > LCD_WIDTH + 5 || y < -5 || y > LCD_HEIGHT + 5) continue;

    display.drawCircle(x, y, 4, 0);
    display.fillCircle(x, y, 2, 0);
  }
}

void MapView::drawPositionArrow(SharpMemLCD &display, float headingDeg) {
  // Arrow is always drawn at screen center, pointing toward headingDeg
  // (0 = up/north). The map itself is not rotated - only the arrow rotates.
  const int cx = LCD_WIDTH / 2;
  const int cy = LCD_HEIGHT / 2;
  const int len = 14;

  float rad = radians(headingDeg);
  int tipX  = cx + (int)(len * sin(rad));
  int tipY  = cy - (int)(len * cos(rad));

  float backRad1 = radians(headingDeg + 150);
  float backRad2 = radians(headingDeg - 150);
  int backX1 = cx + (int)((len * 0.6) * sin(backRad1));
  int backY1 = cy - (int)((len * 0.6) * cos(backRad1));
  int backX2 = cx + (int)((len * 0.6) * sin(backRad2));
  int backY2 = cy - (int)((len * 0.6) * cos(backRad2));

  display.fillTriangle(tipX, tipY, backX1, backY1, backX2, backY2, 0);
}
