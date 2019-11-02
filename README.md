# Terminal++

[![Windows Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/1?label=windows&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=1)
[![Linux Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/2?label=linux&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=2)
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

## Build Scripts

Alternatively the platform specific build scripts can be used. These create the appropriate build directories (`build/debug` and `build/release`), clone all the subprojects, install the dependencies and finallu build all the repositories, generating all installation packages possible at given machine. These scripts are:

- `scripts/build-linux.sh` for Ubuntu (tested on `18.04`)
- `scripts/build-windows.bat` for Windows

# Installing

> Since `cpack` is not able to create multiple packages during a single build, multiple conditional builds must be executed. 

Depending on your OS run either:

    scripts/build-linux.sh

or:
    
    scripts/build-windows.sh

These scripts build the terminal & friends from source and will installation packages if appropriate tools are available on the system (`msi` for Windows, `snap`, `deb` and `rpm` for Linux). 

The packages will be stored in the folder `build/release/packages`. 

## Manual installation

You can use the `-DPACKAGE_INSTALL` `cmake` argument to determine which packages should be created (`terminalpp`, `tpp-ropen` and `tpp-bypass`). You can then install the selected programs via the command:

    sudo cmake --build . --target install

> Note that on Windows there is no manual installation option.
