#pragma once
/*
 * UIScreens.h
 *
 * Top-level app state machine:
 *
 *   HOME_BROWSER  - list of .gpx files from SD, encoder scrolls
 *                   (CW = right/down through list, CCW = left/up),
 *                   button press loads the highlighted file and switches
 *                   to MAP_VIEW.
 *
 *   MAP_VIEW      - shows the map + route + waypoints. A row of three
 *                   "buttons" (Back, Start/Stop, Zoom) is navigated with
 *                   the encoder the same way, button press activates the
 *                   highlighted one.
 *                     - Start/Stop: toggles GPS ride recording. While
 *                       recording, the Back button is removed from the
 *                       navigation cycle (cannot be reached or used).
 *                     - Back: returns to HOME_BROWSER (only reachable when
 *                       not recording).
 *                     - Zoom: enters ZOOMING sub-mode, where the encoder
 *                       controls map zoom level instead of button
 *                       navigation. Pressing the button again exits
 *                       ZOOMING (zoom level is retained) back to normal
 *                       button navigation.
 */

#include <Arduino.h>
#include <vector>
#include "Config.h"
#include "SharpMemLCD.h"
#include "RotaryEncoder.h"
#include "GPSModule.h"
#include "SDStorage.h"
#include "MapView.h"
#include "BatteryMonitor.h"

enum class AppState {
  HOME_BROWSER,
  MAP_VIEW
};

enum class MapMode {
  NAVIGATE,   // encoder cycles between the three buttons
  ZOOMING     // encoder controls zoom level instead
};

enum class MapButton {
  BACK = 0,
  START_STOP = 1,
  ZOOM = 2,
  COUNT = 3
};

class UIScreens {
public:
  void begin(SharpMemLCD *display, RotaryEncoder *encoder, GPSModule *gps,
             SDStorage *storage, BatteryMonitor *battery);

  // Call every loop() iteration. Handles input + triggers a display refresh
  // whenever something visible changed.
  void update();

private:
  SharpMemLCD *_display = nullptr;
  RotaryEncoder *_encoder = nullptr;
  GPSModule *_gps = nullptr;
  SDStorage *_storage = nullptr;
  BatteryMonitor *_battery = nullptr;

  MapView _mapView;

  AppState _state = AppState::HOME_BROWSER;
  MapMode _mapMode = MapMode::NAVIGATE;

  // --- Home browser state ---
  std::vector<String> _fileList;
  int _selectedFileIndex = 0;
  GPXRoute _currentRoute;
  bool _routeLoaded = false;

  // --- Map view state ---
  MapButton _selectedButton = MapButton::START_STOP;

  bool _needsRedraw = true;

  void handleHomeBrowserInput(int16_t steps, bool pressed);
  void handleMapViewInput(int16_t steps, bool pressed);

  void nextSelectableButton(int direction); // +1 = CW/right, -1 = CCW/left
  bool isButtonSelectable(MapButton b) const;

  void renderHomeBrowser();
  void renderMapView();
};
