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
  bool read_only;

public:
  Variable(const std::string name, bool ro) : Unit(name), read_only(ro) {}
  ~Variable() override = default;

  bool isReadOnly() const { return read_only; }
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
class Enum
{
private:
  struct Data
  {
    union Num {
      uint64_t u;
      int64_t  s;
    } n;
    bool        is_signed;
    std::string label;

    Data() = default;
    Data(const std::string& l, bool s = true) : is_signed(s), label(l) {}
  };
  std::vector<Data> m_data_list;

  std::string m_namespace;
  std::string m_name;
  std::string m_type;

public:
  Enum(const std::string& ns, const std::string& n, const std::string& t) : m_namespace(ns), m_name(n), m_type(t) {}
  Enum()  = default;
  ~Enum() = default;

  void pushSigned(const std::string& name, int64_t num)
  {
    Data d(name);
    d.n.s = num;
    m_data_list.push_back(d);
  }
  void pushUnsigned(const std::string& name, uint64_t num)
  {
    Data d(name, false);
    d.n.u = num;
    m_data_list.push_back(d);
  }

  void put(std::ostream& ostr, std::string target) const
  {
    ostr << "  sol::table t_" << m_name << " = lua.create_table_with();\n";
    for (auto& d : m_data_list)
    {
      ostr << "  t_" << m_name << "[\"" << d.label << "\"] = ";
      if (d.is_signed)
      {
        ostr << d.n.s;
      }
      else
      {
        ostr << d.n.u;
      }
      ostr << ";\n";
    }
    if (target.empty())
    {
      ostr << "  t_" << m_namespace << "[\"" << m_name << "\"] = t_" << m_name << ";\n";
    }
    else
    {
      ostr << "  lua[\"" << target << "\"][\"" << m_name << "\"] = t_" << m_name << ";\n";
    }
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

  using Constructor = std::vector<std::string>;

  std::vector<Variable>           m_variables;
  std::vector<Function>           m_functions;
  std::map<std::string, Property> m_properties;
  std::vector<Constructor>        m_constructors;
  std::vector<Enum>               m_enum_list;

  Type m_type = Type::Struct;

public:
  Struct(std::string ns, std::string name) : m_namespace(ns), m_name(name) {}
  ~Struct() = default;

  void               storeType(Type t) { m_type = t; }
  const std::string& getName() const { return m_name; }

  void pushFunction(const std::string func_name) { m_functions.push_back(Function(func_name)); }
  void pushVariable(const std::string var_name, bool ro) { m_variables.push_back(Variable(var_name, ro)); }
  void pushProperty(const std::pair<std::string, bool> prop)
  {
    auto& p = m_properties[prop.first];
    p.setName(prop.first);
    prop.second ? p.haveSetter() : p.haveGetter();
  }
  void pushConstructor(std::vector<std::string>& arg_list) { m_constructors.push_back(arg_list); }
  void pushEnumerate(const Enum& e) { m_enum_list.push_back(e); }

  void put(std::ostream& ostr) const
  {
    std::string module_name = m_namespace.empty() ? m_name : m_namespace + "::" + m_name;
    ostr << "  lua.new_usertype<" << module_name << ">(\n"
         << "    \"" << m_name << "\"";
    if (m_constructors.size() > 0)
    {
      ostr << ",\n    sol::constructors<";
      bool first_c = true;
      for (auto& c : m_constructors)
      {
        if (first_c == false)
        {
          ostr << ", ";
        }
        ostr << "sol::types<";
        bool first_a = true;
        for (auto& arg : c)
        {
          if (first_a == false)
          {
            ostr << ", ";
          }
          ostr << arg;
          first_a = false;
        }
        ostr << ">";
        first_c = false;
      }
      ostr << ">()";
    }
    for (auto& f : m_functions)
    {
      ostr << ",\n    \"" << f.getName() << "\", &" << module_name << "::" << f.getName();
    }
    for (auto& v : m_variables)
    {
      if (v.isReadOnly())
      {
        ostr << ",\n    \"" << v.getName() << "\", sol::readonly(&" << module_name << "::" << v.getName() << ")";
      }
else { ostr << ",\n    \"" << v.getName() << "\", &" << module_name << "::" << v.getName(); }
       
    }
    for (auto& p : m_properties)
    {
      auto prop   = p.second;
      auto getset = prop.getProp();
      auto pname  = prop.getName();
      ostr << ",\n    \"" << p.first << "\", sol::property(";
      if (getset.first)
      {
        ostr << "&" << module_name << "::get" << pname;
        if (getset.second)
        {
          ostr << ", ";
        }
      }
      if (getset.second)
      {
        ostr << "&" << module_name << "::set" << pname;
      }
      ostr << ")";
    }
    ostr << " );" << std::endl;
    for (auto& e : m_enum_list)
    {
      e.put(ostr, m_name);
    }
  }
};

//
//
//
class State
{
  using StructPtr  = std::shared_ptr<Struct>;
  using StructList = std::vector<StructPtr>;
  using EnumList   = std::vector<Enum>;
  using TableList  = std::vector<std::string>;

  std::string m_namespace;
  StructList  m_struct_list;
  StructPtr   m_current_struct;
  EnumList    m_enum_list;
  TableList   m_create_table_list;

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

  void pushEnumerate(const Enum& e)
  {
    auto result = std::find(m_create_table_list.begin(), m_create_table_list.end(), m_namespace);
    if (result == m_create_table_list.end())
    {
      m_create_table_list.push_back(m_namespace);
    }
    m_enum_list.push_back(e);
  }

  StructPtr          getCurrentStruct() { return m_current_struct; }
  const std::string& getNamespace() const { return m_namespace; }

  void putCPP(std::ostream& ostr, std::string input_name, std::string module_name) const
  {
    ostr << "// auto generate file\n#include <sol2.h>\n#include \"" << input_name << "\"\n\n";
    ostr << "namespace LUAMODULES\n{\nvoid module_" << module_name << "(sol::state& lua)\n{\n";
    for (auto& s : m_struct_list)
    {
      s->put(ostr);
    }
    for (auto& t : m_create_table_list)
    {
      ostr << "  sol::table t_" << t << " = lua.create_named_table(\"" << t << "\");\n";
    }
    for (auto& e : m_enum_list)
    {
      e.put(ostr, "");
    }
    ostr << "}\n} // namespace LUAMODULES\n" << std::endl;
  }

  void putHPP(std::ostream& ostr, std::string module_name) const
  {
    ostr << "// auto generate file\n#pragma once\n#include <sol2.h>\n\nnamespace LUAMODULES\n{\n";
    ostr << "void module_" << module_name << "(sol::state& lua);\n";
    ostr << "} // namespace LUAMODULES\n" << std::endl;
  }
};
} // namespace ExportAnnotation