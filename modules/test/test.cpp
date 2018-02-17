//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#include "test.h"
#include <iostream>

namespace TEST
{

//
Test::Test(int i) : id(i) { std::cout << "init number: " << i << std::endl; }

//
Test::Test(const char* m) : msg(m) { std::cout << "init message: " << m << std::endl; }

//
Test::~Test() { std::cout << "Test Destructor" << id << "," << count << "," << msg << std::endl; }

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
