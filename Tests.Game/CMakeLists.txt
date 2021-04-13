# Game/Graphics Test Project

set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(PCHROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# Find files used for project
file(GLOB SRC "${SRCROOT}/*.cpp" "${SRCROOT}/*.hpp")
source_group("Sources" FILES ${SRC})

set(TESTS_GAME_SRC ${SRC} ${INC})

set(PCH_SRC ${PCHROOT}/stdafx.cpp)
set(PCH_INC ${PCHROOT}/stdafx.h)
set(PCH_FILES ${PCH_SRC} ${PCH_INC})
source_group("" FILES ${PCH_FILES})

# Compiler stuff
enable_precompiled_headers("${TESTS_GAME_SRC}" ${PCH_SRC})

add_executable(Tests.Game ${TESTS_GAME_SRC} ${PCH_FILES})
target_compile_features(Tests.Game PUBLIC cxx_std_17)
set_output_postfixes(Tests.Game)
target_include_directories(Tests.Game PRIVATE
    ${SRCROOT}
    ${PCHROOT}
)
target_compile_definitions(Tests.Game PRIVATE
    SDL_MAIN_HANDLED # Because SDL rename our main to replace it by it's own
)

# Dependencies
target_link_libraries(Tests.Game Shared)
target_link_libraries(Tests.Game Graphics)
target_link_libraries(Tests.Game Audio)
target_link_libraries(Tests.Game Beatmap)
target_link_libraries(Tests.Game GUI)
target_link_libraries(Tests.Game Tests)
