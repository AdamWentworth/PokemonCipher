cmake_minimum_required(VERSION 3.22)
project(PokemonCipher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

add_executable(PokemonCiper
  src/main.cpp
  src/Game.cpp
  src/Map.cpp
  src/GameObject.cpp
  src/TextureManager.cpp
  src/manager/AssetManager.cpp
  src/manager/Scene.cpp

  src/utils/Vector2D.cpp
  src/utils/Collision.cpp

  src/ecs/World.cpp
  src/ecs/systems/CollisionSystem.cpp
)

target_include_directories(PokemonCipher PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PokemonCipher PRIVATE
  SDL3::SDL3
  $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
  tinyxml2::tinyxml2
)

# Copy assets next to the exe: build/debug/assets/...
set(ASSETS_SRC "${CMAKE_SOURCE_DIR}/assets")
add_custom_command(TARGET PokemonCipher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${ASSETS_SRC}"
          "$<TARGET_FILE_DIR:PokemonCipher>/assets"
)

if (MSVC)
  set_target_properties(PokemonCipher PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:PokemonCipher>"
  )
endif()

if (WIN32)
  set_target_properties(PokemonCipher PROPERTIES
    WIN32_EXECUTABLE OFF
  )
endif()
