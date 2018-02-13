//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#pragma once
#include <common/export.h>
#include <string>

namespace TEST
{

// test enum
enum class LUAEXPORT TestEnum : uint8_t
{
  HELLO,
  WORLD,
  GOODBYE,
};

// test class
struct LUAEXPORT Test
{
private:
  std::string msg;
  mutable int count = 0;

public:
  Test()  = default;
  ~Test() = default;

  enum class State : uint8_t
  {
    STOP,
    START,
    FINISH
  };

  int id = 0;

  LUAPROPERTY void  setMessage(const std::string m);
  LUAPROPERTY const std::string& getMessage() const;

  LUAPROPERTY int  getCount() const;
  LUAPROPERTY void getID(int i) { id = i; }

  void print() const;

  static constexpr size_t MAX_SIZE = 200;
};

} // namespace TEST
