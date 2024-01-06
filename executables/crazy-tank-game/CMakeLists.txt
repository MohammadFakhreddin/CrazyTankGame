########################################

set(EXECUTABLE "CrazyTankGame")

set(EXECUTABLE_RESOURCES)

list(
    APPEND EXECUTABLE_RESOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameApp.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Map.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Map.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Gameplay.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Gameplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics2D.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics2D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB2D.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB2D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Layers.hpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})


########################################