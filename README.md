# Build

## Windows
- vcpkg install gperf sqlite3 openssl
- clone [`td`](https://github.com/tdlib/td) to `<path_to_td>`
- `cd <path_to_td>`
- 
```sh
build_td.bat
```
- For some reason, cpp runtime setting is ignored. Since reading cmake scripts is harmful for human health, use 
```sh
force_static_runtime_td.ps1 <path_to_td>
```
to override .vcxproj runtime libraries.
- open `<path_to_td>\TDlib.sln` in VS and build `Release`.
- right-click `INSTALL` target and build it too.
- `cd` here and 
```sh
premake5 vs2022 --tdpath=<path_to_td>
```

## Arch
- vcpkg:
```sh
sudo pacman -Syu base-devel git curl zip unzip tar cmake ninja
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install gperf zlib openssl
```
- Install deps
```sh
pacman -S clang cmake gperf php
```
- Build tdlib WITHOUT LTO using https://tdlib.github.io/td/build.html?language=C%2B%2B
- Build tg_eye
```sh
# premake5 is dying -> override cpp standard
premake5 gmake2 && sed -i 's/-std=c++20/-std=c++23/g' ./build/app.make  && make -C build config=release
```
- 