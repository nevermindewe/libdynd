PREREQUISITES
=============

You may want to build this library together with its Python
bindings. The instructions for this can be found here:

    https://github.com/ContinuumIO/dynd-python/blob/master/BUILD_INSTALL.md

This library requires a C++98 or C++11 compiler. On Windows, Visual
Studio 2010 is the minimum supported compiler. On Mac OS X, clang
is the recommended compiler. On Linux, gcc 4.6.1 and 4.7.0 have
been tested.

 * CMake >= 2.8.11

The following libraries/projects are included with the code:

 * Google Test 1.6 (included in project)

BUILD/INSTALL INSTRUCTIONS
==========================

CMake is the only supported build system for this library. This
may expand in the future, but for the time being this is the
only one which will be kept up to date.

  ```
  ~ $ cd libdynd
  ~/libdynd $ mkdir build
  ~/libdynd $ cd build
  ~/libdynd/build $ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
  <...>
  ~/libdynd/build $ make
  <...>
  ~/libdynd/build $ sudo make install
  ```

