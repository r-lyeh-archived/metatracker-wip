cmake_minimum_required(VERSION 2.8)

project(meta)

if (CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    if(MSVC)
#        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        add_definitions(/Zi /Oy- /DDEBUG /Zm2000)
    else()
        add_definitions(-std=c99 -O0 -DDEBUG -D_DEBUG)
    endif()
else()
    if(MSVC)
#        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        add_definitions(/Ox /Oy /DNDEBUG /D_NDEBUG /Zm2000)
    else()
        add_definitions(-std=c99 -O3 -DNDEBUG -D_NDEBUG)
    endif()
endif()

add_subdirectory(deps/imgui/deps/glfw)
#add_subdirectory(deps/imgui)
include_directories(deps/imgui/deps/glew/include)
include_directories(deps/imgui/deps/glfw/include)
include_directories(deps/imgui/src)
include_directories(deps/medea)
include_directories(deps/sand)

add_executable(
    meta
    meta.cc
    dialog.cpp
    locate.cpp
    deps/imgui/src/imgui.cpp
    deps/imgui/src/imguiGL.cpp
    deps/imgui/src/stb_image.c
    deps/imgui/src/fonts/Entypo-Social.ttf.cpp
    deps/imgui/src/fonts/Entypo-Social.ttf.hpp
    deps/imgui/src/fonts/Entypo.ttf.cpp
    deps/imgui/src/fonts/Entypo.ttf.hpp
    deps/imgui/src/fonts/Ubuntu-B.ttf.cpp
    deps/imgui/src/fonts/Ubuntu-B.ttf.hpp
    #deps/imgui/src/fonts/DroidSansFallback.ttf.cpp
    #deps/imgui/src/fonts/DroidSansFallback.ttf.hpp
    deps/imgui/src/fonts/WenQuanYiMicroHei.ttf.cpp
    deps/imgui/src/fonts/WenQuanYiMicroHei.ttf.hpp
    deps/imgui/src/fonts/FontAwesome.ttf.cpp
    deps/imgui/src/fonts/FontAwesome.ttf.hpp
    deps/imgui/deps/glew/src/glew.c
    deps/medea/medea.cpp
    deps/sand/sand.cpp
)

add_executable(
    sample
    sample.cc
)

if(APPLE)
    target_link_libraries(meta glfw ${GLFW_LIBRARIES})
    target_link_libraries(sample)
endif()

if(UNIX)
    target_link_libraries(meta dl glfw ${GLFW_LIBRARIES})
    target_link_libraries(sample dl)
endif()

if(WIN32)
    target_link_libraries(meta ws2_32.lib glfw ${GLFW_LIBRARIES})
    target_link_libraries(sample ws2_32.lib)
endif()
