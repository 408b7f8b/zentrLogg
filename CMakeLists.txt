cmake_minimum_required(VERSION 3.10)
project(zentrLogg)

set(CMAKE_C_STANDARD 90)

add_executable(zentrLogg main.c)
add_executable(zentrLoggTest test.c)
target_link_libraries(zentrLogg m)