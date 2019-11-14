# Audio library

set(INCROOT ${CMAKE_CURRENT_SOURCE_DIR}/include/Audio/)
set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/)

set(SRC
	${INCROOT}/Audio_Impl.hpp
	${SRCROOT}/Audio.cpp
	${INCROOT}/Audio.hpp
	${SRCROOT}/AudioBase.cpp
	${INCROOT}/AudioBase.hpp
	${INCROOT}/AudioOutput.hpp
	${INCROOT}/AudioStream.hpp
	${SRCROOT}/AudioStream.cpp
	${SRCROOT}/AudioStreamBase.cpp
	${SRCROOT}/AudioStreamBase.hpp
	${SRCROOT}/AudioStreamMa.cpp
	${SRCROOT}/AudioStreamMa.hpp
	${SRCROOT}/AudioStreamMp3.cpp
	${SRCROOT}/AudioStreamMp3.hpp
	${SRCROOT}/AudioStreamOgg.cpp
	${SRCROOT}/AudioStreamOgg.hpp
	${SRCROOT}/AudioStreamWav.cpp
	${SRCROOT}/AudioStreamWav.hpp
	${SRCROOT}/DSP.cpp
	${INCROOT}/DSP.hpp
	${SRCROOT}/Sample.cpp
	${INCROOT}/Sample.hpp
)

# Platform specific source files
if(WIN32)
	SET(PLATFORM_SRC
		${SRCROOT}/Platform/AudioOutput_WASAPI.cpp
	)
	# Platform specific source group
	source_group("Source Files\\Platform" FILES ${PLATFORM_SRC})
else()
	SET(PLATFORM_SRC
		${SRCROOT}/Platform/AudioOutput_SDL.cpp
	)
endif(WIN32)

set(AUDIO_SRC ${SRC} ${PLATFORM_SRC})

# Compiler stuff
enable_cpp14()
enable_precompiled_headers("${AUDIO_SRC}" src/stdafx.cpp)

add_library(Audio ${AUDIO_SRC} src/stdafx.cpp)

target_include_directories(Audio PUBLIC
	include/
	include/Audio
	src
)

target_link_libraries(Audio Shared)

# Dependencies

## Find dependencies roots
get_property(MA_ROOT GLOBAL PROPERTY MA_ROOT)
get_property(MINIMP3_ROOT GLOBAL PROPERTY MINIMP3_ROOT)
get_property(SOUNDTOUCH_ROOT GLOBAL PROPERTY SOUNDTOUCH_ROOT)

## Add dependencies headers
target_include_directories(Audio PUBLIC SYSTEM
	${SOUNDTOUCH_ROOT}/include
	${MINIMP3_ROOT}
	${MA_ROOT}
)

## Add dependencies to linkage
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
include(FindLibraries)

target_link_libraries(Audio minimp3)
target_link_libraries(Audio soundtouch)
target_link_libraries(Audio ${OGG_LIBRARIES})
target_link_libraries(Audio ${VORBIS_LIBRARIES})
