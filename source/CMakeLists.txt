file(GLOB sources *.cpp ${PROJECT_SOURCE_DIR}/include/*.h)

add_library(${PROJECT_NAME} STATIC ${sources})

target_include_directories(
        ${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry>
)

# install the library
install(
        TARGETS ${PROJECT_NAME}
        EXPORT PagedGeometryTargets
        RUNTIME LIBRARY ARCHIVE
)

install(
        EXPORT PagedGeometryTargets
        FILE PagedGeometryTargets.cmake
        NAMESPACE PagedGeometry::
        DESTINATION ${CMAKEFILES_INSTALL_DIR}
)

# install the headers: both source and build headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE)
install(DIRECTORY ${PROJECT_BINARY_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE)

# --- Threading support (still needed for GCC even with C++11)
set(CMAKE_THREAD_PREFER_PTHREAD YES)
find_package(Threads REQUIRED)
target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        Threads::Threads
)

target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        ${OGRE_LIBRARIES}
)
target_include_directories(
        ${PROJECT_NAME} PRIVATE
        ${OGRE_INCLUDE_DIRS}
)
