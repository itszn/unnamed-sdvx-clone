# Audio library

# Find miniaudio root
get_property(ma_root GLOBAL PROPERTY MA_ROOT)
get_property(MINIMP3_ROOT GLOBAL PROPERTY MINIMP3_ROOT)

# Find files used for project
file(GLOB Audio_src "src/*.cpp" "include/Audio/*.hpp")

# The soundtouch library used for pitch shifting
file(GLOB soundtouch_src "src/soundtouch/src/*.cpp" "src/soundtouch/src/*.h" "src/soundtouch/include/*.h")

# Platform specific source files
if(WIN32)
	include_directories("src/Windows")
	file(GLOB Platform_src "src/Windows/*.cpp" "src/Windows/*.hpp")
	# Platform specific source group
	source_group("Source Files\\Windows" FILES ${Platform_src})
	source_group("SoundTouch" FILES ${soundtouch_src})
endif(WIN32)

file(GLOB Audio_src ${Audio_src} ${Platform_src})

# Compiler stuff
enable_cpp14()
enable_precompiled_headers("${Audio_src}" src/stdafx.cpp)
precompiled_header_exclude("${soundtouch_src}")

include_directories(
	include/
	include/Audio
	src
	src/soundtouch/src
	src/soundtouch/include
	${MINIMP3_ROOT}
	${ma_root}
	${ma_root}/extras
)
add_library(Audio ${Audio_src} ${soundtouch_src})


# Public include paths for library
target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Dependencies
target_link_libraries(Audio Shared)
target_link_libraries(Audio minimp3)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
include(FindLibraries)
target_link_libraries(Audio ${OGG_LIBRARIES})
target_link_libraries(Audio ${VORBIS_LIBRARIES})
