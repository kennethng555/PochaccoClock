# PochaccoClock

Prerequesites
```
git clone https://github.com/libsdl-org/SDL.git vendored/SDL
git clone https://github.com/libsdl-org/SDL_image.git SDL_image
git clone https://github.com/libsdl-org/SDL_ttf.git SDL_ttf
cd SDL_ttf
./external/download.sh
mkdir build
```

Build commands
```
cmake -S . -B build -G Ninja
cmake --build build
```

For MacOS
```
cmake -S . -B build \
    -G Ninja \
    -DCMAKE_OSX_ARCHITECTURES=arm64
```