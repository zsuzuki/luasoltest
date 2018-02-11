//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#pragma once

#include <iostream>
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
  Variable(std::string name) : m_name(name) {}
  ~Variable() = default;

  const std::string& getName() const { return m_name; }
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
  Function(std::string name) : m_name(name) {}
  ~Function() = default;

  const std::string& getName() const { return m_name; }
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

  void pushFunction(std::string func_name) { m_functions.push_back(Function(func_name)); }
  void pushVariable(std::string var_name) { m_variables.push_back(Variable(var_name)); }

  void put(std::ostream& ostr) const
  {
    ostr << "  lua.new_usertype<" << m_name << ">(\n"
         << "    \"" << m_name << "\",\n";
    for (auto& f : m_functions)
    {
      ostr << "    \"" << f.getName() << "\", &" << m_name << "::" << f.getName() << ",\n";
    }
    for (auto& v : m_variables)
    {
      ostr << "    \"" << v.getName() << "\", &" << m_name << "::" << v.getName() << ",\n";
    }
    ostr << "  );" << std::endl;
  }
};

//
//
//
class State
{
  using StructPtr  = std::shared_ptr<Struct>;
  using StructList = std::vector<StructPtr>;

  std::string m_namespace;
  StructList  m_struct_list;
  StructPtr   m_current_struct;

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

  void put(std::ostream& ostr, std::string module_name) const
  {
    ostr << "// auto generate file\n#include <sol2.h>\n\nvoid regist_module_" << module_name
         << "(sol::state& lua)\n{\n";
    for (auto& s : m_struct_list)
    {
      s->put(ostr);
    }
    ostr << "}\n" << std::endl;
  }
};
} // namespace ExportAnnotation