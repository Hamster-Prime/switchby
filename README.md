# SwitchBy

A native Emby client for Nintendo Switch, inspired by and built with the same tech stack as [wiliwili](https://github.com/xfangfang/wiliwili) (C++ / Borealis).

## Features (Planned)
- Native Switch UI (Borealis)
- Emby Server connection
- Hardware decoding (via ffmpeg/mpv integration)
- Touch & Controller support

## Build
Uses CMake and devkitPro standard workflow.

```bash
mkdir build
cd build
cmake .. -DPLATFORM=SWITCH
make
```

## Credits
- [borealis](https://github.com/nativelbs/borealis) - UI Library
- [wiliwili](https://github.com/xfangfang/wiliwili) - Inspiration & Reference
