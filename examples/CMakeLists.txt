
# PagedGeometry includes
include_directories(${PagedGeometry_SOURCE_DIR}/include)
include_directories(${PagedGeometry_BINARY_DIR}/include)

# external includes
include_directories(${Ogre_INCLUDE_DIRS})
include_directories(${Ogre-Terrain_INCLUDE_DIRS})
include_directories(${Ogre-Paging_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

IF(WIN32)
	# add DX path for checks
	set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} $ENV{DXSDK_DIR}/Include)
		
	# check for libs and include files we want to use
	# XXX TODO: fix directx lib check
	#set(CMAKE_REQUIRED_FLAGS "-L$ENV{DXSDK_DIR}/Lib/x86")
	#CHECK_LIBRARY_EXISTS(dinput8 DirectInput8Create "" HAVE_DXINPUT_LIBS)
	#if(NOT HAVE_DXINPUT_LIBS)
	#	message(FATAL_ERROR "could not link against DirectX input, please check of you have the required libraries installed")
	#endif()
	CHECK_INCLUDE_FILE(dinput.h HAVE_DXINPUT_INCLUDES)
	if(NOT HAVE_DXINPUT_INCLUDES)
		message(FATAL_ERROR "could not find the DirectX includes. Please install them.")
	endif()
	CHECK_INCLUDE_FILE(windows.h HAVE_WINDOWS_INCLUDES)
	if(NOT HAVE_WINDOWS_INCLUDES)
		message(FATAL_ERROR "could not find the windows platform includes. Please install them.")
	endif()

	set(DirectX_INCLUDE_DIRS "$ENV{DXSDK_DIR}/Include" CACHE PATH "The DirectX include path to use")
	set(DirectX_LIBRARY_DIRS "$ENV{DXSDK_DIR}/Lib/x86" CACHE PATH "The DirectX lib path to use")
	set(DirectX_LIBRARIES    "dinput8.lib;dxguid.lib" CACHE PATH "The DirectX lib path to use")
	
ELSEIF(UNIX)
	include_directories(${GTK_INCLUDE_DIRS})
	set(OS_LIBS "X11")
	# check for libs and include files we want to use
	CHECK_LIBRARY_EXISTS(X11 XOpenDisplay "" HAVE_X11_LIBS)
	if(NOT HAVE_X11_LIBS)
		message(FATAL_ERROR "could not link against X11, please check of you have the required libraries installed")
	endif()
	CHECK_INCLUDE_FILE(X11/keysym.h HAVE_X11_INCLUDES)
	if(NOT HAVE_X11_INCLUDES)
		message(FATAL_ERROR "could not find the X11 includes. Please install them.")
	endif()
ELSEIF(APPLE)
	include_directories(${GTK_INCLUDE_DIRS})
	set(OS_LIBS "X11")
	# check for libs and include files we want to use
	CHECK_LIBRARY_EXISTS(X11 XOpenDisplay "" HAVE_X11_LIBS)
	if(NOT HAVE_X11_LIBS)
		message(FATAL_ERROR "could not link against X11, please check of you have the required libraries installed")
	endif()
	CHECK_INCLUDE_FILE(X11/keysym.h HAVE_X11_INCLUDES)
	if(NOT HAVE_X11_INCLUDES)
		message(FATAL_ERROR "could not find the X11 includes. Please install them.")
	endif()
ENDIF(WIN32)


macro(example_app NAME)
	include_directories(${DirectX_INCLUDE_DIRS})
	link_directories   (${DirectX_LIBRARY_DIRS})
	add_executable(${NAME} WIN32 ${NAME}.cpp)
	target_link_libraries(${NAME} PagedGeometry ${OS_LIBS} ${Ogre_LIBRARIES} ${Ogre-Terrain_LIBRARIES} ${Ogre-Paging_LIBRARIES} ${Ois_LIBRARIES} ${DirectX_LIBRARIES})
endmacro(example_app)


example_app(Example1)
example_app(Example2)
example_app(Example3)
example_app(Example4)
example_app(Example5)
example_app(Example6)
example_app(Example7)
example_app(Example8)
example_app(Example9)
example_app(Example10)
example_app(Example11)

