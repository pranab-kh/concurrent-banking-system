# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/appbank_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/appbank_autogen.dir/ParseCache.txt"
  "appbank_autogen"
  )
endif()
