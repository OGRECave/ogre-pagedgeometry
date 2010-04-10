
INCLUDE( ${CMAKE_ROOT}/Modules/FindDoxygen.cmake)
CONFIGURE_FILE(${PagedGeometry_SOURCE_DIR}/docs/Doxyfile.in ${PagedGeometry_SOURCE_DIR}/docs/Doxyfile @ONLY IMMEDIATE)
add_custom_target (PagedGeometryDocumentation doxygen ${PagedGeometry_SOURCE_DIR}/docs/Doxyfile WORKING_DIRECTORY ${PagedGeometry_SOURCE_DIR}/docs/ COMMENT "Generating documentation")
