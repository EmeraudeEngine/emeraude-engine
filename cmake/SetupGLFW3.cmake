message("Configuring GLFW3 framework as sub-project ...")

set(BUILD_SHARED_LIBS Off)
set(GLFW_BUILD_EXAMPLES Off)
set(GLFW_BUILD_TESTS Off)
set(GLFW_BUILD_DOCS Off)
set(GLFW_INSTALL Off)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw EXCLUDE_FROM_ALL)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
