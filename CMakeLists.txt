# SPDX-License-Identifier: CC-BY-4.0
cmake_minimum_required(VERSION 3.16)
project(net_latency_lab C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NET_LATENCY_LAB_SCTP "Build SCTP support (requires libsctp)" ON)

find_package(Threads REQUIRED)

add_executable(net_latency_lab
  src/main.c
  src/timing.c
  src/stats.c
  src/manifest.c
  src/json_out.c
  src/tcp_bench.c
  src/udp_bench.c
  src/server.c
)

target_include_directories(net_latency_lab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(net_latency_lab PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(net_latency_lab PRIVATE Threads::Threads m)

if(NET_LATENCY_LAB_SCTP)
  find_library(SCTP_LIB sctp)
  if(SCTP_LIB)
    target_sources(net_latency_lab PRIVATE src/sctp_bench.c)
    target_compile_definitions(net_latency_lab PRIVATE NET_LATENCY_HAVE_SCTP=1)
    target_link_libraries(net_latency_lab PRIVATE ${SCTP_LIB})
  else()
    message(STATUS "libsctp not found; SCTP disabled")
  endif()
endif()

enable_testing()
add_test(NAME net_latency_selftest COMMAND net_latency_lab selftest)

add_custom_target(net_latency_lab_ci
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH= ${CMAKE_CURRENT_SOURCE_DIR}/python
          python3 ${CMAKE_CURRENT_SOURCE_DIR}/python/validate_json.py
          ${CMAKE_BINARY_DIR}/smoke_bench.json
  DEPENDS net_latency_lab
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
