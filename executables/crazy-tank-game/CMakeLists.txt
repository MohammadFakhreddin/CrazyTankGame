########################################

set(EXECUTABLE "CrazyTankGame")

set(EXECUTABLE_RESOURCES)

list(
    APPEND EXECUTABLE_RESOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameApp.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CrazyTankGameApp.cpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})


########################################