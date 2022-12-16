cmake_minimum_required(VERSION 3.14)
project(RenameViaHash CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
	${SOURCES}
	Source.cpp
	)

add_executable(RenameViaHash WIN32 ${SOURCES})

# libbinaryio
add_subdirectory(3rdparty/libbinaryio "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libbinaryio" EXCLUDE_FROM_ALL)
target_include_directories(RenameViaHash PRIVATE ${ROOT} "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libbinaryio" "${ROOT}/3rdparty/CRCpp/inc" EXCLUDE_FROM_ALL)

target_link_libraries(RenameViaHash PRIVATE libbinaryio)

# VS stuff
set_property(DIRECTORY ${ROOT} PROPERTY VS_STARTUP_PROJECT RenameViaHash)
source_group(TREE ${ROOT} FILES ${SOURCES})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")