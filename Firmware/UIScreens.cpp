#include "UIScreens.h"

void UIScreens::begin(SharpMemLCD *display, RotaryEncoder *encoder, GPSModule *gps,
                       SDStorage *storage, BatteryMonitor *battery) {
  _display = display;
  _encoder = encoder;
  _gps = gps;
  _storage = storage;
  _battery = battery;

  _mapView.begin(storage);

  _fileList = _storage->listRoutes();
  _selectedFileIndex = 0;
  _state = AppState::HOME_BROWSER;
  _needsRedraw = true;
}

void UIScreens::update() {
  int16_t steps = _encoder->readSteps();
  bool pressed = _encoder->buttonPressed();

  if (steps != 0 || pressed) {
    _needsRedraw = true;
  }

  // The map screen redraws continuously anyway (position/stats update as
  // GPS comes in), the home browser only needs to redraw on input.
  if (_state == AppState::MAP_VIEW) {
    _needsRedraw = true;
  }

  switch (_state) {
    case AppState::HOME_BROWSER:
      handleHomeBrowserInput(steps, pressed);
      break;
    case AppState::MAP_VIEW:
      handleMapViewInput(steps, pressed);
      break;
  }

  if (_needsRedraw) {
    switch (_state) {
      case AppState::HOME_BROWSER: renderHomeBrowser(); break;
      case AppState::MAP_VIEW:     renderMapView();     break;
    }
    _display->refresh();
    _needsRedraw = false;
  }
}

// ---------------------------------------------------------------------------
// Home browser
// ---------------------------------------------------------------------------

void UIScreens::handleHomeBrowserInput(int16_t steps, bool pressed) {
  if (_fileList.empty()) return;

  if (steps != 0) {
    int newIndex = _selectedFileIndex + steps; // CW(+) = right/down, CCW(-) = left/up
    _selectedFileIndex = constrain(newIndex, 0, (int)_fileList.size() - 1);
  }

  if (pressed) {
    String fileName = _fileList[_selectedFileIndex];
    _routeLoaded = _storage->loadRoute(fileName, _currentRoute);
    if (_routeLoaded) {
      _mapView.setRoute(&_currentRoute);
      _mapView.setZoom(DEFAULT_ZOOM);
      _selectedButton = MapButton::START_STOP;
      _mapMode = MapMode::NAVIGATE;
      _state = AppState::MAP_VIEW;
    }
  }
}

void UIScreens::renderHomeBrowser() {
  _display->fillScreen(1);
  _display->setTextColor(0);
  _display->setTextSize(1);

  _display->setCursor(10, 10);
  _display->print("Select a route:");

  if (_fileList.empty()) {
    _display->setCursor(10, 40);
    _display->print("No .gpx files found on SD card");
    return;
  }

  const int rowHeight = 20;
  const int visibleRows = (LCD_HEIGHT - 30) / rowHeight;

  int firstVisible = _selectedFileIndex - visibleRows / 2;
  firstVisible = constrain(firstVisible, 0, max(0, (int)_fileList.size() - visibleRows));

  for (int i = 0; i < visibleRows && (firstVisible + i) < (int)_fileList.size(); i++) {
    int idx = firstVisible + i;
    int y = 30 + i * rowHeight;

    if (idx == _selectedFileIndex) {
      _display->fillRect(5, y - 2, LCD_WIDTH - 10, rowHeight - 2, 0);
      _display->setTextColor(1); // invert text on highlighted row
    } else {
      _display->setTextColor(0);
    }

    _display->setCursor(10, y);
    _display->print(_fileList[idx]);
  }
}

// ---------------------------------------------------------------------------
// Map view
// ---------------------------------------------------------------------------

bool UIScreens::isButtonSelectable(MapButton b) const {
  if (b == MapButton::BACK && _gps->isRecording()) return false;
  return true;
}

void UIScreens::nextSelectableButton(int direction) {
  int idx = (int)_selectedButton;
  int count = (int)MapButton::COUNT;

  for (int i = 0; i < count; i++) {
    idx = (idx + direction + count) % count;
    if (isButtonSelectable((MapButton)idx)) {
      _selectedButton = (MapButton)idx;
      return;
    }
  }
  // If nothing else is selectable (shouldn't happen - Start/Stop and Zoom
  // are always selectable), leave selection unchanged.
}

void UIScreens::handleMapViewInput(int16_t steps, bool pressed) {
  if (_mapMode == MapMode::ZOOMING) {
    if (steps > 0) {
      for (int i = 0; i < steps; i++) _mapView.zoomIn();   // CW = zoom in
    } else if (steps < 0) {
      for (int i = 0; i < -steps; i++) _mapView.zoomOut(); // CCW = zoom out
    }

    if (pressed) {
      // Exit zoom mode - zoom level is already stored in _mapView, nothing
      // else to persist. Return to normal button navigation.
      _mapMode = MapMode::NAVIGATE;
    }
    return;
  }

  // --- NAVIGATE mode ---
  if (steps > 0) {
    for (int i = 0; i < steps; i++) nextSelectableButton(+1); // CW = right
  } else if (steps < 0) {
    for (int i = 0; i < -steps; i++) nextSelectableButton(-1); // CCW = left
  }

  if (pressed) {
    switch (_selectedButton) {
      case MapButton::BACK:
        if (isButtonSelectable(MapButton::BACK)) {
          _state = AppState::HOME_BROWSER;
        }
        break;

      case MapButton::START_STOP:
        if (_gps->isRecording()) {
          _gps->stopRecording();
        } else {
          _gps->startRecording();
        }
        break;

      case MapButton::ZOOM:
        _mapMode = MapMode::ZOOMING;
        break;

      default:
        break;
    }
  }
}

void UIScreens::renderMapView() {
  GPSFix fix = _gps->getFix();
  bool recording = _gps->isRecording();

  // Keep the map centered on our current position while recording (and
  // whenever we have a fix at all) - otherwise stay centered on wherever
  // it was last set (e.g. route start).
  if (fix.valid) {
    _mapView.centerOn(fix.lat, fix.lon);
  }

  _mapView.render(*_display, fix, recording /* show arrow only once started */);

  // --- Stats overlay (only while recording, per spec) ---
  if (recording) {
    _display->setTextColor(0);
    _display->setTextSize(1);
    _display->fillRect(0, 0, LCD_WIDTH, 34, 1); // small opaque banner at top

    char buf[64];
    float curKph = _gps->getCurrentSpeedMps() * 3.6f;
    float avgKph = _gps->getAvgSpeedMps() * 3.6f;
    float distKm = _gps->getDistanceMeters() / 1000.0f;

    snprintf(buf, sizeof(buf), "Spd: %.1f  Avg: %.1f  Dist: %.2fkm", curKph, avgKph, distKm);
    _display->setCursor(6, 4);
    _display->print(buf);

    _display->drawLine(0, 34, LCD_WIDTH, 34, 0);
  }

  // --- Button bar along the bottom ---
  const int barY = LCD_HEIGHT - 24;
  const int btnW = LCD_WIDTH / 3;

  _display->drawLine(0, barY, LCD_WIDTH, barY, 0);

  const char *labels[3];
  labels[(int)MapButton::BACK] = "Back";
  labels[(int)MapButton::START_STOP] = recording ? "Stop" : "Start";
  labels[(int)MapButton::ZOOM] = (_mapMode == MapMode::ZOOMING) ? "Zoom*" : "Zoom";

  for (int i = 0; i < 3; i++) {
    int x0 = i * btnW;
    bool selectable = isButtonSelectable((MapButton)i);
    bool highlighted = (_mapMode == MapMode::NAVIGATE) && ((int)_selectedButton == i);

    if (highlighted) {
      _display->fillRect(x0 + 2, barY + 2, btnW - 4, 22, 0);
      _display->setTextColor(1);
    } else {
      _display->setTextColor(selectable ? 0 : 1); // "grey out" back button by inverting/dimming
    }

    _display->setCursor(x0 + 8, barY + 8);
    _display->print(labels[i]);
  }
}
