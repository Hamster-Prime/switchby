# SwitchBy

[![Build Switch NRO](https://github.com/Hamster-Prime/switchby/actions/workflows/build.yml/badge.svg)](https://github.com/Hamster-Prime/switchby/actions/workflows/build.yml)

A native Emby client for Nintendo Switch, built with C++ and Borealis UI framework.

## Features

- **Server Connection** - Connect to your Emby server
- **User Authentication** - Login with username/password
- **Media Library Browsing** - Browse movies, TV shows, and more
- **Video Playback** - Hardware-accelerated playback via MPV
- **Resume Playback** - Continue watching from where you left off
- **Search** - Find content across your library
- **Playback Reporting** - Sync watch progress with server

## Screenshots

*Coming soon*

## Installation

1. Download the latest `switchby.nro` from [Releases](https://github.com/Hamster-Prime/switchby/releases)
2. Copy to `/switch/switchby/` on your SD card
3. Launch via Homebrew Menu

## Build from Source

### Prerequisites

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with Switch toolchain
- CMake 3.15+

### Build Commands

```bash
# Clone the repository
git clone https://github.com/Hamster-Prime/switchby.git
cd switchby

# Build for Switch
source $DEVKITPRO/switchvars.sh
mkdir build && cd build
cmake .. -DPLATFORM=SWITCH -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake
make -j$(nproc)
```

The output `switchby.nro` will be in the `build/` directory.

## Project Structure

```
switchby/
├── src/
│   ├── main.cpp                 # Entry point
│   ├── api/
│   │   ├── emby_client.hpp      # Emby API client
│   │   └── emby_client.cpp
│   ├── activity/
│   │   ├── server_select_activity.hpp
│   │   ├── login_activity.hpp
│   │   ├── tab_activity.hpp
│   │   ├── home_tab.hpp
│   │   ├── library_activity.hpp
│   │   ├── detail_activity.hpp/cpp
│   │   ├── player_activity.hpp/cpp
│   │   ├── search_activity.hpp
│   │   └── settings_activity.hpp
│   ├── utils/
│   │   ├── config.hpp           # Configuration management
│   │   ├── thread_pool.hpp      # Async task handling
│   │   └── cache_manager.hpp    # Image cache LRU
│   └── view/
│       └── poster_cell.hpp      # Media poster component
├── resources/                   # App resources
├── CMakeLists.txt
└── .github/workflows/build.yml  # CI/CD
```

## Dependencies

| Library | Purpose |
|---------|---------|
| [borealis](https://github.com/natinusala/borealis) | Switch UI framework |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing |
| [libcurl](https://curl.se/libcurl/) | HTTP client |
| [mpv](https://mpv.io/) | Video playback |

## Configuration

Config file is stored at `/switch/switchby/config.json`:

```json
{
  "servers": [
    {
      "url": "http://your-server:8096",
      "name": "My Emby",
      "userId": "...",
      "accessToken": "...",
      "username": "user"
    }
  ],
  "lastServerIndex": 0
}
```

## Controls

| Button | Action |
|--------|--------|
| A | Select / Play/Pause |
| B | Back |
| L/R | Seek ±10s |
| D-Pad | Seek ±5s / Navigate |
| X | Cycle audio track |
| Y | Cycle subtitles |

## Roadmap

- [ ] Jellyfin support
- [ ] Multiple user profiles
- [ ] Offline playback
- [ ] Live TV support

## Credits

- [wiliwili](https://github.com/xfangfang/wiliwili) - Inspiration & reference implementation
- [borealis](https://github.com/natinusala/borealis) - UI framework
- [devkitPro](https://devkitpro.org/) - Switch homebrew toolchain

## License

MIT License - see [LICENSE](LICENSE) file

## Disclaimer

This project is not affiliated with Emby or Nintendo. Use at your own risk.
