cmake_minimum_required(VERSION 3.16)

project(d3dgame LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAME_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(GAME_VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

file(GLOB_RECURSE PROJECT_SOURCES
    "${GAME_SOURCE_DIR}/*.cpp"
    "${GAME_SOURCE_DIR}/*.h"
    "${GAME_SOURCE_DIR}/*.hpp"
)

add_executable(d3dgame ${PROJECT_SOURCES})

target_include_directories(d3dgame PRIVATE ${GAME_SOURCE_DIR})

#--------------------------------------------------------------------
# Direct3D11
#--------------------------------------------------------------------
target_link_libraries(d3dgame PRIVATE
  d3d11
  d3dcompiler
  dxgi
  dxguid)

#--------------------------------------------------------------------
# SDL3
#--------------------------------------------------------------------
set(SDL_TEST_LIBRARY OFF)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)

target_link_libraries(d3dgame PRIVATE SDL3-static)
    
add_subdirectory(${GAME_VENDOR_DIR}/SDL)
include_directories(${GAME_VENDOR_DIR}/vendor/SDL/include)

set(CMAKE_SUPPRESS_REGENERATION true)
set(SDLSHADERCROSS_VENDORED ON CACHE INTERNAL "")
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE INTERNAL "")
set(SPIRV_REFLECT_STATIC_LIB ON CACHE INTERNAL "")

#--------------------------------------------------------------------
# GLM
#--------------------------------------------------------------------
add_subdirectory(${GAME_VENDOR_DIR}/glm)
target_link_libraries(d3dgame PRIVATE glm::glm)

#--------------------------------------------------------------------
# STB
#--------------------------------------------------------------------
target_include_directories(d3dgame PRIVATE ${GAME_VENDOR_DIR}/stb)

#--------------------------------------------------------------------
# Folder structuring in Visual Studio
#--------------------------------------------------------------------
if(MSVC_IDE)
    set_target_properties(SDL3-static PROPERTIES FOLDER "Vendor/Engine/SDL3")
    set_target_properties(SDL_uclibc PROPERTIES FOLDER "Vendor/Engine/SDL3")

	foreach(source IN LISTS PROJECT_SOURCES)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" relative_source_path "${source_path}")
		string(REPLACE "/" "\\" source_path_msvc "${relative_source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()
endif()
