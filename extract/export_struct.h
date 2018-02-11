//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ExportAnnotation
{

//
class Variable
{
  std::string m_name;

public:
  Variable()  = default;
  ~Variable() = default;
};

//
class Property
{
  std::string m_name;

public:
  Property()  = default;
  ~Property() = default;
};

//
class Function
{
  std::string m_name;

public:
  Function()  = default;
  ~Function() = default;
};

//
class Struct
{
public:
  enum class Type : uint8_t
  {
    Struct,
    Class,
    NotSupport
  };

private:
  std::string m_namespace;
  std::string m_name;

  std::vector<Variable> m_variables;
  std::vector<Property> m_properties;
  std::vector<Function> m_functions;

  Type m_type = Type::Struct;

public:
  Struct(std::string ns, std::string name) : m_namespace(ns), m_name(name) {}
  ~Struct() = default;

  void storeType(Type t) { m_type = t; }
};

//
//
//
class State
{
  using StructPtr = std::shared_ptr<Struct>;

  std::string            m_namespace;
  std::vector<StructPtr> m_struct_list;
  StructPtr              m_current_struct;

public:
  State()  = default;
  ~State() = default;

  void pushNamespace(std::string ns) { m_namespace = ns; }
  void popNamespace() { m_namespace = ""; }

  void startStruct(std::string name)
  {
    auto ptr = std::make_shared<Struct>(m_namespace, name);
    m_struct_list.push_back(ptr);
    m_current_struct = ptr;
  }
  void endStruct() { m_current_struct.reset(); }

  StructPtr getCurrentStruct() { return m_current_struct; }
};
} // namespace ExportAnnotation