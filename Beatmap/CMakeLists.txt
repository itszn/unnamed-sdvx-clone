# Beatmap library

set(INCROOT ${CMAKE_CURRENT_SOURCE_DIR}/include/Beatmap/)
set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(PCHROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

file(GLOB INC "${INCROOT}/*.hpp")
source_group("Public Headers" FILES ${INC})

file(GLOB SRC "${SRCROOT}/*.cpp" "${SRCROOT}/*.hpp")
source_group("Sources" FILES ${SRC})

# Find files used for project
set(BEATMAP_SRC ${SRC} ${INC})

set(PCH_SRC ${PCHROOT}/stdafx.cpp)
set(PCH_INC ${PCHROOT}/stdafx.h)
set(PCH_FILES ${PCH_SRC} ${PCH_INC})
source_group("" FILES ${PCH_FILES})

# Compiler stuff
enable_cpp14()
enable_precompiled_headers("${BEATMAP_SRC}" ${PCH_SRC})

add_library(Beatmap ${BEATMAP_SRC} ${PCH_FILES})

# Public include paths for library
target_include_directories(Beatmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(Beatmap PRIVATE
	${INCROOT}
	${SRCROOT}
	${PCHROOT}
)

# Dependencies

## Local Dependencies
target_link_libraries(Beatmap Shared)

## 3rd Party dependencies
get_property(SQLITE3_ROOT GLOBAL PROPERTY SQLITE3_ROOT)
target_include_directories(Beatmap PRIVATE SYSTEM ${SQLITE3_ROOT})
target_link_libraries(Beatmap sqlite3)
