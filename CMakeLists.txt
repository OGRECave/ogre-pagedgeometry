######################################################################
# PagedGeometry BUILD SYSTEM
# Welcome to the CMake build system for PagedGeometry.
# This is the main file where we prepare the general build environment
# and provide build configuration options.
######################################################################
# cmake system for PagedGeometry updated on 2-9-2021 by Edgar{at}AnotherFoxGuy{DOT}com

cmake_minimum_required(VERSION 3.1)

# define the project
project(
        PagedGeometry
        HOMEPAGE_URL https://ogrecave.github.io/ogre-pagedgeometry/
        DESCRIPTION "PagedGeometry is a library that helps you to add grass and trees to your Ogre3D scene"
        VERSION 1.3.0
)

# add some functions we use that are shipped with cmake
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)


# build static libs by default
SET(BUILD_SHARED_LIBS OFF)

SET(CMAKE_USE_RELATIVE_PATHS OFF)


# setup paths
SET(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_DIRECTORY})
SET(CMAKEFILES_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/PagedGeometry")

# fix executable paths for windows
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUNTIME_OUTPUT_DIRECTORY})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY})


# some optimization flags
if (MSVC)
    # add multi processor compilation flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}                 /MP /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP /Zi")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}           /MP /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}   /MP /Od /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}                     /MP /Od /Zi")
    # some general flags
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    include_directories(${DirectX_INCLUDE_DIR})

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}                   /DEBUG /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}               /DEBUG /SUBSYSTEM:WINDOWS /LTCG /OPT:REF")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}         /DEBUG /SUBSYSTEM:WINDOWS /LTCG /OPT:REF")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /SUBSYSTEM:WINDOWS /LTCG /OPT:REF")
elseif ()
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}      -mfpmath=sse -msse2 -mmmx -msse -msse3 -m3dnow -O2 -fomit-frame-pointer -fstrict-aliasing -ffast-math -ftracer")
    set(CMAKE_EXE_LINKER_FLAGS_RelWithDebug "${CMAKE_EXE_LINKER_FLAGS_RelWithDebug} -O0")
endif ()

# some PG build options
set(PAGEDGEOMETRY_BUILD_SAMPLES "FALSE" CACHE BOOL "build the examples")
set(PAGEDGEOMETRY_ALTERNATE_COORDSYSTEM "FALSE" CACHE BOOL "alternate coordinate system, do not use unless you are very sure about it")
set(PAGEDGEOMETRY_USER_DATA "FALSE" CACHE BOOL "ability to attach user data to entities")

# configuration of the config.h
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/include/PagedGeometryConfig.h.in" "${PROJECT_BINARY_DIR}/include/PagedGeometryConfig.h")

# some additional compiler flags
if (NOT WIN32)
    ADD_DEFINITIONS(-Wall -Wno-unused-parameter)
    CHECK_C_COMPILER_FLAG(-Wextra HAVE_W_EXTRA)
    if (HAVE_W_EXTRA)
        ADD_DEFINITIONS(-Wextra)
    endif ()
endif ()

# Set visibility options if available
IF (NOT WIN32)
    CHECK_C_SOURCE_COMPILES("int foo() __attribute__((destructor));
  int main() {return 0;}" HAVE_GCC_DESTRUCTOR)

    CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_SWITCH)
    IF (HAVE_VISIBILITY_SWITCH)
        CHECK_C_SOURCE_COMPILES("int foo() __attribute__((visibility(\"default\")));
  int main() {return 0;}" HAVE_GCC_VISIBILITY)
        IF (HAVE_GCC_VISIBILITY)
            ADD_DEFINITIONS(-fvisibility=hidden -DHAVE_GCC_VISIBILITY)
        ENDIF ()
    ENDIF ()
ENDIF ()

# --- Ogre 3D graphics engine ---
find_package(OGRE REQUIRED CONFIG)

# now add the directories
add_subdirectory(source)

if (PAGEDGEOMETRY_BUILD_SAMPLES)
    add_subdirectory(examples)
endif (PAGEDGEOMETRY_BUILD_SAMPLES)


# doxygen stuff
find_package(Doxygen)
if (DOXYGEN_FOUND)
    message("found doxygen, generating documentation")
    # prepare doxygen configuration file
    configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)

    add_custom_target(
            doc_doxygen
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating documentation with Doxygen."
            VERBATIM
    )

    # cleanup $build/docs on "make clean"
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES docs)

    #install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/api)
    # install man pages into packages, scope is now project root..
    #install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/man/man3 DESTINATION share/man/man3/ )
endif (DOXYGEN_FOUND)

# other doc files
set(DOC_FILES Contributors.txt LICENSE.txt README.md Todo.txt)
install(FILES ${DOC_FILES} DESTINATION TYPE DOC)

# install the PkgConfig file
configure_file("${PROJECT_SOURCE_DIR}/PagedGeometry.pc.in" "${PROJECT_BINARY_DIR}/PagedGeometry.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/PagedGeometry.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

configure_package_config_file(
        PagedGeometryConfig.cmake.in
	${PROJECT_BINARY_DIR}/PagedGeometryConfig.cmake
        INSTALL_DESTINATION ${CMAKEFILES_INSTALL_DIR}
)

write_basic_package_version_file(
	${PROJECT_BINARY_DIR}/PagedGeometryConfigVersion.cmake
        VERSION "${CMAKE_PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
)

install(
	FILES ${PROJECT_BINARY_DIR}/PagedGeometryConfig.cmake
	${PROJECT_BINARY_DIR}/PagedGeometryConfigVersion.cmake
        DESTINATION ${CMAKEFILES_INSTALL_DIR}
)
