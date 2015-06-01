cmake_minimum_required (VERSION 2.8)
project (voltmeter-server)

# Setup Qt4
find_package(Qt4 REQUIRED QtCore QtNetwork)
include_directories(${QT_INCLUDES})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_FILES
    main.cpp)
    
add_executable(voltmeter-server ${SOURCE_FILES})
target_link_libraries(voltmeter-server Qt4::QtCore Qt4::QtNetwork)
