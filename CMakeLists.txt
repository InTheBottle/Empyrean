cmake_minimum_required(VERSION 3.24)

# -------- Project ----------
project(
	PathsOfEmpyrean
	VERSION 1.0.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
	Skyrim64Path
	"[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition;installed path]"
	ABSOLUTE CACHE
)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/Bethesda-CMakeModules/Modules"
)
include(SKSEPlugin)
set(SKSE_USE_XBYAK ON)
add_compile_definitions(SKSE_SUPPORT_XBYAK)

set(SKSE_COMMONLIBSSE_PATH "External/CommonLibSSE-NG")
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL "" FORCE)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
		"${PROJECT_NAME}"
		PRIVATE
			JsonCpp::JsonCpp
)

target_compile_definitions(
		"${PROJECT_NAME}"
		PRIVATE
			NOMINMAX
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

# ---- Compile Papyrus Scripts ----
include(Papyrus)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Data/Source/Scripts")
	file(GLOB ${PROJECT_NAME}_SOURCES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"Data/Source/Scripts/*.psc"
	)

	Papyrus_Add(
		"Papyrus"
		GAME ${Skyrim64Path}
		IMPORTS ${CMAKE_CURRENT_SOURCE_DIR}/Data/Source/Scripts
		SOURCES ${${PROJECT_NAME}_SOURCES}
		OPTIMIZE ANONYMIZE
	)
else()
	message(STATUS "No scripts to compile found.")
endif()