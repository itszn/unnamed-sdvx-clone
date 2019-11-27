# Main Project

# Find files used for project

set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(PCHROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

file(GLOB SRC "${SRCROOT}/*.cpp" "${SRCROOT}/*.hpp")
source_group("Source" FILES ${SRC})

file(GLOB AUDIO_SRC "${SRCROOT}/Audio/*.cpp" "${SRCROOT}/Audio/*.hpp")
source_group("Source\\Audio" FILES ${AUDIO_SRC})

file(GLOB GUI_SRC "${SRCROOT}/GUI/*.cpp" "${SRCROOT}/GUI/*.hpp")
source_group("Source\\GUI" FILES ${GUI_SRC})

file(GLOB NUK_INCLUDE "${SRCROOT}/nuklear/*.h")
source_group("Source\\nuklear" FILES ${NUK_SRC})

set(MAIN_SRC ${SRC} ${AUDIO_SRC} ${GUI_SRC} ${NUK_SRC})


file(GLOB RESOURCE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Resource.rc)
source_group("" FILES ${RESOURCE_SRC})

set(PCH_SRC ${PCHROOT}/stdafx.cpp)
set(PCH_INC ${PCHROOT}/stdafx.h)
set(PCH_FILES ${PCH_SRC} ${PCH_INC})
source_group("" FILES ${PCH_FILES})

# Compiler stuff
enable_precompiled_headers("${MAIN_SRC}" ${PCH_SRC})

include_directories(Audio GUI .)
add_executable(usc-game ${MAIN_SRC} ${RESOURCE_SRC} ${PCH_FILES})
target_include_directories(usc-game PRIVATE
   ${SRCROOT}
   ${SRCROOT}/GUI
   ${SRCROOT}/Audio
)
target_compile_features(usc-game PUBLIC cxx_std_14)
target_compile_definitions(usc-game PRIVATE VERSION_MINOR=${PROJECT_VERSION_MINOR})
target_compile_definitions(usc-game PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(usc-game PRIVATE VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_compile_definitions(usc-game PRIVATE GIT_COMMIT=${GIT_DATE_HASH})

set_output_postfixes(usc-game)

# Target subsystem on windows, set debugging folder
if(MSVC)
   set_target_properties(usc-game PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
   set_target_properties(usc-game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
endif(MSVC)

# Dependencies
target_link_libraries(usc-game Shared)
target_link_libraries(usc-game Graphics)
target_link_libraries(usc-game Audio)
target_link_libraries(usc-game GUI)
target_link_libraries(usc-game Beatmap)
target_link_libraries(usc-game nanovg)
#target_link_libraries(Main nuklear)
target_link_libraries(usc-game lua)
target_link_libraries(usc-game discord-rpc)
target_link_libraries(usc-game cpr)
target_link_libraries(usc-game nlohmann_json)
#target_link_libraries(usc-game archive)

target_link_libraries(usc-game ${SDL2_LIBRARY})
target_link_libraries(usc-game ${LibArchive_LIBRARIES})

