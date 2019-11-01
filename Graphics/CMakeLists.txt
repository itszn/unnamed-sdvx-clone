# Graphics library

get_property(GLEW_ROOT GLOBAL PROPERTY GLEW_ROOT)

# Find files used for project
file(GLOB Graphics_src "src/*.cpp" "include/Graphics/*.hpp")

# Platform specific source files
if(WIN32)
	include_directories("src/Windows")
	file(GLOB Platform_src "src/Windows/*.cpp" "src/Windows/*.hpp")
	# Platform specific source group
	source_group("Source Files\\Windows" FILES ${Platform_src})
else(WIN32)
	include_directories("src/Linux")
	file(GLOB Platform_src "src/Linux/*.cpp")
endif(WIN32)

file(GLOB Graphics_src ${Graphics_src} ${Platform_src})

# Compiler stuff
enable_cpp14()
enable_precompiled_headers("${Graphics_src}" src/stdafx.cpp)

# Look for static included GLEW before other directories
include_directories(include include/Graphics src)
add_library(Graphics ${Graphics_src})

# Public include paths for library
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${GLEW_ROOT}/include)

# Dependencies
target_link_libraries(Graphics Shared)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
include(FindLibraries)
target_link_libraries(Graphics ${FREETYPE_LIBRARIES})
target_link_libraries(Graphics ${SDL2_LIBRARY})
target_link_libraries(Graphics ${JPEG_LIBRARIES})
target_link_libraries(Graphics ${PNG_LIBRARIES})
target_link_libraries(Graphics GLEW)
if(UNIX)
	target_link_libraries(Graphics pthread) 
	target_link_libraries(Graphics dl)
	if(APPLE)
		find_library(OPEN_GL OpenGL)
		target_link_libraries(Graphics ${OPEN_GL})
	else(APPLE)
		target_link_libraries(Graphics GL)
		find_package(X11 REQUIRED)
		include_directories(${X11_INCLUDE_DIR})
	endif(APPLE)
endif(UNIX)

include_directories(${FREETYPE_INCLUDE_DIRS})
find_package(Freetype REQUIRED)
