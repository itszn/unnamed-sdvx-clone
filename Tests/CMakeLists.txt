# Shared Library

set(INCROOT ${CMAKE_CURRENT_SOURCE_DIR}/include/Tests/)
set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(PCHROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# Find files used for project
file(GLOB INC "${INCROOT}/*.hpp")
source_group("Public Headers" FILES ${INC})

file(GLOB SRC "${SRCROOT}/*.cpp" "${SRCROOT}/*.hpp")
source_group("Sources" FILES ${SRC})

set(TESTS_SRC ${SRC} ${INC})

set(PCH_SRC ${PCHROOT}/stdafx.cpp)
set(PCH_INC ${PCHROOT}/stdafx.h)
set(PCH_FILES ${PCH_SRC} ${PCH_INC})
source_group("" FILES ${PCH_FILES})

# Compiler stuff
enable_precompiled_headers("${TESTS_SRC}" ${PCH_SRC})

add_library(Tests ${TESTS_SRC} ${PCH_FILES})
target_compile_features(Tests PUBLIC cxx_std_17)
# Public include paths for library
target_include_directories(Tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(Tests PRIVATE
    ${SRCROOT}
    ${INCROOT}
    ${PCHROOT}
)
if(MSVC)
    target_compile_options(Tests PUBLIC /EHa) # SEH
endif()

target_link_libraries(Tests Shared)

