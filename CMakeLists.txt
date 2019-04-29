cmake_minimum_required(VERSION 2.4)
project(zentrLogg)

set(CMAKE_C_STANDARD 99)

add_executable(zentrLogg main.c)
add_executable(zentrLoggTest test.c)
target_link_libraries(zentrLogg m)