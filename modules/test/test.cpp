//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#include "test.h"
#include <iostream>

namespace TEST
{

//
void
Test::setMessage(const std::string m)
{
  msg = m;
}

//
const std::string&
Test::getMessage() const
{
  return msg;
}

//
void
Test::print() const
{
  std::cout << "[" << id << "]: " << msg << std::endl;
  count++;
}

//
int
Test::getCount() const
{
  return count;
}

} // namespace TEST
