# ofxProjectMExample

Standalone `ofxProjectM` example with an ImGui control panel.

What it shows:

- generated audio driving `projectM`
- preset browsing and random preset changes
- live control over volume, pan, and noise mode
- a simple 3D scene textured through `projectM`

## Addon dependencies

- `ofxProjectM`
- `ofxImGui`

## Runtime assets

For a useful `projectM` result, place preset and texture packs in:

- `bin/data/presets`
- `bin/data/textures`

If you package your own standalone app, copy the preset and texture folders you need into that app's `bin/data`.

Quick setup:

```bash
bash ../scripts/download-projectm-assets.sh --example ofxProjectMExample
```

## Controls

The main controls are in the ImGui panel:

- previous / next / random preset
- start / stop audio
- volume
- pan
- noise toggle

Keyboard fallback:

- `S`: start audio
- `E`: stop audio
- `+` / `-`: adjust volume
- any other key: random preset
