.. Terminal++ documentation master file, created by
   sphinx-quickstart on Wed Sep  4 16:21:51 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. toctree::
   :hidden:
   :includehidden:
   :maxdepth: 2
   :caption: Contents:

   helpers/index.rst
   vterm/index.rst

Terminal++ 
==========

Terminal++ (or ``tpp``) is a cross-platform (Windows and Linux for now [#apple]_) terminal emulator. Its main purpose is to support all the "modern" terminal features (such as mouse & clipboard support, etc.) and provide identical and unobtrusive user experience across the supported operating systems. 

.. note::

    For now the Terminal++ is an umbrella project over several projects which together provide its functionality. While it is in an alpha state, for convenience, all these projects are in the same repo, but will be split over time.

Features
--------

- full mouse support (on Windows, due to ConPTY limitations, the *wsl bypass* must be used for mouse support)
- bi-directional clipboard support, full primary and clipboard buffer support in Linux, primary buffer emulation on Windows (i.e. across the application)

.. [#apple] Since I do not use Apple computers, the motivation to do native client on apple is a bit limited for now. However, if X server is installed, the Linux version can be built and used. 
