# Terminal++

[![Windows build](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/windows-build?logo=windows&logoColor=white&style=flat-square&label=native)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Awindows-build)
[![Windows build QT](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/windows-build-qt?logo=windows&logoColor=white&style=flat-square&label=qt)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Awindows-build-qt)
[![Linux build](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/linux-build?logo=linux&logoColor=white&style=flat-square&label=native)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Alinux-build)
[![Linux build QT](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/linux-build-qt?logo=linux&logoColor=white&style=flat-square&label=qt)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Alinux-build-qt)
[![macOs build QT](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/macos-build-qt?logo=apple&logoColor=white&style=flat-square&label=qt)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Amacos-build-qt)
[![packages](https://img.shields.io/github/workflow/status/terminalpp/terminalpp/packages?label=packages&logo=buffer&logoColor=white&style=flat-square)](https://github.com/terminalpp/terminalpp/actions?query=workflow%3Apackages)

[![Codacy grade](https://img.shields.io/codacy/grade/fd4f07b095634b9d90bbb9edb11fc12c?logo=codacy&style=flat-square)](https://app.codacy.com/manual/zduka/terminalpp)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/terminalpp/terminalpp?logo=LGTM&style=flat-square)](https://lgtm.com/projects/g/terminalpp/terminalpp?mode=list)
[![Coverage Status](https://coveralls.io/repos/github/terminalpp/tpp/badge.svg?branch=master)](https://coveralls.io/github/terminalpp/terminalpp?branch=master)


> Please note that `terminalpp` is in an alpha stage and there may (and will) be rough edges. That said, it has been used by a few people as their daily driver with only minor issues. If you encounter a problem, please file an issue!

This is the main development repository for the `terminalpp` and its suppport repositories. For more details about how to install `terminalpp` on your machine please visit the [homepage](https://terminalpp.com). This readme provides information on how to build the repository from source only. 

# Supported Platforms

Platform | Renderer | Notes
---------|----------|---------------
Windows  | Native   | uses DirectWrite
Linux    | Native   | uses X11
macOS    | QT       | limited testing as I do not have real Apple computer

# Building From Sources

`cmake` is used to build the terminal and related projects as well as to orchestrate the generation of installation packages. Following sections describe the build process on the different platforms:

> A good way to start is to look at the CI configurations in `.github/workflows` folder where build steps for each supported platform are detailed. 

## Windows 

The latest Windows 10 stable version is always supported. `terminalpp` may run on older Windows 10 versions since Fall 2018 (first ConPTY release). Visual Studio 2019 for C++ and Win32 apps must be installed. 

Build process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release 

For details about how to build the `tpp-bypass` app, refer to the Linux instructions below as bypass is built inside the WSL it is intended for.

## Linux

Tested on latest and LTS versions of Ubuntu. Before building the prerequisite packages must be installed via the `setup-linux.sh` script, i.e.:

    bash scripts/setup-linux.sh

Then build the application using the following commands:

    mkdir -p build/release
    cd build/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8
    cmake --build .

## macOS

macOS Catalina 10.15 and above is supported (due to lack of `std::filesystem` in previous releases). On macOS native rendere is not available and QT must be used instead. Before installation, `brew` must be available. To install prerequisites, run the setup script, i.e: 

    bash scripts/setup-macos.sh

Then build the application using the following commands:

    mkdir -p build/release
    cd build/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release
    cmake --build . 

If QT is not found, try adding adding the following to the cmake command `-DCMAKE_PREFIX_PATH=/usr/local/opt/qt`.

# Building Installation Packages

Use the `packages` target and `-DINSTALL=xxx` `cmake` configuration option to determine which packages should be built (`xxx` can be `terminalpp` (default), `ropen`, or `tpp-bypass`). 

Depending on the availability of the packaging tools (WiX, MakeAppx.exe, rpmbuild, snapcraft, etc.) the respective packages will be created in the `packages` directory inside the build. 

> For more details see the github action `packages`.

## Manual installation

Once the package to be created has been selected via `-DINSTALL=xxx`, the default `install` target can be used to install the respective applications (Linux only):

    sudo cmake --build . --target install



