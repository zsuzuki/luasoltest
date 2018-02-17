//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#include <iostream>
#include <sol2.h>

#include <luabridge.h>
#include <test/test.h>

const char* Hello = "Hello,Gekko";

int
main(int argc, const char** argv)
{
  sol::state lua;

  std::string inputfile;
  std::string inputscript;
  for (int i = 1; i < argc; i++)
  {
    auto        getnext = [&]() { return (++i < argc) ? argv[i] : ""; };
    std::string arg     = argv[i];
    if (arg[0] == '-')
    {
      if (arg == "-e")
      {
        inputscript = getnext();
      }
    }
    else if (inputfile.empty())
    {
      inputfile = arg;
    }
  }

  lua["hello"] = []() { std::cout << Hello << std::endl; };
  LUAMODULES::module_test(lua);
  TEST::Test t("C++");
  lua["t"] = t;
  try
  {
    if (inputscript.empty())
    {
      inputfile.empty() ? lua.script("hello()") : lua.script_file(inputfile);
    }
    else
    {
      lua.script(inputscript);
    }
  }
  catch (sol::error& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << "execute failed..." << std::endl;
  }
  // lua.open_libraries(sol::lib::base, sol::lib::package);
  return 0;
}
