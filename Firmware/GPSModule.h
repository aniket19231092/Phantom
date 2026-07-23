#pragma once
/*
 * GPSModule.h
 *
 * Wraps TinyGPS++ over HardwareSerial. Provides position, heading (frozen
 * below a speed threshold to avoid jitter when stopped), and ride-recording
 * stats (distance via haversine summation between fixes, current/avg speed).
 *
 * Requires the "TinyGPSPlus" library (Mikal Hart) - install via Library
 * Manager.
 */

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Config.h"

struct GPSFix {
  bool valid;
  double lat;
  double lon;
  float speedMps;
  float headingDeg;   // 0-360, 0 = north
  uint32_t timestampMs;
};

class GPSModule {
public:
  void begin();
  void update();                 // Call every loop() iteration

  GPSFix getFix() const { return _lastFix; }
  bool hasFix() const { return _lastFix.valid; }

  // Ride recording
  void startRecording();
  void stopRecording();
  bool isRecording() const { return _recording; }

  float getCurrentSpeedMps() const { return _displaySpeedMps; }
  float getAvgSpeedMps() const;
  float getDistanceMeters() const { return _distanceMeters; }
  float getDisplayHeadingDeg() const { return _displayHeadingDeg; }

private:
  HardwareSerial _serial{GPS_UART_NUM};
  TinyGPSPlus _tinyGps;

  GPSFix _lastFix{};
  bool _haveLastRecordedPoint = false;
  double _lastRecordedLat = 0, _lastRecordedLon = 0;

  bool _recording = false;
  float _distanceMeters = 0;
  uint32_t _recordingStartMs = 0;
  uint32_t _movingTimeMs = 0;
  uint32_t _lastUpdateMs = 0;

  float _displaySpeedMps = 0;
  float _displayHeadingDeg = 0;

  static double haversineMeters(double lat1, double lon1, double lat2, double lon2);
};
