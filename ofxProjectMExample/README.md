# ofxProjectMExample

Primary `ofxProjectM` example with `ofxVlc4`, a simple playlist, and an ImGui control panel.

What it shows:

- `libVLC` playback feeding audio-reactive `projectM`
- a compact playlist UI with replace/add/remove/clear behavior
- side-by-side video and `projectM` displays
- preset browsing and texture routing

## Addon dependencies

- `ofxProjectM`
- `ofxVlc4`
- `ofxImGui` on its `develop` branch

## Runtime assets

For a useful `projectM` result, place preset and texture packs in:

- `bin/data/presets`
- `bin/data/textures`

If you package your own standalone app, copy the preset and texture folders you need into that app's `bin/data`.

Quick setup (`ofxVlc4` must be prepared first):

```bash
bash ../scripts/download-projectm-assets.sh
bash ../ofxVlc4/scripts/install-libvlc.sh
bash ../scripts/sync_vlc_runtime.sh
```

`addons.make` gives Project Generator the `ofxVlc4` code dependency, but the actual VLC runtime still comes from `ofxVlc4/scripts/install-libvlc.sh`.

This repo keeps `bin/data` present but empty by default. Download presets/textures when you want them locally, and add optional media seed files like `finger.mp4` or `fingers.mp4` only when you want the example to open media on startup.

## Controls

The main controls are in the ImGui panel:

- open/replace media
- add media to the playlist
- play / pause / stop
- remove selected item / clear playlist
- previous / next / random preset
- toggle whether `projectM` sees the VLC video texture

Keyboard fallback:

- `O`: open/replace media
- `A`: add media
- `Space`: play / pause
- `S`: stop
- `Up` / `Down`: move through playlist
- `Delete`: remove selected playlist item
- `Left` / `Right`: previous / next preset
- `N`: random preset
- `T`: toggle the VLC texture feed


