# Terminal++


[![Windows build](https://github.com/terminalpp/tpp/workflows/windows-build/badge.svg)](https://github.com/terminalpp/tpp/actions?query=workflow%3Awindows-build)
[![Linux build](https://github.com/terminalpp/tpp/workflows/linux-build/badge.svg)](https://github.com/terminalpp/tpp/actions?query=workflow%3Alinux-build)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/fd4f07b095634b9d90bbb9edb11fc12c)](https://www.codacy.com/manual/zduka/tpp?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=terminalpp/tpp&amp;utm_campaign=Badge_Grade)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/terminalpp/tpp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/terminalpp/tpp/context:cpp)

This is the main development repository for the `terminal++` and its suppport repositories. 

> Please note that `tpp` is in an  alpha stage and there may (and will be) rough edges. That said, it has been used by a few people as their daily driver with only minor issues. Notably the font detection is lacking and unless you have the Iosevka font tpp uses by default the terminal might look very ugly before you edit the settings file. 

# Building From Sources

`cmake` is used to build the terminal and related projects as well as to orchestrate the generation of installation packages. First the related projects must be fetched, i.e. :

- `tpp-bypass` - containing the Windows ConPTY bypass directly to `WSL`
- `ropen` - a simple app that transfers remote files through the terminal connection so that they can be opened locally
- `website` - `tpp` website

> To clone them all, the `scripts/setup-repos.sh` script can be executed (works with `bash` and `cmd.exe`), which uses the `https` connection. In case `ssh` keys are used the `scripts/setup-repos-ssh.sh` should be executed instead.

## Prerequisites

`tpp` has very minimal dependencies. On Windows latest [Visual Studio](https://visualstudio.microsoft.com) must be installed (or [Visual Studio Code](https://code.microsoft.com) with `c++` support) and [cmake](https://cmake.org), while the packagres required on Linux can be automatically installed via the `scripts/setup-linux.sh`.

## Building via CMake

The whole build process on all platforms is automated via `cmake`. The following are examples on how to create release build for the supported platforms:

> A good way to start is to look at the CI configurations in `.github/workflows` folder where build steps for each supported platform are detailed. 

### Windows

    mkdir build
    cd build
    cmake .. 
    cmake --build . --config Release

### Linux

    mkdir -p build/release
    cd build/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8

## Building Installation packages

By default, installation packages for `terminalpp` are created for the given platform and subject to the availability of the package creation tools (`WIX` toolset for Windows, `snapcraft`, and DEB and RPM tools on Linux).

To create them, build the `packages` target:

    cmake --build . --target packages

> Since `cpack` is not able to create multiple packages during a single build, multiple conditional builds must be executed. 

To build installation packages for `ropen` and `bypass`, the project must be reconfigured with the `-DPACKAGE_INSTALL=tpp-ropen` or `-DCPACKAGE_INSTALL=tpp-bypass` respectively. 

### Manual installation

Once the package to be creates has been selected via `-DPACKAGE_INSTALL` (or left default), the default `install` target can be used to install the respective applications (Linux only):

    sudo cmake --build . --target install
