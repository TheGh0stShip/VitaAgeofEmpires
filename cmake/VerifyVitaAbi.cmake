# Fail configuration early when the selected compiler or a dependency does
# not match Vita's actual 32-bit ARM hard-float ABI.

function(aoe_verify_vita_elf label path)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "${label} ABI input does not exist: ${path}")
  endif()

  execute_process(
    COMMAND "${CMAKE_READELF}" -h -A "${path}"
    RESULT_VARIABLE readelf_result
    OUTPUT_VARIABLE elf_attributes
    ERROR_VARIABLE readelf_error)
  if(NOT readelf_result EQUAL 0)
    message(FATAL_ERROR
      "Could not inspect ${label} ABI (${path}): ${readelf_error}")
  endif()

  foreach(required_pattern
      "Class:[ \t]+ELF32"
      "Data:[ \t]+2's complement, little endian"
      "Machine:[ \t]+ARM"
      "Tag_CPU_arch:[ \t]+v7"
      "Tag_CPU_arch_profile:[ \t]+Application"
      "Tag_ABI_VFP_args:[ \t]+VFP registers")
    if(NOT elf_attributes MATCHES "${required_pattern}")
      message(FATAL_ERROR
        "${label} does not satisfy the Vita ABI (${required_pattern}):\n"
        "${elf_attributes}")
    endif()
  endforeach()
endfunction()

function(aoe_verify_vita_abi)
  if(NOT VITA)
    message(FATAL_ERROR "aoe_verify_vita_abi is only valid in a Vita build")
  endif()
  if(NOT CMAKE_READELF)
    message(FATAL_ERROR "The Vita toolchain did not provide CMAKE_READELF")
  endif()

  set(probe_source "${CMAKE_BINARY_DIR}/CMakeFiles/aoe_vita_abi_probe.c")
  set(probe_object "${CMAKE_BINARY_DIR}/CMakeFiles/aoe_vita_abi_probe.o")
  file(WRITE "${probe_source}" [=[
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if !defined(__arm__) || defined(__aarch64__)
#error "PS Vita code must be compiled for 32-bit ARM"
#endif
#if !defined(__ARM_ARCH_7A__)
#error "PS Vita code must be compiled for ARMv7-A"
#endif
#if !defined(__ARM_PCS_VFP)
#error "VitaSDK dependencies and project code must use the hard-float PCS"
#endif
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "PS Vita is little-endian"
#endif

_Static_assert(CHAR_BIT == 8, "Vita uses 8-bit bytes");
_Static_assert(sizeof(int) == 4, "Vita int must be 32-bit");
_Static_assert(sizeof(long) == 4, "Vita long must be 32-bit");
_Static_assert(sizeof(void *) == 4, "Vita pointers must be 32-bit");
_Static_assert(sizeof(size_t) == 4, "Vita size_t must be 32-bit");
_Static_assert(sizeof(uintptr_t) == 4, "Vita uintptr_t must be 32-bit");

int aoe_vita_abi_probe(void) { return 0; }
]=])

  execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -std=c11 -c "${probe_source}" -o "${probe_object}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error)
  if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR
      "Vita compiler failed the ARMv7/ILP32/hard-float probe:\n"
      "${compile_output}${compile_error}")
  endif()

  aoe_verify_vita_elf("Vita compiler probe" "${probe_object}")
  message(STATUS "Verified Vita ABI: little-endian ARMv7-A ELF32, ILP32, hard-float PCS")
endfunction()
