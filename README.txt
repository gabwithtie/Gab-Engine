INSTALLATION/COMPILATION:
1. Clone the repo.
2. Open the folder in VS20XX.
3. Make sure vcpkg is install integrated and cmake is enabled and working.
4.1 Configure CMake.
(follow steps below if it refuses to pull bgfx.cmake)
`
git clone https://github.com/bkaradzic/bgfx.cmake.git
`


4.2. Open a terminal on Ext/bgfx.cmake and run:
`
git submodule init
git submodule update
cmake -S. -Bcmake-build # $CMakeOptions
cmake --build cmake-build
`
4.3 Re Configure CMake.

5. Build.
6. Run.
