mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=../tdlib -DCMAKE_TOOLCHAIN_FILE:FILEPATH=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" -DVCPKG_TARGET_TRIPLET=x64-windows-static ..
@REM cmake --build . --parallel --config Release --target install
