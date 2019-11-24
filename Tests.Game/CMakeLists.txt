# Game/Graphics Test Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp")

# Compiler stuff
enable_precompiled_headers("${Main_src}" stdafx.cpp)

add_executable(Tests.Game ${Main_src})
target_compile_features(Tests.Game PUBLIC cxx_std_14)
set_output_postfixes(Tests.Game)
target_include_directories(Tests.Game PRIVATE .)

if(MSVC)
	# SEH and Debug
	target_compile_options(Tests.Game PUBLIC /EHa /Zi)
else()
	target_compile_options(Tests.Game PUBLIC -rdynamic)
endif()	

# Dependencies
target_link_libraries(Tests.Game Shared)
target_link_libraries(Tests.Game Graphics)
target_link_libraries(Tests.Game Audio)
target_link_libraries(Tests.Game Beatmap)
target_link_libraries(Tests.Game GUI)
target_link_libraries(Tests.Game Tests)
