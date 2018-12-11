add_executable (run run.cpp)
target_link_libraries( run lib )

add_executable( MB000 MB000.cpp)
target_link_libraries (MB000 lib gtest)

add_executable(MB001 MB001.cpp)
target_link_libraries (MB001 lib gtest)

add_executable(test000 test000.cpp)
target_link_libraries (test000 gtest)
