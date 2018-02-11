//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#include <iostream>
#include <sol2.h>

#define ANNO(label) __attribute__((annotate(#label)))

struct ANNO(test) Hello
{
  ANNO(open) int n = 0;
  ANNO(func) void put() { std::cout << "Struct" << std::endl; }
};

const char* Hello = "Hello,Gekko";

int
main(int argc, const char** argv)
{
  sol::state lua;

  lua["hello"] = []() { std::cout << Hello << std::endl; };
  // lua.open_libraries(sol::lib::base, sol::lib::package);
  lua.script("hello()");
  return 0;
}
