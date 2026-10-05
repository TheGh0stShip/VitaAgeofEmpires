# Verify every object in a build directory's static archives against the Vita
# ELF ABI, after a build. The configure-time gate (VerifyVitaAbi.cmake) only
# inspects a compiler probe and zlib; this covers the project's own objects.
#
#   cmake -DAOE_ABI_DIR=build/vita -DAOE_ABI_READELF=arm-vita-eabi-readelf \
#         -DAOE_ABI_AR=arm-vita-eabi-ar -P cmake/VerifyVitaArchives.cmake
#
# Beyond the configure-gate patterns it requires small enums, matching
# VitaSDK's arm-vita-eabi-gcc default (see docs/VITA_ABI.md).

cmake_minimum_required(VERSION 3.16)

foreach(required AOE_ABI_DIR AOE_ABI_READELF AOE_ABI_AR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Define ${required} (see the header of this script)")
  endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/VerifyVitaAbi.cmake")
set(CMAKE_READELF "${AOE_ABI_READELF}")

file(GLOB archives "${AOE_ABI_DIR}/*.a")
list(SORT archives)
if(NOT archives)
  message(FATAL_ERROR "No static archives found in ${AOE_ABI_DIR}")
endif()

set(work "${AOE_ABI_DIR}/CMakeFiles/aoe_vita_archive_check")
set(object_count 0)
list(LENGTH archives archive_count)
foreach(archive IN LISTS archives)
  get_filename_component(archive_name "${archive}" NAME)
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")
  execute_process(
    COMMAND "${AOE_ABI_AR}" x "${archive}"
    WORKING_DIRECTORY "${work}"
    RESULT_VARIABLE ar_result
    ERROR_VARIABLE ar_error)
  if(NOT ar_result EQUAL 0)
    message(FATAL_ERROR "Could not extract ${archive_name}: ${ar_error}")
  endif()
  file(GLOB members "${work}/*")
  if(NOT members)
    message(FATAL_ERROR "${archive_name} contains no objects")
  endif()
  foreach(member IN LISTS members)
    get_filename_component(member_name "${member}" NAME)
    set(label "${archive_name}(${member_name})")
    aoe_verify_vita_elf("${label}" "${member}")
    execute_process(
      COMMAND "${AOE_ABI_READELF}" -A "${member}"
      OUTPUT_VARIABLE member_attributes
      RESULT_VARIABLE readelf_result)
    if(NOT readelf_result EQUAL 0 OR
       NOT member_attributes MATCHES "Tag_ABI_enum_size:[ \t]+small")
      message(FATAL_ERROR
        "${label} does not use VitaSDK's small enums:\n${member_attributes}")
    endif()
    math(EXPR object_count "${object_count} + 1")
  endforeach()
endforeach()
file(REMOVE_RECURSE "${work}")

message(STATUS
  "Verified Vita ABI attributes of ${object_count} objects in ${archive_count} archives")
