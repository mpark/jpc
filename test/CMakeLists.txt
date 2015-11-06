cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++14 -Wall -Wextra -Werror)

function(add_test_case name)
  add_executable(${name} ${name}.cpp ${name}.pb.cc)
  target_link_libraries(${name} gtest gtest_main pthread protobuf)
  add_test(${name} ${name} --gtest_color=yes)
endfunction(add_test_case)

add_test_case(intro)
