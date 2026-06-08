# QML Flight Instruments

This an update of [Berkbavas QML port](https://github.com/berkbavas/QmlFlightInstruments)
of [Marek M. Cel](http://marekcel.pl/)'
s [QFlightinstruments](https://github.com/marek-cel/QFlightinstruments). This update:

- Is compatible with Qt6 (tested against 6.11.0). This required many changes, including the replacement of the Qt Quick
  Extras CircularGauge with a custom circular gauge.
- Replaces the Qt project structure with a pure CMake build system

SVG files are from Marek's repository. See `application/ui/images/LICENSE` for copyright.

## Features

- **Electronic Flight Instrument System (EFIS)**
    - Electronic Attitude Direction Indicator (EADI)
    - Electronic Horizontal Situation Indicator (EHSI)
- **Basic Six**
    - Airspeed Indicator (ASI)
    - Attitude Indicator (AI)
    - Altimeter (ALT)
    - Turn Coordinator (TC)
    - Heading Indicator (HI)
    - Vertical Speed Indicator (VSI)
- **Additional Gauges**
    - Fuel Tank
    - Exhaust Gas Temperature (EGT) & Fuel Flow
    - Propeller
    - Battery
    - Temperature & Pressure

## Building

1. Install **Qt 6.11.0**.
2. Open `CMakeLists.txt` in **Your IDE Of Choice**.
3. Update the root `CMakeLists.txt` file to reflect your **QT_INSTALL_LOCATION** and **QT_VERSION_TO_USE**
    - Optionally, update **APPLICATION_URI**, **APPLICATION_BUNDLE_GUI_ID** and **APPLICATION_ICON_PATH** for macOS
      deployment
4. Build and run the project.

I have only test on macOS so far, but plan on building in Linux in the near future. Testing on Windows would be
appreciated!

## Credits & License

- Original code and SVGs by Marek M. Cel. See [QFlightinstruments](https://github.com/marek-cel/QFlightinstruments).
- Additional gauges and QML port by this project.
- This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
