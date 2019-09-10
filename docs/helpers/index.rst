.. highlight: cpp

C++ Helper Classes
==================

The `helpers` namespace contains a header-only implementation of various helpful classes that could be used across many projects. Functionality in Windows (`msvc`) and Linux (`gcc`, `clang`) is tested regularly, MacOS should mostly work too. 

The main file ``helpers.h`` implements the very basics, i.e. :

- ``STR`` macro which is a syntactic sugar that allows concatenating strings via streams::

    STR("Hello wodld" << 67 << std::endl);

