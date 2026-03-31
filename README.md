Use Milkdrop presets with [projectM](https://github.com/projectM-visualizer/projectm) and openFrameworks.

## Rebuilding bundled projectM libraries

Both helper scripts clone and build `projectM` in a temporary directory outside the addon tree, then copy only the finished libraries and headers back into `libs/projectM`. This avoids broken Git checkouts when the addon lives inside a OneDrive-synced folder.

### Windows

Run:

```powershell
scripts\build_projectm_windows.cmd
```

Outputs:

- `libs/projectM/lib/vs/libprojectM-4.lib`
- `libs/projectM/lib/vs/libprojectM-4-playlist.lib`
- `libs/projectM/include/projectM-4`

### Linux / WSL

Run:

```bash
bash scripts/build_projectm_linux.sh
```

Outputs:

- `libs/projectM/lib/linux64/libprojectM-4.a`
- `libs/projectM/lib/linux64/libprojectM-4-playlist.a`
- `libs/projectM/include/projectM-4`

## Recommended assets

These packs are recommended for Milkdrop content:

- [presets-cream-of-the-crop](https://github.com/projectM-visualizer/presets-cream-of-the-crop)
- [presets-milkdrop-texture-pack](https://github.com/projectM-visualizer/presets-milkdrop-texture-pack)

Place them like this:

- Presets from `presets-cream-of-the-crop` go in the `presets` folder.
- Textures from `presets-milkdrop-texture-pack` go in the `textures` folder.
