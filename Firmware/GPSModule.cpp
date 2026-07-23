#include "GPSModule.h"

void GPSModule::begin() {
  _serial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
}

double GPSModule::haversineMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0; // Earth radius, meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) *
             sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void GPSModule::update() {
  while (_serial.available() > 0) {
    _tinyGps.encode(_serial.read());
  }

  bool freshFix = _tinyGps.location.isUpdated() && _tinyGps.location.isValid();

  if (freshFix) {
    _lastFix.valid = true;
    _lastFix.lat = _tinyGps.location.lat();
    _lastFix.lon = _tinyGps.location.lng();
    _lastFix.speedMps = _tinyGps.speed.isValid() ? (float)(_tinyGps.speed.kmph() / 3.6) : 0.0f;
    _lastFix.headingDeg = _tinyGps.course.isValid() ? (float)_tinyGps.course.deg() : _lastFix.headingDeg;
    _lastFix.timestampMs = millis();

    // Freeze displayed heading/speed jitter below the moving threshold -
    // GPS course-over-ground is unreliable at very low speed / standstill.
    if (_lastFix.speedMps >= GPS_STOPPED_THRESHOLD_MPS) {
      _displayHeadingDeg = _lastFix.headingDeg;
    }
    _displaySpeedMps = _lastFix.speedMps;

    if (_recording) {
      uint32_t now = millis();
      uint32_t dt = _lastUpdateMs ? (now - _lastUpdateMs) : 0;
      _lastUpdateMs = now;

      if (_haveLastRecordedPoint) {
        double d = haversineMeters(_lastRecordedLat, _lastRecordedLon,
                                    _lastFix.lat, _lastFix.lon);
        _distanceMeters += (float)d;

        if (_lastFix.speedMps >= GPS_STOPPED_THRESHOLD_MPS) {
          _movingTimeMs += dt;
        }
      }
      _lastRecordedLat = _lastFix.lat;
      _lastRecordedLon = _lastFix.lon;
      _haveLastRecordedPoint = true;
    }
  }

  // Fix timeout - mark invalid if we haven't heard a fix in a while so the
  // UI can show "no GPS" rather than a stale position indefinitely.
  if (_lastFix.valid && (millis() - _lastFix.timestampMs) > GPS_FIX_TIMEOUT_MS) {
    _lastFix.valid = false;
  }
}

void GPSModule::startRecording() {
  _recording = true;
  _distanceMeters = 0;
  _movingTimeMs = 0;
  _lastUpdateMs = 0;
  _haveLastRecordedPoint = false;
  _recordingStartMs = millis();
}

void GPSModule::stopRecording() {
  _recording = false;
}

float GPSModule::getAvgSpeedMps() const {
  if (_movingTimeMs == 0) return 0.0f;
  return _distanceMeters / (_movingTimeMs / 1000.0f);
}
