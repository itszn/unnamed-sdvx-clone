# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp")

include_directories(.)
add_executable(Tests.Shared ${Main_src})
target_compile_features(Tests.Shared PUBLIC cxx_std_14)

set_output_postfixes(Tests.Shared)

if(MSVC)
	# SEH and Debug
	target_compile_options(Tests.Shared PUBLIC /EHa /Zi)
else()
	target_compile_options(Tests.Shared PUBLIC -rdynamic)
endif()

# Dependencies
target_link_libraries(Tests.Shared Shared)
target_link_libraries(Tests.Shared Tests)
