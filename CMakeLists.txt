#
# Copyright (C) 2011-14 Mark Wiebe, DyND Developers
# BSD 2-Clause License, see LICENSE.txt
#

# Travis-CI has 2.8.7, 2.8.11 or higher is recommended
cmake_minimum_required(VERSION 2.8.7)
project(libdynd)

find_package(CUDA)

set(LIB_SUFFIX "" CACHE STRING
    "Typically an empty string or 64. Controls installation to lib or lib64")

# -DDYND_ELWISE_MAX, the maximum number of arguments that the elwise function
#   in libdynd will support
SET(DYND_ELWISE_MAX 4 CACHE STRING
    "Build a libdynd library with an elwise function that supports the specified maximum number of arguments")
# TODO: Check that the preprocessor can handle this number

# Only add these options if this is the top level CMakeLists.txt
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
################################################
# Some options configurable from the CMAKE command execution
#
# -DDYND_SHARED_LIB=ON/OFF, whether to build a shared or a static library.
    if(WIN32)
        option(DYND_SHARED_LIB
            "Build a libdynd shared library instead of a static library"
            OFF)
    else()
        option(DYND_SHARED_LIB
            "Build a libdynd shared library instead of a static library"
            ON)
    endif()
# -DDYND_CUDA=ON/OFF, whether to build libdynd with or without the CUDA Toolkit
    option(DYND_CUDA
        "Build a libdynd library with CUDA"
        OFF) # While CUDA support is in development, we default to OFF.
# -DDYND_FFTW=ON/OFF, whether to build libdynd with or without FFTW
    option(DYND_FFTW
        "Build a libdynd library with FFTW"
        OFF)
#
# -DDYND_INSTALL_LIB=ON/OFF, whether to install libdynd into the
#   CMAKE_INSTALL_PREFIX. Its main purpose is to allow dynd-python and
#   libdynd to be built inside the source tree of another project, like
#   in the libraries/libdynd subdirectory of dynd-python.
    option(DYND_INSTALL_LIB
        "Do installation of the built libdynd library to the CMAKE_INSTALL_PREFIX"
        ON)
# -DUSE_RELATIVE_RPATH=ON/OFF, For OSX, to use the @rpath mechanism
#   for creating a build will be linked against with a relative path.
#   Build dynd-python with -DUSE_RELATIVE_PATH=ON as well, and look in
#   the CMakeLists.txt for dynd-python to see how to use this in other
#   projects.
    if(APPLE)
        option(USE_RELATIVE_RPATH
            "OSX: Set the install name of libdynd to '@rpath'."
            OFF)
    endif()
# -DDYND_BUILD_TESTS=ON/OFF, whether to build the googletest unit tests.
    option(DYND_BUILD_TESTS
        "Build the googletest unit tests for libdynd."
        ON)
#
################################################
endif()

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(CMAKE_VERBOSE_MAKEFILE 1)

# Embedded libraries
add_subdirectory(thirdparty/cephes)
add_subdirectory(thirdparty/datetime)
include_directories(thirdparty/datetime/include)

set(DYND_LINK_LIBS cephes datetime)

if(WIN32)
    # Treat warnings as errors (-WX does this)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -WX -EHsc")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 16)
            message(FATAL_ERROR "Only MSVC 2010 (Version 16.0) and later are supported by LibDyND. Found version ${CMAKE_CXX_COMPILER_VERSION}.")
        endif ()
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fomit-frame-pointer -fstrict-aliasing -fPIC -Wall -Wextra -Werror -Wno-missing-field-initializers")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # The '-fmax-errors' flag was first supported in gcc 4.6
        if (NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.6")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=20")
        endif()
        # Enable C++11 features for gcc >= 4.7 when CUDA support is disabled
        if (NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.7" AND
                NOT ${DYND_CUDA})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        endif()
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=20")
        # Apple has done this weird thing where its Clang reports a different
        # version than the official Clang.
        # TODO: someone more OS X-savvy will have to fine-tune this.
        if (APPLE OR "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "3.2")
            # Enable C++11 features when CUDA support is disabled
            if (NOT ${DYND_CUDA})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
            endif()
        else()
            # Enable C++11 features when CUDA support is disabled
            if (NOT ${DYND_CUDA})
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
            endif()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdocumentation")
        endif()
    endif()
endif()

# Get the git revision
include(GetGitRevisionDescriptionDyND)
get_git_head_revision("${CMAKE_CURRENT_SOURCE_DIR}" GIT_REFSPEC DYND_GIT_SHA1)
git_describe("${CMAKE_CURRENT_SOURCE_DIR}" DYND_VERSION_STRING
             --dirty --always --match "v*")
message(STATUS "DyND version: ${DYND_VERSION_STRING}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dynd/git_version.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/dynd/git_version.cpp" @ONLY)

# Extract the version number from the version string
string(REPLACE "v" "" DYND_VERSION "${DYND_VERSION_STRING}")
string(REPLACE "-" ";" DYND_VERSION "${DYND_VERSION}")
list(GET DYND_VERSION 0 "${DYND_VERSION}")

set(libdynd_SRC
    # Preprocessor
    include/dynd/pp/arithmetic.hpp
    include/dynd/pp/arrfunc_util.hpp
    include/dynd/pp/comparison.hpp
    include/dynd/pp/if.hpp
    include/dynd/pp/list.hpp
    include/dynd/pp/logical.hpp
    include/dynd/pp/meta.hpp
    include/dynd/pp/token.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/pp/gen.hpp
    # CodeGen
    src/dynd/codegen/unary_kernel_adapter_codegen_windows_x64.cpp
    src/dynd/codegen/unary_kernel_adapter_codegen_x64_sysvabi.cpp
    src/dynd/codegen/unary_kernel_adapter_codegen_unsupported.cpp
    src/dynd/codegen/binary_kernel_adapter_codegen_windows_x64.cpp
    src/dynd/codegen/binary_kernel_adapter_codegen_x64_sysvabi.cpp
    src/dynd/codegen/binary_kernel_adapter_codegen_unsupported.cpp
    src/dynd/codegen/binary_reduce_kernel_adapter_codegen.cpp
    src/dynd/codegen/codegen_cache.cpp
    include/dynd/codegen/unary_kernel_adapter_codegen.hpp
    include/dynd/codegen/binary_kernel_adapter_codegen.hpp
    include/dynd/codegen/binary_reduce_kernel_adapter_codegen.hpp
    include/dynd/codegen/calling_conventions.hpp
    include/dynd/codegen/codegen_cache.hpp
    # Types
    src/dynd/types/adapt_type.cpp
    src/dynd/types/base_bytes_type.cpp
    src/dynd/types/base_type.cpp
    src/dynd/types/base_expr_type.cpp
    src/dynd/types/base_memory_type.cpp
    src/dynd/types/base_string_type.cpp
    src/dynd/types/base_struct_type.cpp
    src/dynd/types/base_tuple_type.cpp
    src/dynd/types/base_dim_type.cpp
    src/dynd/types/busdate_type.cpp
    src/dynd/types/builtin_type_properties.cpp
    src/dynd/types/bytes_type.cpp
    src/dynd/types/byteswap_type.cpp
    src/dynd/types/categorical_type.cpp
    src/dynd/types/cfixed_dim_type.cpp
    src/dynd/types/char_type.cpp
    src/dynd/types/arrfunc_type.cpp
    src/dynd/types/convert_type.cpp
    src/dynd/types/cstruct_type.cpp
    src/dynd/types/ctuple_type.cpp
    src/dynd/types/cuda_host_type.cpp
    src/dynd/types/cuda_device_type.cpp
    src/dynd/types/datashape_formatter.cpp
    src/dynd/types/datashape_parser.cpp
    src/dynd/types/date_parser.cpp
    src/dynd/types/date_type.cpp
    src/dynd/types/date_util.cpp
    src/dynd/types/datetime_parser.cpp
    src/dynd/types/datetime_type.cpp
    src/dynd/types/datetime_util.cpp
    src/dynd/types/dynd_complex.cpp
    src/dynd/types/dynd_float16.cpp
    src/dynd/types/dynd_float128.cpp
    src/dynd/types/dynd_int128.cpp
    src/dynd/types/dynd_uint128.cpp
    src/dynd/types/dim_fragment_type.cpp
    src/dynd/types/ellipsis_dim_type.cpp
    src/dynd/types/expr_type.cpp
    src/dynd/types/fixed_dim_type.cpp
    src/dynd/types/fixedbytes_type.cpp
    src/dynd/types/fixedstring_type.cpp
    src/dynd/types/funcproto_type.cpp
    src/dynd/types/groupby_type.cpp
    src/dynd/types/json_type.cpp
    src/dynd/types/ndarrayarg_type.cpp
    src/dynd/types/option_type.cpp
    src/dynd/types/pointer_type.cpp
    src/dynd/types/property_type.cpp
    src/dynd/types/static_type_instances.cpp
    src/dynd/types/strided_dim_type.cpp
    src/dynd/types/string_type.cpp
    src/dynd/types/struct_type.cpp
    src/dynd/types/time_parser.cpp
    src/dynd/types/time_type.cpp
    src/dynd/types/time_util.cpp
    src/dynd/types/tuple_type.cpp
    src/dynd/types/type_alignment.cpp
    src/dynd/types/type_id.cpp
    src/dynd/types/type_pattern_match.cpp
    src/dynd/types/type_substitute.cpp
    src/dynd/types/type_type.cpp
    src/dynd/types/typevar_dim_type.cpp
    src/dynd/types/typevar_type.cpp
    src/dynd/types/unary_expr_type.cpp
    src/dynd/types/var_dim_type.cpp
    src/dynd/types/view_type.cpp
    src/dynd/types/void_pointer_type.cpp
    include/dynd/types/adapt_type.hpp
    include/dynd/types/base_bytes_type.hpp
    include/dynd/types/base_type.hpp
    include/dynd/types/base_memory_type.hpp
    include/dynd/types/base_expr_type.hpp
    include/dynd/types/base_string_type.hpp
    include/dynd/types/base_struct_type.hpp
    include/dynd/types/base_tuple_type.hpp
    include/dynd/types/base_dim_type.hpp
    include/dynd/types/busdate_type.hpp
    include/dynd/types/builtin_type_properties.hpp
    include/dynd/types/bytes_type.hpp
    include/dynd/types/byteswap_type.hpp
    include/dynd/types/categorical_type.hpp
    include/dynd/types/char_type.hpp
    include/dynd/types/arrfunc_type.hpp
    include/dynd/types/convert_type.hpp
    include/dynd/types/ctuple_type.hpp
    include/dynd/types/cuda_host_type.hpp
    include/dynd/types/cuda_device_type.hpp
    include/dynd/types/datashape_formatter.hpp
    include/dynd/types/datashape_parser.hpp
    include/dynd/types/date_parser.hpp
    include/dynd/types/date_type.hpp
    include/dynd/types/date_util.hpp
    include/dynd/types/datetime_parser.hpp
    include/dynd/types/datetime_type.hpp
    include/dynd/types/datetime_util.hpp
    include/dynd/types/dim_fragment_type.hpp
    include/dynd/types/ellipsis_dim_type.hpp
    include/dynd/types/funcproto_type.hpp
    include/dynd/types/type_type.hpp
    include/dynd/types/dynd_complex.hpp
    include/dynd/types/dynd_float16.hpp
    include/dynd/types/dynd_float128.hpp
    include/dynd/types/dynd_int128.hpp
    include/dynd/types/dynd_uint128.hpp
    include/dynd/types/expr_type.hpp
    include/dynd/types/fixed_dim_type.hpp
    include/dynd/types/property_type.hpp
    include/dynd/types/static_type_instances.hpp
    include/dynd/types/type_alignment.hpp
    include/dynd/types/cfixed_dim_type.hpp
    include/dynd/types/fixedbytes_type.hpp
    include/dynd/types/fixedstring_type.hpp
    include/dynd/types/cstruct_type.hpp
    include/dynd/types/groupby_type.hpp
    include/dynd/types/json_type.hpp
    include/dynd/types/ndarrayarg_type.hpp
    include/dynd/types/option_type.hpp
    include/dynd/types/pointer_type.hpp
    include/dynd/types/strided_dim_type.hpp
    include/dynd/types/string_type.hpp
    include/dynd/types/struct_type.hpp
    include/dynd/types/cstruct_type.hpp
    include/dynd/types/time_parser.hpp
    include/dynd/types/time_type.hpp
    include/dynd/types/time_util.hpp
    include/dynd/types/tuple_type.hpp
    include/dynd/types/type_id.hpp
    include/dynd/types/type_pattern_match.hpp
    include/dynd/types/type_substitute.hpp
    include/dynd/types/typevar_dim_type.hpp
    include/dynd/types/typevar_type.hpp
    include/dynd/types/unary_expr_type.hpp
    include/dynd/types/var_dim_type.hpp
    include/dynd/types/view_type.hpp
    include/dynd/types/void_pointer_type.hpp
    # Eval
    src/dynd/eval/eval_context.cpp
    src/dynd/eval/eval_elwise_vm.cpp
    src/dynd/eval/eval_engine.cpp
    src/dynd/eval/elwise_reduce_eval.cpp
    src/dynd/eval/groupby_elwise_reduce_eval.cpp
    src/dynd/eval/unary_elwise_eval.cpp
    include/dynd/eval/eval_context.hpp
    include/dynd/eval/eval_elwise_vm.hpp
    include/dynd/eval/eval_engine.hpp
    include/dynd/eval/elwise_reduce_eval.hpp
    include/dynd/eval/groupby_elwise_reduce_eval.hpp
    include/dynd/eval/unary_elwise_eval.hpp
    # Func
    src/dynd/func/arrfunc.cpp
    src/dynd/func/callable.cpp
    src/dynd/func/copy_arrfunc.cpp
    src/dynd/func/chain_arrfunc.cpp
    src/dynd/func/elwise_gfunc.cpp
    src/dynd/func/elwise_reduce_gfunc.cpp
    src/dynd/func/lift_arrfunc.cpp
    src/dynd/func/lift_reduction_arrfunc.cpp
    src/dynd/func/rolling_arrfunc.cpp
    src/dynd/func/take_arrfunc.cpp
    include/dynd/func/arrfunc.hpp
    include/dynd/func/callable.hpp
    include/dynd/func/call_callable.hpp
    include/dynd/func/copy_arrfunc.hpp
    include/dynd/func/chain_arrfunc.hpp
    include/dynd/func/elwise.hpp
    include/dynd/func/elwise_common.hpp
    include/dynd/func/elwise_callrefres.hpp
    include/dynd/func/elwise_callretres.hpp
    include/dynd/func/elwise_funcrefres.hpp
    include/dynd/func/elwise_funcretres.hpp
    include/dynd/func/elwise_methrefres.hpp
    include/dynd/func/elwise_methretres.hpp
    include/dynd/func/elwise_gfunc.hpp
    include/dynd/func/elwise_reduce_gfunc.hpp
    include/dynd/func/functor_arrfunc.hpp
    include/dynd/func/make_callable.hpp
    include/dynd/func/math_arrfunc.hpp
    include/dynd/func/lift_arrfunc.hpp
    include/dynd/func/lift_reduction_arrfunc.hpp
    include/dynd/func/rolling_arrfunc.hpp
    include/dynd/func/take_arrfunc.hpp
    # Iter
    src/dynd/iter/string_iter.cpp
    include/dynd/iter/string_iter.hpp
    # Kernels
    src/dynd/kernels/assignment_kernels.cpp
    src/dynd/kernels/var_dim_assignment_kernels.cpp
    src/dynd/kernels/buffered_binary_kernels.cpp
    src/dynd/kernels/bytes_assignment_kernels.cpp
    src/dynd/kernels/byteswap_kernels.cpp
    src/dynd/kernels/ckernel_common_functions.cpp
    src/dynd/kernels/comparison_kernels.cpp
    src/dynd/kernels/date_assignment_kernels.cpp
    src/dynd/kernels/date_adapter_kernels.cpp
    src/dynd/kernels/datetime_assignment_kernels.cpp
    src/dynd/kernels/datetime_adapter_kernels.cpp
    src/dynd/kernels/date_expr_kernels.cpp
    src/dynd/kernels/elwise_expr_kernels.cpp
    src/dynd/kernels/expr_kernel_generator.cpp
    src/dynd/kernels/expr_kernels.cpp
    src/dynd/kernels/expression_assignment_kernels.cpp
    src/dynd/kernels/expression_comparison_kernels.cpp
    src/dynd/kernels/make_lifted_ckernel.cpp
    src/dynd/kernels/make_lifted_reduction_ckernel.cpp
    src/dynd/kernels/option_assignment_kernels.cpp
    src/dynd/kernels/option_kernels.cpp
    src/dynd/kernels/reduction_kernels.cpp
    src/dynd/kernels/string_assignment_kernels.cpp
    src/dynd/kernels/string_algorithm_kernels.cpp
    src/dynd/kernels/string_numeric_assignment_kernels.cpp
    src/dynd/kernels/string_comparison_kernels.cpp
    src/dynd/kernels/struct_assignment_kernels.cpp
    src/dynd/kernels/struct_comparison_kernels.cpp
    src/dynd/kernels/time_assignment_kernels.cpp
    src/dynd/kernels/kernels_for_disassembly.cpp
    src/dynd/kernels/single_assigner_builtin.hpp
    src/dynd/kernels/single_assigner_builtin_int128.hpp
    src/dynd/kernels/single_assigner_builtin_uint128.hpp
    src/dynd/kernels/single_assigner_builtin_float16.hpp
    src/dynd/kernels/single_assigner_builtin_float128.hpp
    src/dynd/kernels/single_comparer_builtin.hpp
    include/dynd/kernels/assignment_kernels.hpp
    include/dynd/kernels/var_dim_assignment_kernels.hpp
    include/dynd/kernels/buffered_binary_kernels.hpp
    include/dynd/kernels/bytes_assignment_kernels.hpp
    include/dynd/kernels/byteswap_kernels.hpp
    include/dynd/kernels/ckernel_builder.hpp
    include/dynd/kernels/ckernel_common_functions.hpp
    include/dynd/kernels/ckernel_prefix.hpp
    include/dynd/kernels/comparison_kernels.hpp
    include/dynd/kernels/date_assignment_kernels.hpp
    include/dynd/kernels/date_adapter_kernels.hpp
    include/dynd/kernels/datetime_assignment_kernels.hpp
    include/dynd/kernels/datetime_adapter_kernels.hpp
    include/dynd/kernels/date_expr_kernels.hpp
    include/dynd/kernels/elwise_expr_kernels.hpp
    include/dynd/kernels/expr_kernels.hpp
    include/dynd/kernels/expr_kernel_generator.hpp
    include/dynd/kernels/expression_assignment_kernels.hpp
    include/dynd/kernels/expression_comparison_kernels.hpp
    include/dynd/kernels/make_lifted_ckernel.hpp
    include/dynd/kernels/make_lifted_reduction_ckernel.hpp
    include/dynd/kernels/option_assignment_kernels.hpp
    include/dynd/kernels/option_kernels.hpp
    include/dynd/kernels/reduction_kernels.hpp
    include/dynd/kernels/string_assignment_kernels.hpp
    include/dynd/kernels/string_algorithm_kernels.hpp
    include/dynd/kernels/string_numeric_assignment_kernels.hpp
    include/dynd/kernels/string_comparison_kernels.hpp
    include/dynd/kernels/struct_assignment_kernels.hpp
    include/dynd/kernels/struct_comparison_kernels.hpp
    include/dynd/kernels/time_assignment_kernels.hpp
    # MemBlock
    src/dynd/memblock/memory_block.cpp
    src/dynd/memblock/executable_memory_block_windows_x64.cpp
    src/dynd/memblock/executable_memory_block_darwin_x64.cpp
    src/dynd/memblock/executable_memory_block_linux_x64.cpp
    src/dynd/memblock/external_memory_block.cpp
    src/dynd/memblock/fixed_size_pod_memory_block.cpp
    src/dynd/memblock/memmap_memory_block.cpp
    src/dynd/memblock/pod_memory_block.cpp
    src/dynd/memblock/array_memory_block.cpp
    src/dynd/memblock/objectarray_memory_block.cpp
    src/dynd/memblock/zeroinit_memory_block.cpp
    include/dynd/memblock/memory_block.hpp
    include/dynd/memblock/executable_memory_block.hpp
    include/dynd/memblock/external_memory_block.hpp
    include/dynd/memblock/fixed_size_pod_memory_block.hpp
    include/dynd/memblock/memmap_memory_block.hpp
    include/dynd/memblock/pod_memory_block.hpp
    include/dynd/memblock/array_memory_block.hpp
    include/dynd/memblock/objectarray_memory_block.hpp
    include/dynd/memblock/zeroinit_memory_block.hpp
    # VM
    src/dynd/vm/elwise_program.cpp
    src/dynd/vm/register_allocation.cpp
    include/dynd/vm/elwise_program.hpp
    include/dynd/vm/register_allocation.hpp
    # Main
    src/dynd/arithmetic_op.cpp
    src/dynd/array.cpp
    src/dynd/fft.cpp
    src/dynd/array_range.cpp
    src/dynd/config.cpp
    src/dynd/dim_iter.cpp
    src/dynd/type.cpp
    src/dynd/typed_data_assign.cpp
    src/dynd/type_promotion.cpp
    src/dynd/exceptions.cpp
    src/dynd/git_version.cpp.in # Included here for ease of editing in IDEs
    ${CMAKE_CURRENT_BINARY_DIR}/src/dynd/git_version.cpp
    src/dynd/json_formatter.cpp
    src/dynd/json_parser.cpp
    src/dynd/lowlevel_api.cpp
    src/dynd/parser_util.cpp
    src/dynd/random.cpp
    src/dynd/shape_tools.cpp
    src/dynd/special.cpp
    src/dynd/string.cpp
    src/dynd/string_encodings.cpp
    src/dynd/view.cpp
    include/dynd/array.hpp
    include/dynd/array_range.hpp
    include/dynd/array_iter.hpp
    include/dynd/arrmeta_holder.hpp
    include/dynd/atomic_refcount.hpp
    include/dynd/auxiliary_data.hpp
    include/dynd/buffer_storage.hpp
    include/dynd/config.hpp
    include/dynd/cmake_config.hpp.in # Included here for ease of editing in IDEs
    ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/cmake_config.hpp
    include/dynd/cuda_config.hpp
    include/dynd/cling_all.hpp
    include/dynd/diagnostics.hpp
    include/dynd/dim_iter.hpp
    include/dynd/dynd_math.hpp
    include/dynd/ensure_immutable_contig.hpp
    include/dynd/random.hpp
    include/dynd/fft.hpp
    include/dynd/type.hpp
    include/dynd/typed_data_assign.hpp
    include/dynd/type_promotion.hpp
    include/dynd/exceptions.hpp
    include/dynd/fpstatus.hpp
    include/dynd/json_formatter.hpp
    include/dynd/json_parser.hpp
    include/dynd/irange.hpp
    include/dynd/lowlevel_api.hpp
    include/dynd/parser_util.hpp
    include/dynd/platform_definitions.hpp
    include/dynd/shortvector.hpp
    include/dynd/shape_tools.hpp
    include/dynd/special.hpp
    include/dynd/string.hpp
    include/dynd/string_encodings.hpp
    include/dynd/view.hpp
    )

include_directories(
    include
    thirdparty/utf8/source
    )

source_group("Main Source" REGULAR_EXPRESSION "src/dynd/.*cpp")
source_group("Main Headers" REGULAR_EXPRESSION "include/dynd/.*hpp")
source_group("CodeGen Source" REGULAR_EXPRESSION "src/dynd/codegen/.*cpp")
source_group("CodeGen Headers" REGULAR_EXPRESSION "include/dynd/codegen/.*hpp")
source_group("Types Source" REGULAR_EXPRESSION "src/dynd/types/.*cpp")
source_group("Types Headers" REGULAR_EXPRESSION "include/dynd/types/.*hpp")
source_group("Eval Source" REGULAR_EXPRESSION "src/dynd/eval/.*cpp")
source_group("Eval Headers" REGULAR_EXPRESSION "include/dynd/eval/.*hpp")
source_group("Func Source" REGULAR_EXPRESSION "src/dynd/func/.*cpp")
source_group("Func Headers" REGULAR_EXPRESSION "include/dynd/func/.*hpp")
source_group("Iter Source" REGULAR_EXPRESSION "src/dynd/iter/.*cpp")
source_group("Iter Headers" REGULAR_EXPRESSION "include/dynd/iter/.*hpp")
source_group("Kernels Source" REGULAR_EXPRESSION "src/dynd/kernels/.*cpp")
source_group("Kernels Headers" REGULAR_EXPRESSION "include/dynd/kernels/.*hpp")
source_group("MemBlock Source" REGULAR_EXPRESSION "src/dynd/memblock/.*cpp")
source_group("MemBlock Headers" REGULAR_EXPRESSION "include/dynd/memblock/.*hpp")
source_group("PP Headers" REGULAR_EXPRESSION "include/dynd/pp/.*hpp")
source_group("VM Source" REGULAR_EXPRESSION "src/dynd/vm/.*cpp")
source_group("VM Headers" REGULAR_EXPRESSION "include/dynd/vm/.*hpp")
source_group("Internal Headers" REGULAR_EXPRESSION "src/dynd/.*hpp")

if (DYND_CUDA)
    # Replace some source files with their CUDA versions
    list(REMOVE_ITEM libdynd_SRC
        src/dynd/types/dynd_complex.cpp
        src/dynd/types/dynd_float16.cpp
        src/dynd/types/dynd_float128.cpp
        src/dynd/types/dynd_int128.cpp
        src/dynd/types/dynd_uint128.cpp
        src/dynd/kernels/assignment_kernels.cpp
        )        
    list(APPEND libdynd_SRC
        src/dynd/kernels/assignment_kernels.cu
        )

    # Disable warnings about "controlling expression is constant"
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-Xcudafe;--diag_suppress=boolean_controlling_expr_is_constant)

#    set(CUDA_SEPARABLE_COMPILATION ON)
endif()

if (DYND_FFTW)
    find_path(FFTW_PATH fftw3.h)
    include_directories(${FFTW_PATH})
    set(DYND_LINK_LIBS ${DYND_LINK_LIBS} fftw3 fftw3f)
endif()

if ((NOT DYND_SHARED_LIB) AND (DYND_INSTALL_LIB))
    # If we're making an installable static library,
    # include the sublibraries source directly because
    # including static libraries in other static libraries
    # is a hassle
    get_directory_property(cephes_SRC
                           DIRECTORY "thirdparty/cephes"
                           DEFINITION cephes_SRC)
    get_directory_property(datetime_SRC
                           DIRECTORY "thirdparty/datetime"
                           DEFINITION datetime_SRC)
    set(libdynd_SRC
        ${libdynd_SRC}
        ${cephes_SRC}
        ${datetime_SRC}
        )
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if (DYND_SHARED_LIB)
    if (DYND_CUDA)
        set(CUDA_OPTIONS "-arch=compute_20")
        if (NOT WIN32)
            set(CUDA_OPTIONS ${CUDA_OPTIONS}  "-Xcompiler" "-fPIC")
        endif()
        cuda_add_library(libdynd SHARED ${libdynd_SRC} OPTIONS ${CUDA_OPTIONS})
    else()
        add_library(libdynd SHARED ${libdynd_SRC})
    endif()
    set_target_properties(libdynd
        PROPERTIES
        # For now, during development, embed the full version string
        # to reduce the chance of accidentally linking the wrong one.
        OUTPUT_NAME "dynd_${DYND_VERSION_STRING}"
        PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}"
        )
else()
    if (DYND_CUDA)
        set(CUDA_OPTIONS "-arch=compute_20")
        if (NOT WIN32)
            set(CUDA_OPTIONS ${CUDA_OPTIONS}  "-Xcompiler" "-fPIC")
        endif()
        cuda_add_library(libdynd STATIC ${libdynd_SRC} OPTIONS ${CUDA_OPTIONS})
    else()
        add_library(libdynd STATIC ${libdynd_SRC})
    endif()
    set_target_properties(libdynd
        PROPERTIES
        OUTPUT_NAME "dynd"
        PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}"
        )
endif()

# Add preprocessor definitions from CMake
configure_file("include/dynd/cmake_config.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/dynd/cmake_config.hpp")

# Special build targets that generate part of the library
execute_process(COMMAND ${CMAKE_COMMAND}
                -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/pp)
add_executable(ppgen include/dynd/pp/gen.cpp)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/pp/gen.hpp
        DEPENDS include/dynd/pp/gen.cpp
        COMMAND ppgen 8 3 ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/pp)
# Make libdynd depend on ppgen_run to make sure gen.hpp is there when it builds
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/include/dynd/pp/gen.hpp
                            PROPERTIES GENERATED true)
add_dependencies(libdynd ppgen)

# Copy the headers from cephes that need to be installed with libdynd
file(COPY thirdparty/cephes/rename.h
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/cephes)
file(COPY thirdparty/cephes/protos.h
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/dynd/cephes)

if(WIN32)
    set_target_properties(libdynd
        PROPERTIES
        PREFIX "lib"
        )
elseif(APPLE)
    if(USE_RELATIVE_RPATH)
        message(STATUS "Setting the libdynd install name to @rpath")
        # MW: The INSTALL_NAME_DIR and rpath stuff is a bit confusing,
        #     hopefully someone who really knows this stuff can fix it
        #     up one day.
        set_target_properties(libdynd
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH ON
            INSTALL_NAME_DIR "@rpath"
            )
    endif()
endif()

if (DYND_SHARED_LIB OR (NOT DYND_INSTALL_LIB))
    # If we're not making an installable static library,
    # link the sublibraries normally
    target_link_libraries(libdynd ${DYND_LINK_LIBS})
endif()

# add_subdirectory(basic_kernels)
if(DYND_BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(examples)

# Create a libdynd-config script
get_property(dynd_library_prefix TARGET libdynd PROPERTY PREFIX)
get_property(dynd_output_name TARGET libdynd PROPERTY OUTPUT_NAME)
set(DYND_LINK_FLAG "-l${dynd_output_name}")
if (DYND_SHARED_LIB)
    if ("${CMAKE_IMPORT_LIBRARY_SUFFIX}" STREQUAL "")
        set(DYND_LIB_FILE "${dynd_library_prefix}${dynd_output_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
    else()
        set(DYND_LIB_FILE "${dynd_library_prefix}${dynd_output_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}")
    endif()
else()
    set(DYND_LIB_FILE "${dynd_library_prefix}${dynd_output_name}${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()
if(WIN32)
    if (DYND_SHARED_LIB)
        set(DYND_STATIC_LIB_DIR "")
    else()
        set(DYND_STATIC_LIB_DIR "\\static")
    endif()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/libdynd-config.bat.in"
        "${CMAKE_CURRENT_BINARY_DIR}/libdynd-config.bat" @ONLY)
else()
    if (DYND_SHARED_LIB)
        set(DYND_STATIC_LIB_DIR "")
    else()
        set(DYND_STATIC_LIB_DIR "/static")
    endif()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/libdynd-config.in"
        "${CMAKE_CURRENT_BINARY_DIR}/libdynd-config" @ONLY)
endif()

# If requested, install the .lib/.so/.dll file and headers to the install prefix
if (DYND_INSTALL_LIB)
    # Install the libdynd binary
    install(TARGETS libdynd
        RUNTIME DESTINATION lib${LIB_SUFFIX}
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX}/static)
    # Install the libdynd headers
    install(DIRECTORY "include/dynd"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/dynd"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
    # Install the libdynd-config script
    if(WIN32)
        install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libdynd-config.bat"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    else()
        install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libdynd-config"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    endif()
endif()

