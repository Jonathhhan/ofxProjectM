# ofxProjectMSimpleVlcExample

Small example that wires `ofxVlc4` into `ofxProjectM`.

What it shows:

- `libvlc` plays a dropped or opened media file
- captured VLC audio is sent both to the system output and to `projectM`
- the current VLC video texture can optionally be exposed to `projectM`
- an ImGui control panel for transport, preset changes, and texture routing

## Addon dependencies

- `ofxProjectM`
- `ofxVlc4`
- `ofxImGui`

## Runtime assets

For a useful `projectM` result, place preset and texture packs in:

- `bin/data/presets`
- `bin/data/textures`

If you package your own standalone app, copy the preset and texture folders you need into that app's `bin/data`.

The full packs are the same ones listed in the main addon README:

- `presets-cream-of-the-crop`
- `presets-milkdrop-texture-pack`

Quick setup:

```bash
bash ../scripts/download-projectm-assets.sh --example ofxProjectMSimpleVlcExample
bash ../scripts/sync_vlc_runtime.sh
```

This repo keeps `bin/data` present but empty by default. Add presets, textures, and optional media seeds only when you want them locally.

Optional media seed files can be placed in `bin/data` as:

- `movie.mp4`
- `sample.mp4`

## Controls

The main controls are in the ImGui panel:

- open media
- play / pause
- previous / next / random preset
- toggle whether `projectM` sees the VLC video texture

Keyboard fallback:

- `O`: open a media file
- `Space`: play / pause
- `Left` / `Right`: previous / next preset
- `N`: random preset
- `T`: toggle whether `projectM` sees the VLC video texture

You can also drag a media file onto the window.
