add_executable(updater updater.cpp)
target_link_libraries(updater cpr)
target_link_libraries(updater ${LibArchive_LIBRARIES})
target_include_directories(updater SYSTEM PRIVATE ${LibArchive_INCLUDE_DIRS})
