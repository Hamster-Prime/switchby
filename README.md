# SwitchBy

A native Emby client for Nintendo Switch, built with C++ and Borealis UI framework.

## Features

- **Server Connection** - Connect to your Emby server
- **User Authentication** - Login with username/password
- **Media Library Browsing** - Browse movies, TV shows, and more
- **Video Playback** - Hardware-accelerated playback via MPV
- **Resume Playback** - Continue watching from where you left off
- **Search** - Find content across your library

## Building for Nintendo Switch

This project uses the **devkitPro** toolchain and the **Borealis** UI library (specifically the `wiliwili` branch for CMake support).

### One-Click Build Script (Recommended)

A helper script is provided to automatically set up the environment (on Debian/Ubuntu/WSL) and build the project.

```bash
sudo ./build_switch.sh
```

This script will:
1. Install devkitPro and `dkp-pacman`.
2. Install the required Switch libraries (`switch-libmpv`, `switch-ffmpeg`, etc.).
3. Clone necessary dependencies locally (`borealis`, `json`, etc.) to avoid timeout issues.
4. Compile the project and output `build/switchby.nro`.

### Manual Build Instructions

If you prefer to build manually or are on a different OS:

#### 1. Prerequisites

- **devkitPro**: Follow the instructions at [devkitPro Getting Started](https://devkitpro.org/wiki/Getting_Started).
- **Switch Libraries**: Install the following packages using `dkp-pacman`:
  ```bash
  dkp-pacman -S switch-dev switch-curl switch-libmpv switch-ffmpeg \
                switch-mbedtls switch-libjpeg-turbo switch-freetype \
                switch-sdl2 switch-zlib switch-libpng switch-bzip2 \
                switch-mesa
  ```

#### 2. Build

```bash
# Set environment variables
export DEVKITPRO=/opt/devkitpro # Adjust path if different
export DEVKITA64=$DEVKITPRO/devkitA64

# Create build directory
mkdir build && cd build

# Configure CMake (for Switch)
# Note: FetchContent might time out on slow connections. 
# Pre-cloning dependencies to build/_deps/ is recommended if that happens.
cmake .. -DPLATFORM=SWITCH -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake

# Build
make -j$(nproc)
```

The output `switchby.nro` will be in the `build/` directory.

## Installation

1. Copy `switchby.nro` to `/switch/switchby/` on your SD card.
2. Launch via Homebrew Menu.

## Credits

- [Borealis](https://github.com/natinusala/borealis) - UI Framework
- [wiliwili](https://github.com/xfangfang/wiliwili) - For the CMake-compatible Borealis fork and MPV integration reference.
- [MPV](https://mpv.io/) - Video playback core.
- [devkitPro](https://devkitpro.org/) - Nintendo Switch homebrew toolchain.

## License

MIT License