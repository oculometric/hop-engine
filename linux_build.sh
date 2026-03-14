mkdir -p bin
cd bin
cmake -S .. --preset x64-release-lin -B . -G Ninja
cmake --build . --target demo --parallel
