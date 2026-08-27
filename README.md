# PochaccoClock

Prerequesites
```
git clone https://github.com/libsdl-org/SDL.git vendored/SDL
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