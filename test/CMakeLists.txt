aux_source_directory(./ SRC_LIST)
add_executable(test test.c)
target_link_libraries(test qlc)
