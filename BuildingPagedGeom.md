# Building PagedGeometry {#building-pageom}

PagedGeometry uses [CMake](https://cmake.org/) as its build system on all supported platforms.
CMake is a popular cross-platform build configurator which generates a native build system for your platform and toolset.
This guide will explain to you how to use CMake to build PagedGeometry from source.

Preparing the build environment
------------------------------------

You should now create a build directory for PagedGeometry somewhere outside
PagedGeometry's sources. This is the directory where CMake will create the
build system for your chosen platform and compiler, and this is also
where the PagedGeometry libraries will be compiled. This way, the PagedGeometry source
dir stays clean, and you can have multiple build directories (e.g. for Android and for Linux) all
working from the same PagedGeometry source.

Getting dependencies
--------------------

PagedGeometry doesn't have any dependencies besides OGRE itself.
The simplest and recommended way of obtaining OGRE is downloading a prebuilt SDK from [ogre3d.org download page](https://www.ogre3d.org/download/sdk/sdk-ogre).

Running CMake
-------------

Now start the program cmake-gui by either typing the name in a console
or selecting it from the start menu. In the field *Where is the source
code* enter the path to the PagedGeometry source directory (the directory which
contains this file). In the field *Where to build the binaries* enter
the path to the build directory you created.
Hit *Configure*. A dialogue will appear asking you to select a generator.


Click *Finish*. CMake will now gather some information about your
build environment and try to locate the dependencies. It will then show
a list of build options.
- To proceed at all, you must set `OGRE_DIR` to point to 'CMake' subdirectory of your OGRE SDK.
- Unless you've already seen them before, tick the `PAGEDGEOMETRY_BUILD_SAMPLES` checkbox.

Once you are set up, hit
*Configure* again and then click on *Generate*. CMake will then create
the build system for you.

Building
--------

Go to your chosen build directory. CMake has generated a build system for
you which you will now use to build PagedGeometry. If you are using Visual Studio,
you should find the file PagedGeometry.sln. Open it and compile the target
*BUILD_ALL*. Similarly you will find an Xcode project to build PagedGeometry
on MacOS.


