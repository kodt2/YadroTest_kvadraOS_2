cmake_minimum_required(VERSION 3.14)
project(SystemMonitor)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

find_package(Threads REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG master
)
FetchContent_MakeAvailable(ixwebsocket)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

add_executable(monitor_backend 
    main.cpp 
    SystemMonitor.cpp
)

target_link_libraries(monitor_backend 
    PRIVATE 
    ixwebsocket 
    httplib::httplib
    nlohmann_json::nlohmann_json
    Threads::Threads
    ZLIB::ZLIB
    $<$<PLATFORM_ID:Linux>:ssl crypto>
)