file(GLOB sources *.cpp ${CMAKE_SOURCE_DIR}/include/*.h)

add_library(${CMAKE_PROJECT_NAME} STATIC ${sources})

target_include_directories(
        ${CMAKE_PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry>
)

# install the library
install(
        TARGETS ${CMAKE_PROJECT_NAME}
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
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PagedGeometry FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE)

# --- Threading support (still needed for GCC even with C++11)
set(CMAKE_THREAD_PREFER_PTHREAD YES)
find_package(Threads REQUIRED)
target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        Threads::Threads
)

target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${OGRE_LIBRARIES}
)
target_include_directories(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${OGRE_INCLUDE_DIRS}
)
