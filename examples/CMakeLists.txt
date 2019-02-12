
# PagedGeometry includes
include_directories(${PagedGeometry_SOURCE_DIR}/include)
include_directories(${PagedGeometry_BINARY_DIR}/include)

# external includes
include_directories(${OGRE_INCLUDE_DIRS})
link_directories   (${OGRE_LIBRARY_DIRS})

macro(example_app NAME)
	add_executable(${NAME} WIN32 ${NAME}.cpp)
	target_link_libraries(${NAME} PagedGeometry ${OGRE_LIBRARIES})
endmacro(example_app)

example_app(Example1)
example_app(Example2)
example_app(Example3)
example_app(Example4)
example_app(Example5)
example_app(Example6)
example_app(Example7)

if(OFF)
# these are still using the TerrainSceneManager from Ogre 1.7
# need porting for Ogre 1.8+
example_app(Example8)
example_app(Example9)
example_app(Example10)
endif()

example_app(Example11)

