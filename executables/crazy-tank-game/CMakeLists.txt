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
    "${CMAKE_CURRENT_SOURCE_DIR}/Layers.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tank.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tank.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bullet.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bullet.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FollowCamera.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FollowCamera.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PathFinder.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PathFinder.cpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

########################################