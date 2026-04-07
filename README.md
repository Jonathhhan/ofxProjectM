# ofxProjectM

`ofxProjectM` wraps `projectM 4` for openFrameworks and focuses on Milkdrop-style visual playback, texture output, playlist control, runtime parameter control, logging, touch input, and sprites.

The addon is useful both as a standalone visualizer wrapper and as a companion addon for the full `ofxVlc4` example.

Examples included in this addon:

- `ofxProjectMExample`
  - standalone audio-driven `projectM` with an ImGui control panel
- `ofxProjectMSimpleVlcExample`
  - minimal `ofxProjectM` + `ofxVlc4` bridge with VLC audio capture and an ImGui control panel

## Note

Parts of this addon, its examples, and its documentation were developed with AI-assisted help during implementation and refinement.

## Release

- addon release version: `1.0.1`
- changelog: `CHANGELOG.md`

## Highlights

- `projectM 4` playback in openFrameworks
- internal FBO/texture output
- preset playlist and preset switching helpers
- runtime parameters:
  - FPS
  - mesh size
  - frame time
  - beat sensitivity
  - cut settings
  - texel offset
  - easter egg
  - preset start clean
- texture search path control
- runtime logging callback + log level
- touch API
- user sprites
- version info helpers

## Version helpers

There are now two different version concepts available:

- addon version
  - `ofxProjectM::getAddonVersionInfo()`
- wrapped runtime version
  - `ofxProjectM::getVersionInfo()`
  - reports the linked `projectM` runtime version and VCS string

## Source layout

The public facade stays in:

- `src/ofxProjectM.h`

Implementation is split into:

- `src/ofxProjectM.cpp`
  - core lifecycle, logging, shared callbacks, runtime parameters
- `src/ofxProjectMPlaylist.cpp`
  - playlist, preset loading, switching, filtering
- `src/ofxProjectMRender.cpp`
  - FBO/texture rendering, audio input, touch, sprites

Small internal helper headers:

- `src/ofxProjectMPlaylist.h`
- `src/ofxProjectMRender.h`

## Building projectM libraries

The unified shell script clones and builds `projectM` in a temporary directory outside the addon tree, then copies only the finished libraries and headers back into `libs/projectM`.

The repository keeps `libs/projectM` as placeholder directories only. Run the build script after cloning if you want the local static libraries and headers populated.

If you want one entry point across platforms, use:

```bash
bash scripts/build_projectm.sh
```

The wrapper dispatches to:

- macOS: native static build into `libs/projectM/lib/osx`
- Linux: native static build into `libs/projectM/lib/linux64`
- Git Bash / MSYS / Cygwin on Windows: embedded PowerShell build into `libs/projectM/lib/vs`
- WSL: auto-detected as Windows by default so a Windows checkout still fills `libs/projectM/lib/vs`

### Windows

Run:

```bash
bash scripts/build_projectm.sh
```

This builds both `Debug` and `Release` by default on Windows.
WSL now follows this Windows path automatically unless you override it.
If you only want one configuration, use for example:

```bash
bash scripts/build_projectm.sh --configuration Release
```

Outputs:

- `libs/projectM/lib/vs/Debug/libprojectM-4.lib`
- `libs/projectM/lib/vs/Debug/libprojectM-4-playlist.lib`
- `libs/projectM/lib/vs/Release/libprojectM-4.lib`
- `libs/projectM/lib/vs/Release/libprojectM-4-playlist.lib`
- `libs/projectM/include/projectM-4`

## VLC bridge example runtime

`ofxProjectMSimpleVlcExample` depends on `ofxVlc4` for `libVLC`.

For a normal local setup with Project Generator, first install the VLC runtime in the sibling addon:

```bash
bash ../ofxVlc4/scripts/install-libvlc.sh
```

Then sync the runnable VLC layout into the simple bridge example:

```bash
bash scripts/sync_vlc_runtime.sh
```

That prepares:

- `ofxProjectMSimpleVlcExample/bin/libvlc.dll`
- `ofxProjectMSimpleVlcExample/bin/libvlccore.dll`
- `ofxProjectMSimpleVlcExample/bin/plugins/`
- `ofxProjectMSimpleVlcExample/bin/lua/`
- `ofxProjectMSimpleVlcExample/dll/x64/` with only the root VLC DLLs needed by generated Visual Studio projects

### macOS

Run:

```bash
bash scripts/build_projectm.sh
```

Outputs:

- `libs/projectM/lib/osx/libprojectM-4.a`
- `libs/projectM/lib/osx/libprojectM-4-playlist.a`
- `libs/projectM/include/projectM-4`

By default the script asks CMake for a universal `arm64;x86_64` build. You can override that before the run, for example:

```bash
OSX_ARCHITECTURES=arm64 bash scripts/build_projectm.sh
```

### Linux / WSL

Run:

```bash
bash scripts/build_projectm.sh
```

Outputs:

- `libs/projectM/lib/linux64/libprojectM-4.a`
- `libs/projectM/lib/linux64/libprojectM-4-playlist.a`
- `libs/projectM/include/projectM-4`

If you are running inside WSL and want Linux artifacts instead of the default Windows/Visual Studio path, use:

```bash
bash scripts/build_projectm.sh --target linux
```

## Runtime assets

The addon helper downloads the Cream of the Crop preset pack and the full texture pack:

- [presets-cream-of-the-crop](https://github.com/projectM-visualizer/presets-cream-of-the-crop)
- [presets-milkdrop-texture-pack](https://github.com/projectM-visualizer/presets-milkdrop-texture-pack)

By default it installs both:

```bash
bash scripts/download-projectm-assets.sh
```

Useful variants:

```bash
bash scripts/download-projectm-assets.sh --example ofxProjectMSimpleVlcExample
bash scripts/download-projectm-assets.sh --presets
bash scripts/download-projectm-assets.sh --textures --example ofxProjectMSimpleVlcExample
```

The script installs into:

- `ofxProjectMExample/bin/data/presets`
- `ofxProjectMExample/bin/data/textures`
- `ofxProjectMSimpleVlcExample/bin/data/presets`
- `ofxProjectMSimpleVlcExample/bin/data/textures`

The packs are intentionally separate because they are fairly large.
For normal addon development, keep the built `projectM` libraries shared under `libs/projectM` and only copy the preset and texture folders into an app's `bin/data` when you want that app to be truly standalone.

Manual layout if you prefer:

- presets from `presets-cream-of-the-crop` go in the `presets` folder
- textures from `presets-milkdrop-texture-pack` go in the `textures` folder

## Notes

- The addon uses one public `ofxProjectM` facade even though implementation is now split internally.
- The source split does not require `addon_config.mk` updates because openFrameworks auto-scans `src`.
- The full `ofxVlc4Example` exposes part of the `projectM` surface, but the addon API is broader than the current GUI.
