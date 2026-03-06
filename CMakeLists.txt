cmake_minimum_required(VERSION 3.22)
project(PokemonCipher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

add_executable(PokemonCipher
  src/main.cpp
  src/app/LaunchOptions.cpp
  src/engine/Game.cpp
  src/engine/TextureManager.cpp
  src/engine/Map.cpp
  src/engine/TilemapRenderer.cpp

  src/game/OverworldScene.cpp
  src/game/OverworldSceneMap.cpp
  src/game/OverworldSceneDev.cpp
  src/game/OverworldSceneInteraction.cpp
  src/game/OverworldSceneEncounter.cpp
  src/game/BattleScene.cpp
  src/game/TitleScene.cpp
  src/game/dev/OverworldDevConsole.cpp
  src/game/dev/OverworldDevConsoleEncounterHandlers.cpp
  src/game/dev/OverworldDevConsoleParsing.cpp
  src/game/dev/OverworldDevConsoleInfoHandlers.cpp
  src/game/dev/OverworldDevConsoleScriptHandlers.cpp
  src/game/dev/OverworldDevConsoleTravelHandlers.cpp
  src/game/dev/OverworldDevConsoleStateHandlers.cpp
  src/game/encounters/WildEncounterService.cpp
  src/game/scripting/LuaOverworldScriptLoader.cpp
  src/game/save/SaveGameStorage.cpp
  src/game/data/SpeciesRegistry.cpp
  src/game/ui/DialogueOverlay.cpp
  src/game/ui/FrlgTextRenderer.cpp
  src/game/ui/PartyMenuLayout.cpp
  src/game/ui/PartyMenuOverlay.cpp
  src/game/ui/PokemonSummaryLayout.cpp
  src/game/ui/PokemonSummaryOverlay.cpp
  src/game/ui/StartMenuOverlay.cpp
  src/game/world/DefaultMapRegistry.cpp
  src/game/world/MapRegistryLoader.cpp
  src/game/world/OverworldCharacterAssets.cpp
  src/game/world/OverworldEntityFactory.cpp

  src/engine/manager/AssetManager.cpp

  src/engine/utils/Vector2D.cpp
  src/engine/utils/Collision.cpp

  src/engine/ecs/World.cpp
  src/engine/ecs/systems/CollisionSystem.cpp
)

target_include_directories(PokemonCipher PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PokemonCipher PRIVATE
  SDL3::SDL3
  $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
  tinyxml2::tinyxml2
)

if (TARGET Lua::Lua)
  target_link_libraries(PokemonCipher PRIVATE Lua::Lua)
else()
  target_include_directories(PokemonCipher PRIVATE ${LUA_INCLUDE_DIR})
  target_link_libraries(PokemonCipher PRIVATE ${LUA_LIBRARIES})
endif()

set(ASSETS_SRC "${CMAKE_SOURCE_DIR}/assets")
message(STATUS "PokemonCipher assets source: assets")
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
