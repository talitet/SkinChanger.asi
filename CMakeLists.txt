cmake_minimum_required(VERSION 3.15)

project(SkinChanger)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(PSDK_EXAMPLES OFF)
set(PSDK_INSTALL  OFF)
set(PSDK_GTASA    ON)

include(FetchContent)

FetchContent_Declare(
	rakhook
	GIT_REPOSITORY https://github.com/imring/RakHook.git
	GIT_TAG master
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rakhook
)
FetchContent_Declare(
    plugin-sdk
    GIT_REPOSITORY https://github.com/imring/plugin-sdk.git
    GIT_TAG 81b18b744b64db3058f48344c144591c9030205b
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pluginsdk
)
FetchContent_Declare(
    SAMPAPI
    GIT_REPOSITORY https://github.com/BlastHackNet/SAMP-API.git
    GIT_TAG 6d4db99ab41f19d1a6a7c6cd48f5878bd1e14b62
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sampapi
)
FetchContent_MakeAvailable(rakhook plugin-sdk sampapi)

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE
	${${PROJECT_NAME}_LIST}
	src/main.cpp
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}
	SUFFIX ".asi"
)

if(NOT "${DXSDK_DIR}" STREQUAL "")
    target_include_directories( ${PROJECT_NAME} PUBLIC ${DXSDK_DIR}/Include)
    target_link_directories(${PROJECT_NAME} PUBLIC ${DXSDK_DIR}/Lib/x86)
elseif(NOT "$ENV{DXSDK_DIR}" STREQUAL "")
    target_include_directories( ${PROJECT_NAME} PUBLIC $ENV{DXSDK_DIR}/Include)
    target_link_directories( ${PROJECT_NAME} PUBLIC $ENV{DXSDK_DIR}/Lib/x86)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC rakhook pluginsdk::gtasa sampapi)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
