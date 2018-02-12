//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
#pragma once

#include <ctype.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ExportAnnotation
{

//
class Unit
{
  std::string m_name;

public:
  Unit() = default;
  Unit(const std::string name) : m_name(name) {}
  virtual ~Unit() = default;

  void               setName(const std::string& name) { m_name = name; }
  const std::string& getName() const { return m_name; }
};

//
class Variable : public Unit
{
public:
  Variable(const std::string name) : Unit(name) {}
  ~Variable() override = default;
};

//
class Function : public Unit
{
public:
  Function(const std::string name) : Unit(name) {}
  ~Function() override = default;
};

//
class Property : public Unit
{
  bool has_getter = false;
  bool has_setter = false;

public:
  Property()  = default;
  ~Property() = default;

  void                  haveSetter() { has_setter = true; }
  void                  haveGetter() { has_getter = true; }
  std::pair<bool, bool> getProp() const { return std::make_pair(has_getter, has_setter); }

  static std::pair<std::string, bool> getPropName(std::string name)
  {
    if (name.length() < 4)
    {
      return std::make_pair("", false);
    }

    auto dirname = name.substr(0, 3);
    bool dir     = true;
    if (dirname == "get" || dirname == "Get")
    {
      dir = false;
    }
    else if (dirname != "set" && dirname != "Set")
    {
      return std::make_pair("", false);
    }

    return std::make_pair(name.substr(3), dir);
  }
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

  std::vector<Variable>           m_variables;
  std::vector<Function>           m_functions;
  std::map<std::string, Property> m_properties;

  Type m_type = Type::Struct;

public:
  Struct(std::string ns, std::string name) : m_namespace(ns), m_name(name) {}
  ~Struct() = default;

  void storeType(Type t) { m_type = t; }

  void pushFunction(const std::string func_name) { m_functions.push_back(Function(func_name)); }
  void pushVariable(const std::string var_name) { m_variables.push_back(Variable(var_name)); }
  void pushProperty(const std::pair<std::string, bool> prop)
  {
    auto& p = m_properties[prop.first];
    p.setName(prop.first);
    prop.second ? p.haveSetter() : p.haveGetter();
  }

  void put(std::ostream& ostr) const
  {
    std::string module_name = m_namespace.empty() ? m_name : m_namespace + "::" + m_name;
    ostr << "  lua.new_usertype<" << module_name << ">(\n"
         << "    \"" << m_name << "\"";
    for (auto& f : m_functions)
    {
      ostr << ",\n    \"" << f.getName() << "\", &" << module_name << "::" << f.getName();
    }
    for (auto& v : m_variables)
    {
      ostr << ",\n    \"" << v.getName() << "\", &" << module_name << "::" << v.getName();
    }
    for (auto& p : m_properties)
    {
      auto prop   = p.second;
      auto getset = prop.getProp();
      auto pname  = prop.getName();
      ostr << ",\n    \"" << p.first << "\", sol::property(";
      getset.first ? ostr << "&" << module_name << "::get" << pname : ostr << "nulptr";
      ostr << ", ";
      getset.second ? ostr << "&" << module_name << "::set" << pname : ostr << "nulptr";
      ostr << ")";
    }
    ostr << " );" << std::endl;
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

  void putCPP(std::ostream& ostr, std::string input_name, std::string module_name) const
  {
    ostr << "// auto generate file\n#include <sol2.h>\n#include \"" << input_name << "\"\n\n";
    ostr << "namespace LUAMODULES\n{\nvoid module_" << module_name << "(sol::state& lua)\n{\n";
    for (auto& s : m_struct_list)
    {
      s->put(ostr);
    }
    ostr << "}\n} // namespace LUAMODULES\n" << std::endl;
  }
};
} // namespace ExportAnnotation