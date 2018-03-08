cmake_minimum_required(VERSION 3.6)

project(PolarSim)

set(NOCTARE_ENGINE_DIR "${PROJECT_SOURCE_DIR}/../../NoctareEngine")
set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../")

include_directories(
	${PROJECT_SOURCE_DIR}/../include
	${NOCTARE_ENGINE_DIR}/Include
	${NOCTARE_ENGINE_DIR}/ThirdParty
)

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset configurations" FORCE)
endif()

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/../source/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/../include/*.hpp)

add_executable(PolarSim WIN32 ${SOURCE_FILES} ${HEADER_FILES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PolarSim)

if(${WIN32})
	set(DEBUG_LINK_LIBRARIES
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/NoctareEngine.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/SDL2.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/SDL2main.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/SDL2_image.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/SDL2_mixer.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/SDL2_ttf.lib
		debug ${NOCTARE_ENGINE_DIR}/Libraries/debug/glew32d.lib
		debug opengl32.lib
		debug glu32.lib
	)
	set(RELEASE_LINK_LIBRARIES
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/NoctareEngine.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/SDL2.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/SDL2main.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/SDL2_image.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/SDL2_mixer.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/SDL2_ttf.lib
		optimized ${NOCTARE_ENGINE_DIR}/Libraries/release/glew32.lib
		optimized opengl32.lib
		optimized glu32.lib
	)
	set(ALL_LINK_LIBRARIES ${DEBUG_LINK_LIBRARIES} ${RELEASE_LINK_LIBRARIES})
	target_link_libraries(PolarSim ${ALL_LINK_LIBRARIES})
	add_custom_command(TARGET PolarSim PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "../../../NoctareEngine/Binaries/debug/NoctareEngine.dll" "${ROOT_DIR}/development/NoctareEngine.dll"
	)
endif()
