call scripts\setup-repos.sh
mkdir build
cd build
cmake ..
cmake --build . --clean-first --config Debug
cmake --build . --clean-first --config Release
cmake --build . --target packages