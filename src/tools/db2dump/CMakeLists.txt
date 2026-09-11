# db2dump: convert TrinityCore static DB2 files into SQL for ad-hoc querying.

add_executable(db2dump)

CollectAndAddSourceFiles(
  db2dump
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(db2dump
  PRIVATE
    common)

target_include_directories(db2dump
  PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/server/shared
    ${CMAKE_SOURCE_DIR}/src/server/shared/DataStores
    ${CMAKE_SOURCE_DIR}/src/server/game
    ${CMAKE_SOURCE_DIR}/src/server/game/DataStores
    ${CMAKE_SOURCE_DIR}/src/server/database/Database
    ${CMAKE_SOURCE_DIR}/src/server/database/Database/Implementation)

set_target_properties(db2dump
  PROPERTIES
    COMPILE_WARNING_AS_ERROR ${WITH_WARNINGS_AS_ERRORS}
    FOLDER "tools")

if(WIN32)
  # /EHa lets the SEH per-table guard (db2dump.cpp) coexist with C++ object
  # unwinding in the same translation unit.
  target_compile_options(db2dump PRIVATE /EHa)
endif()

if(UNIX)
  install(TARGETS db2dump DESTINATION bin)
elseif(WIN32)
  install(TARGETS db2dump DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()