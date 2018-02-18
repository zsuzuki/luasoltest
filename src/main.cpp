//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#include <iostream>
#include <luabridge.h>
#include <test/test.h>

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

  try
  {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::math,
                       sol::lib::table, sol::lib::debug, sol::lib::bit32);
    LUAMODULES::module_test(lua);
    if (inputscript.empty())
    {
      inputfile.empty() ? lua.script("print('Hello')") : lua.script_file(inputfile);
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
  return 0;
}
