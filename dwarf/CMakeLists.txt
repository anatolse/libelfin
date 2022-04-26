cmake_minimum_required(VERSION 3.13)

set(SRC 
	dwarf.cc 
	cursor.cc
	die.cc 
	value.cc 
	abbrev.cc 
	expr.cc
	rangelist.cc 
	line.cc 
	attrs.cc 
	die_str_map.cc
	elf.cc 
	to_string.cc
)

set(HEADERS
	dwarf++.hh
	data.hh
	internal.hh
	small_vector.hh
	../elf/to_hex.hh
)

add_library(dwarf STATIC ${SRC} ${HEADERS})
target_include_directories(dwarf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dwarf PUBLIC Boost::filesystem )