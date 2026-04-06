# Changelog

## Unreleased

## 1.0.1

Highlights:

- fixed `ofxProjectMExample` to use the current `projectM.init()` lifecycle instead of the removed `load()` call
- fixed `ofxProjectMSimpleVlcExample` autoplay after loading media into a fresh playlist
- added `ofxImGui` control panels to both examples:
  - `ofxProjectMExample`
  - `ofxProjectMSimpleVlcExample`
- normalized the VLC bridge example onto the current `ofxVlc4` addon name instead of the compatibility alias
- refreshed example documentation to match the current controls and setup flow

## 1.0.0

Initial public release of the current `ofxProjectM` addon wrapper.

Highlights:

- `projectM 4` wrapper for openFrameworks with texture/FBO rendering
- playlist and preset loading helpers
- runtime control for:
  - FPS
  - frame time
  - mesh size
  - beat sensitivity
  - cut behavior
  - texel offset
  - easter egg
  - preset start/lock behavior
- texture search path and debug image helpers
- logging helpers and runtime log callback control
- touch and user sprite support
- addon version helper:
  - `ofxProjectM::getAddonVersionInfo()`
- runtime version helper:
  - `ofxProjectM::getVersionInfo()`

Notes:

- addon release version: `1.0.0`
- runtime `projectM` version depends on the bundled/linked library in `libs/projectM`
