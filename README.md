# Build

## Windows
- cd to the cloned [`td`](https://github.com/tdlib/td)
- run `build_td.bat`
- for some reason, cpp runtime setting is ignored. since reading cmake is harmful for human health, use `force_static_runtime_td.ps1 <path_to_td>` to override .vcxproj runtime libraries.
- open `<path_to_td>\TDlib.sln` and build it.
- go here `premake5 vs2022 --tdpath=<path_to_td>`