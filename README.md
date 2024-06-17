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
- Make to have an [AUR helper](https://wiki.archlinux.org/title/AUR_helpers).
- Install deps
```sh
pacman -S clang cmake gperf php
```
- Build tdlib:
```sh
yay -S libtd
```
