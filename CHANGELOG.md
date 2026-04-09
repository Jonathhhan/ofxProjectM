# Changelog

## 1.0.2

Highlights:

- added `isInitialized()` convenience method to check if the projectM engine is ready
- renamed `draw(x, y, a, b)` parameters to `draw(x, y, width, height)` for clarity
- guarded `ofRandom()` boundary edge case in `reloadPresets()` and `randomPreset()` with `std::min`
- added FBO allocation failure checks with error logging in `init()` and `setWindowSize()`
- removed unnecessary `const_cast` in callback registration methods
- added comments for unsupported platform sections in `addon_config.mk`
- added MIT LICENSE file with projectM LGPL-2.1 notice

## Unreleased

Highlights:

- removed the repo-owned app examples again
- documented `ofxVlc4Example` as the maintained `ofxProjectM` integration example
- moved preset and texture asset downloads out of `ofxProjectM` and into `ofxVlc4Example`
- added `audio(const ofSoundBuffer &)` overload for direct `ofxVlc4` integration

## 1.0.1

Highlights:

- fixed `ofxProjectMExample` to use the current `projectM.init()` lifecycle instead of the removed `load()` call
- fixed `ofxProjectMSimpleVlcExample` autoplay after loading media into a fresh playlist
- added `ofxImGui` control panels to both examples:
  - `ofxProjectMExample`
  - `ofxProjectMSimpleVlcExample`
- normalized the VLC bridge example onto the current `ofxVlc4` addon name instead of the compatibility alias
- refreshed example documentation to match the current controls and setup flow
- cleaned `libs/projectM` back to placeholder directories so the build script is the source of truth for generated headers and libraries

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
