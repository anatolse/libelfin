cmake_minimum_required(VERSION 3.13)

add_executable(dump-lines dump-lines.cc)
target_link_libraries(dump-lines dwarf)

add_executable(find-pc find-pc.cc)
target_link_libraries(find-pc dwarf)