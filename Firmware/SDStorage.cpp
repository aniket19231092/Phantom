#include "SDStorage.h"
#include "FS.h"
#include "SD_MMC.h"

bool SDStorage::begin() {
  pinMode(PIN_SD_CD, INPUT); // external pull-up already on board

  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_DAT0, PIN_SD_DAT1, PIN_SD_DAT2, PIN_SD_DAT3);

  if (!SD_MMC.begin("/sdcard", false /* 4-bit mode */)) {
    _cardOk = false;
    return false;
  }
  _cardOk = true;
  return true;
}

bool SDStorage::cardPresent() {
  // CD_PIN reads LOW when a card is physically inserted (switch to GND).
  return digitalRead(PIN_SD_CD) == LOW;
}

std::vector<String> SDStorage::listRoutes() {
  std::vector<String> result;
  if (!_cardOk) return result;

  File dir = SD_MMC.open(ROUTES_DIR);
  if (!dir || !dir.isDirectory()) return result;

  File entry = dir.openNextFile();
  while (entry) {
    String name = entry.name();
    if (!entry.isDirectory() && name.endsWith(".gpx")) {
      // entry.name() may return a full path depending on core version;
      // normalize to just the filename.
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      result.push_back(name);
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return result;
}

// --- Minimal helpers for the line-based GPX parser -------------------------

static bool extractAttr(const String &line, const char *attr, float &outVal) {
  String needle = String(attr) + "=\"";
  int idx = line.indexOf(needle);
  if (idx < 0) return false;
  int start = idx + needle.length();
  int end = line.indexOf('"', start);
  if (end < 0) return false;
  outVal = line.substring(start, end).toFloat();
  return true;
}

static String extractTagText(const String &line, const char *tag) {
  String openTag = String("<") + tag + ">";
  String closeTag = String("</") + tag + ">";
  int start = line.indexOf(openTag);
  if (start < 0) return "";
  start += openTag.length();
  int end = line.indexOf(closeTag, start);
  if (end < 0) return "";
  return line.substring(start, end);
}

bool SDStorage::loadRoute(const String &fileName, GPXRoute &out) {
  if (!_cardOk) return false;

  String path = String(ROUTES_DIR) + "/" + fileName;
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) return false;

  out.fileName = fileName;
  out.track.clear();
  out.waypoints.clear();

  bool insideWpt = false;
  Waypoint pendingWpt;

  // Decimation: if the file is larger than we can hold, keep every Nth
  // point. We do a rough two-pass-free estimate: count trkpt lines is not
  // known ahead of time on a stream, so instead we keep a running counter
  // and only start dropping points once we exceed the cap, thinning by
  // skipping every other point retroactively is not possible on a stream -
  // simplest robust approach: keep first MAX_TRACK_POINTS, then coarsen by
  // only accepting every Nth new point once the cap would otherwise be hit.
  uint32_t seenPoints = 0;
  uint32_t keepEvery = 1;

  while (f.available()) {
    String line = f.readStringUntil('\n');

    if (line.indexOf("<wpt ") >= 0) {
      insideWpt = true;
      pendingWpt = Waypoint{};
      extractAttr(line, "lat", pendingWpt.lat);
      extractAttr(line, "lon", pendingWpt.lon);
    } else if (insideWpt && line.indexOf("<name>") >= 0) {
      pendingWpt.name = extractTagText(line, "name");
    } else if (insideWpt && line.indexOf("</wpt>") >= 0) {
      out.waypoints.push_back(pendingWpt);
      insideWpt = false;
    } else if (line.indexOf("<trkpt ") >= 0) {
      float lat, lon;
      if (extractAttr(line, "lat", lat) && extractAttr(line, "lon", lon)) {
        seenPoints++;

        if (out.track.size() >= MAX_TRACK_POINTS) {
          // Re-thin: double the keep-interval and drop half of what we
          // already have, so long routes degrade gracefully instead of
          // just truncating at whatever point the buffer filled up.
          std::vector<RoutePoint> thinned;
          thinned.reserve(out.track.size() / 2 + 1);
          for (size_t i = 0; i < out.track.size(); i += 2) {
            thinned.push_back(out.track[i]);
          }
          out.track = thinned;
          keepEvery *= 2;
        }

        if ((seenPoints % keepEvery) == 0) {
          out.track.push_back(RoutePoint{lat, lon});
        }
      }
    }
  }

  f.close();
  return true;
}

bool SDStorage::loadTile(int zoom, long tileX, long tileY, uint8_t *buf) {
  if (!_cardOk) return false;

  String path = String(TILES_DIR) + "/" + zoom + "/" + tileX + "/" + tileY + ".bin";
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) return false;

  size_t n = f.read(buf, TILE_BYTES);
  f.close();
  return n == TILE_BYTES;
}
