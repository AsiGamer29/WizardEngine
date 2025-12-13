cmake_minimum_required(VERSION 3.10)

project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE CORE_SOURCES "src/Core/*.cpp" "src/Core/*.h")
file(GLOB_RECURSE WINDOW_SOURCES "src/Window/*.cpp" "src/Window/*.h")
file(GLOB_RECURSE RENDERING_SOURCES "src/Rendering/*.cpp" "src/Rendering/*.h")
file(GLOB_RECURSE SCENE_SOURCES "src/Scene/*.cpp" "src/Scene/*.h")
file(GLOB_RECURSE COMPONENTS_SOURCES "src/Components/*.cpp" "src/Components/*.h")
file(GLOB_RECURSE GEOMETRY_SOURCES "src/Geometry/*.cpp" "src/Geometry/*.h")
file(GLOB_RECURSE ASSETS_SOURCES "src/Assets/*.cpp" "src/Assets/*.h")
file(GLOB_RECURSE EDITOR_SOURCES "src/Editor/*.cpp" "src/Editor/*.h")

set(MAIN_SOURCE "src/main.cpp")

set(ALL_SOURCES
    ${MAIN_SOURCE}
    ${CORE_SOURCES}
    ${WINDOW_SOURCES}
    ${RENDERING_SOURCES}
    ${SCENE_SOURCES}
    ${COMPONENTS_SOURCES}
    ${GEOMETRY_SOURCES}
    ${ASSETS_SOURCES}
    ${EDITOR_SOURCES}
)

source_group("Core" FILES ${CORE_SOURCES})
source_group("Window" FILES ${WINDOW_SOURCES})
source_group("Rendering" FILES ${RENDERING_SOURCES})
source_group("Scene" FILES ${SCENE_SOURCES})
source_group("Components" FILES ${COMPONENTS_SOURCES})
source_group("Geometry" FILES ${GEOMETRY_SOURCES})
source_group("Assets" FILES ${ASSETS_SOURCES})
source_group("Editor" FILES ${EDITOR_SOURCES})
source_group("" FILES ${MAIN_SOURCE})

add_executable(Engine ${ALL_SOURCES})

target_include_directories(Engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Window
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Editor
)


target_link_libraries(Engine PRIVATE fmt::fmt)
target_link_libraries(Engine PRIVATE SDL3::SDL3)
target_link_libraries(Engine PRIVATE glad::glad)
target_link_libraries(Engine PRIVATE glm::glm)
target_link_libraries(Engine PRIVATE assimp::assimp)
target_link_libraries(Engine PRIVATE DevIL::IL)
target_link_libraries(Engine PRIVATE DevIL::ILU)
target_link_libraries(Engine PRIVATE imgui::imgui)
target_link_libraries(Engine PRIVATE imguizmo::imguizmo)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)