cmake_minimum_required(VERSION 3.10)
project(HUST_AudioSDK)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES 
    src/main.cpp 
    src/AudioEngine.cpp 
    src/AudioSource.cpp 
)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} ${SOURCES})

# Tự động tìm và link OpenAL cùng 3 module của SFML (Graphics, Window, System)
target_link_libraries(${PROJECT_NAME} 
    openal 
    sfml-graphics 
    sfml-window 
    sfml-system
)
