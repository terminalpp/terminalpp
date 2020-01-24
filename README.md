# Terminal++

![Windows build](https://img.shields.io/github/workflow/status/terminalpp/tpp/windows-build?logo=windows&logoColor=white&style=flat-square)
![Linux build](https://img.shields.io/github/workflow/status/terminalpp/tpp/linux-build?logo=linux&logoColor=white&style=flat-square)
![Windows MSI](https://img.shields.io/github/workflow/status/terminalpp/tpp/windows-packages?label=msi&logo=windows&logoColor=white&style=flat-square)
![Linux DEB/RPM](https://img.shields.io/github/workflow/status/terminalpp/tpp/linux-packages?label=deb%2Frpm&logo=linux&logoColor=white&style=flat-square)
![Snapcraft](https://img.shields.io/github/workflow/status/terminalpp/tpp/snapcraft-build?label=snap&logo=snapcraft&logoColor=white&style=flat-square)

![Codacy grade](https://img.shields.io/codacy/grade/fd4f07b095634b9d90bbb9edb11fc12c?logo=codacy&style=flat-square)
![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/terminalpp/tpp?logo=LGTM&style=flat-square)


[![Coverage Status](https://coveralls.io/repos/github/terminalpp/tpp/badge.svg?branch=master)](https://coveralls.io/github/terminalpp/tpp?branch=master)
[![terminalpp](https://snapcraft.io//terminalpp/badge.svg)](https://snapcraft.io/terminalpp)

This is the main development repository for the `terminal++` and its suppport repositories. 

# Releases

> Please note that `terminalpp` is in an  alpha stage and there may (and will be) rough edges. That said, it has been used by a few people as their daily driver with only minor issues. Note that backwards compatibility with previous versions is not guaranteed before release 1.0, although it should mostly work from version 0.5 up.

Major & minor releases are released via github where the binary packages for supported platforms can be downloaded. Github actions provide binaries for every successful push.

See the file `CHANGELOG.md` for details about all released versions. 

## Snaps

On each release, new snap version is released to the *edge* channel. The corresponding branch is `release-edge`, i.e. any new push to the branch will automatically upload new version to snap store.

> In the future other channels will also be supported. 

## Windows Store

> Release in Windows store is planned, but has not happened yet.

# Building From Sources

`cmake` is used to build the terminal and related projects as well as to orchestrate the generation of installation packages. First the related projects must be fetched, i.e. :

- `tpp-bypass` - containing the Windows ConPTY bypass directly to `WSL`
- `ropen` - a simple app that transfers remote files through the terminal connection so that they can be opened locally
- `website` - `terminalpp` website

> To clone them all, the `scripts/setup-repos.sh` script can be executed (works with `bash` and `cmd.exe`), which uses the `https` connection. In case `ssh` keys are used the `scripts/setup-repos-ssh.sh` should be executed instead.

## Prerequisites

`terminalpp` has very minimal dependencies. On Windows latest [Visual Studio](https://visualstudio.microsoft.com) must be installed (or [Visual Studio Code](https://code.microsoft.com) with `c++` support) and [cmake](https://cmake.org), while the packagres required on Linux can be automatically installed via the `scripts/setup-linux.sh`.

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

> Since `cpack` is not able to create multiple packages during a single build, multiple conditional builds must be executed if packages for the `bypass` and `ropen` are needed as well. 

To build installation packages for `ropen` and `bypass`, the project must be reconfigured with the `-DPACKAGE_INSTALL=tpp-ropen` or `-DCPACKAGE_INSTALL=tpp-bypass` respectively. 

> Installation packages are also regularly built every time push is made to the repository. These can be downloaded via the Github actions artifacts.

### Manual installation

Once the package to be creates has been selected via `-DPACKAGE_INSTALL` (or left default), the default `install` target can be used to install the respective applications (Linux only):

    sudo cmake --build . --target install

