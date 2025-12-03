cmake_minimum_required(VERSION 3.25)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()
endif()

project(grayworld VERSION 1.0.2 LANGUAGES CXX)

option(BUILD_AVS_LIB "Build library for AviSynth+" ON)
option(BUILD_VS_LIB "Build library for VapourSynth" ON)

message(STATUS "Build library for AviSynth - ${BUILD_AVS_LIB}")
message(STATUS "Build library for VapourSynth - ${BUILD_VS_LIB}")

add_library(${PROJECT_NAME} MODULE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_c.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_sse2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_avx2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_avx512.cpp"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)

if (BUILD_AVS_LIB)
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs_sse2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs_avx2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs_avx512.cpp"
    )

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    find_package(AvisynthPlus REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE AvisynthPlus::headers)
endif()

if (BUILD_VS_LIB)
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs_sse2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs_avx2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs_avx512.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/VCL2/instrset_detect.cpp"
    )

    if (NOT WIN32)
        target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include/vapoursynth)
    endif()
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_avx2.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2;-mfma>")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_avx512.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX512>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx512f;-mavx512bw;-mavx512dq;-mavx512vl;-mavx512cd;-mfma>")

if (BUILD_AVS_LIB)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs_avx2.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2;-mfma>")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/avs/grayworld_avs_avx512.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX512>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx512f;-mavx512bw;-mavx512dq;-mavx512vl;-mavx512cd;-mfma>")
endif()

if (BUILD_VS_LIB)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs_avx2.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2;-mfma>")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/vs/grayworld_vs_avx512.cpp" PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX512>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx512f;-mavx512bw;-mavx512dq;-mavx512vl;-mavx512cd;-mfma>")
endif()

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL Debug)
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL Release)
        target_compile_definitions(${PROJECT_NAME} PRIVATE RELEASE_BUILD)
    endif()

    if (NOT MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:-s>)
    endif()

    message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<BOOL:${MSVC}>:/EHsc>")

if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_compile_options(${PROJECT_NAME} PRIVATE "/fp:precise")
endif()

if(MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
elseif(UNIX)
    string(TOLOWER "${PROJECT_NAME}" FILE_NAME_OUTPUT)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${FILE_NAME_OUTPUT}.${PROJECT_VERSION}")
endif()

if (WIN32)
    string(REGEX MATCH "^([0-9.]+)" CORE_VERSION_STRING "${PROJECT_VERSION}")
    if(NOT CORE_VERSION_STRING)
        message(WARNING "Could not parse core version from '${PROJECT_VERSION}'. Defaulting to 0.0.0.")
        set(CORE_VERSION_STRING "0.0.0")
    endif()

    string(REPLACE "." ";" VERSION_NUMERIC_PARTS_LIST "${CORE_VERSION_STRING}")
    list(LENGTH VERSION_NUMERIC_PARTS_LIST NUM_PARTS)

    set(VERSION_MAJOR 0)
    set(VERSION_MINOR 0)
    set(VERSION_PATCH 0)
    set(VERSION_BUILD 0)

    if(NUM_PARTS GREATER_EQUAL 1)
        list(GET VERSION_NUMERIC_PARTS_LIST 0 VERSION_MAJOR)
    endif()

    if(NUM_PARTS GREATER_EQUAL 2)
        list(GET VERSION_NUMERIC_PARTS_LIST 1 VERSION_MINOR)
    endif()

    if(NUM_PARTS GREATER_EQUAL 3)
        list(GET VERSION_NUMERIC_PARTS_LIST 2 VERSION_PATCH)
    endif()

    if(NUM_PARTS GREATER_EQUAL 4)
        list(GET VERSION_NUMERIC_PARTS_LIST 3 VERSION_BUILD)
    endif()

    if(PROJECT_VERSION MATCHES "-")
        set(PROJECT_IS_PRERELEASE 1)
    else()
        set(PROJECT_IS_PRERELEASE 0)
    endif()

    if(NOT CMAKE_CONFIGURATION_TYPES)
        if(CMAKE_BUILD_TYPE MATCHES "^Debug$")
            set(PROJECT_DEBUG_BUILD_VALUE 1)
        else()
            set(PROJECT_DEBUG_BUILD_VALUE 0)
        endif()
    else()
        set(PROJECT_DEBUG_BUILD_VALUE -1)
    endif()

    set(PROJECT_DEBUG_BUILD ${PROJECT_DEBUG_BUILD_VALUE})

    set(FILE_DESCRIPTION "Adjust white balance using LAB gray world algorithm")
    set(INTERNAL_NAME "${PROJECT_NAME}")
    set(ORIGINAL_FILENAME "${PROJECT_NAME}.dll")
    set(PRODUCT_NAME "${PROJECT_NAME}")
    set(PROJECT_VERSION_STRING_FULL "${PROJECT_VERSION}")

    set(RC_FILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc") # Output to build dir

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.rc.in"
        "${RC_FILE_OUT}"
        @ONLY
    )

    target_sources(${PROJECT_NAME} PRIVATE "${RC_FILE_OUT}")

    if(MSVC)
        set_source_files_properties("${RC_FILE_OUT}" PROPERTIES
            VS_RESOURCE_GENERATOR "RC")
    endif()
endif()

if(UNIX)
    include(GNUInstallDirs)

    if (BUILD_AVS_LIB)
        INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
    endif()

    if (BUILD_VS_LIB)
        INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/vapoursynth")
    endif()

    # uninstall target
    if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
    endif()
endif()
