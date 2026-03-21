# Building SDL3 from Source for Tempest SDL3

Since SDL3 is not yet widely available in package managers, you may need to build it from source.

## Quick Build Guide

### 1. Install Build Dependencies

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    libwayland-dev libxkbcommon-dev wayland-protocols \
    extra-cmake-modules libx11-dev libxext-dev libxrandr-dev \
    libxrender-dev libxi-dev libxcursor-dev libxinerama-dev \
    libxxf86vm-dev libxss-dev libgl1-mesa-dev libdbus-1-dev
```

On Fedora:
```bash
sudo dnf install -y gcc cmake git pkgconfig \
    wayland-devel libxkbcommon-devel wayland-protocols \
    libX11-devel libXext-devel libXrandr-devel libXrender-devel \
    libXi-devel libXcursor-devel libXinerama-devel libXxf86vm-devel \
    libXScrnSaver-devel mesa-libGL-devel dbus-devel
```

### 2. Build SDL3

```bash
# Clone SDL3 repository
git clone https://github.com/libsdl-org/SDL.git sdl3
cd sdl3

# Checkout latest stable release
git checkout $(git tag | grep "release-" | sort -V | tail -1)

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=OFF
make -j$(nproc)

# Install (may require sudo)
sudo make install
```

### 3. Update library cache

```bash
sudo ldconfig
```

### 4. Build Tempest SDL3

```bash
cd /path/to/tempest-sdl3
./build_native.sh
```

## Minimal Build (No Video/Audio)

If you only need basic functionality and want to avoid video/audio dependencies:

```bash
cmake .. -DSDL_VIDEO=OFF -DSDL_AUDIO=OFF -DSDL_RENDER=OFF -DCMAKE_BUILD_TYPE=Release
```

**Note:** This may not work for Tempest SDL3 as it requires video functionality.

## Troubleshooting

### Missing X11/Wayland
If you get errors about missing X11 or Wayland:
- Install the required development packages as shown above
- Or try: `cmake .. -DSDL_VIDEO=OFF` (may not work for games)

### Permission denied
Use `sudo` for the install step, or install to a local directory:
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install
```

### SDL3 not found after install
Make sure pkg-config can find it:
```bash
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
```

## Alternative: Use Pre-built Binaries

Check if your distribution has SDL3 packages:
- Arch Linux: `sudo pacman -S sdl3`
- Some newer Ubuntu versions may have SDL3 in universe
- Fedora may have SDL3 in updates

## Docker Option

For a completely isolated build environment:

```bash
docker run -it --rm -v $(pwd):/src ubuntu:latest bash -c \
"apt update && apt install -y build-essential cmake git pkg-config \
libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules \
libx11-dev libxext-dev && \
git clone https://github.com/libsdl-org/SDL.git && \
cd SDL && git checkout $(git tag | grep 'release-' | sort -V | tail -1) && \
mkdir build && cd build && \
cmake .. -DCMAKE_BUILD_TYPE=Release && \
make -j$(nproc) && \
make install && \
ldconfig"
```
