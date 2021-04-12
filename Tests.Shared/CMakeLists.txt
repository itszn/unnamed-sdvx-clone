# Main Project

set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Find files used for project
file(GLOB SRC "${SRCROOT}/*.cpp" "${SRCROOT}/*.hpp")
source_group("Sources" FILES ${SRC})

set(MAIN_SRC ${SRC})

add_executable(Tests.Shared ${MAIN_SRC})
target_compile_features(Tests.Shared PUBLIC cxx_std_17)
target_include_directories(Tests.Shared PRIVATE
    ${SRCROOT}
)
set_output_postfixes(Tests.Shared)

# Dependencies
target_link_libraries(Tests.Shared Shared)
target_link_libraries(Tests.Shared Tests)
