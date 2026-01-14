# Find PostgreSQL
#
# Find the PostgreSQL includes and library
#
# This module defines PG_INCLUDE_DIRS, where to find header, etc. PG_LIBRARIES,
# the libraries needed to use PostgreSQL. pg_FOUND, If false, do not try to use
# PostgreSQL.
# pg_lib - The imported target library.

find_package(PostgreSQL)
if(PostgreSQL_FOUND)
  set(PG_LIBRARIES ${PostgreSQL_LIBRARIES})
  set(PG_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIRS})
  message(STATUS "pg inc: " ${PostgreSQL_INCLUDE_DIRS})
  add_library(pg_lib INTERFACE IMPORTED)
  set_target_properties(pg_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${PostgreSQL_INCLUDE_DIRS}")
  # Use PostgreSQL target if available (vcpkg provides this), otherwise use raw libraries
  if(TARGET PostgreSQL::PostgreSQL)
    target_link_libraries(pg_lib INTERFACE PostgreSQL::PostgreSQL)
  else()
    # Handle optimized/debug keywords in library list
    set(_pg_libs "")
    set(_next_is_debug FALSE)
    set(_next_is_optimized FALSE)
    foreach(_lib ${PostgreSQL_LIBRARIES})
      if(_lib STREQUAL "debug")
        set(_next_is_debug TRUE)
      elseif(_lib STREQUAL "optimized")
        set(_next_is_optimized TRUE)
      elseif(_next_is_debug)
        list(APPEND _pg_libs "$<$<CONFIG:Debug>:${_lib}>")
        set(_next_is_debug FALSE)
      elseif(_next_is_optimized)
        list(APPEND _pg_libs "$<$<NOT:$<CONFIG:Debug>>:${_lib}>")
        set(_next_is_optimized FALSE)
      else()
        list(APPEND _pg_libs "${_lib}")
      endif()
    endforeach()
    set_target_properties(pg_lib PROPERTIES INTERFACE_LINK_LIBRARIES "${_pg_libs}")
  endif()
  mark_as_advanced(PG_INCLUDE_DIRS PG_LIBRARIES)
endif(PostgreSQL_FOUND)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pg
                                  DEFAULT_MSG
                                  PG_LIBRARIES
                                  PG_INCLUDE_DIRS)
