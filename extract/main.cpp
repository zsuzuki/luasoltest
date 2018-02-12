//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//

// 以下のサイトを参考にして作成しています
// https://qiita.com/Chironian/items/6021d35bf2750341d80c
//

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <fstream>
#include <iostream>

#include "export_struct.h"

using namespace clang;

namespace
{

std::string output_filename; // 出力ファイル名(オプション)
std::string input_filename;  // 入力ファイル名

// オプションから出力ファイル名に相当する部分を抜き出して、それ以外をそのまま返す
std::vector<const char*>
build_option(int argc, const char** argv)
{
  std::vector<const char*> option_list;
  option_list.reserve(argc);
  option_list.resize(0);

  bool check_finish = false;
  for (int i = 0; i < argc; i++)
  {
    std::string arg     = argv[i];
    bool        to_copy = check_finish;
    if (to_copy == false)
    {
      if (arg == "--")
      {
        to_copy = true;
      }
      else if (arg == "-o")
      {
        i++;
        if (i >= argc)
        {
          break;
        }
        output_filename = argv[i];
      }
      else
      {
        to_copy = true;
        if (i > 0 && arg[0] != '-' && input_filename.empty())
        {
          input_filename = arg;
        }
      }
    }

    if (to_copy)
    {
      option_list.push_back(argv[i]);
    }
  }

  return option_list;
}

} // namespace

//
// 解析
//
class ExportVisitor : public DeclVisitor<ExportVisitor, bool>
{
private:
  PrintingPolicy          Policy;
  const SourceManager&    SM;
  ExportAnnotation::State State;

  enum class AnnotatePolicy : uint8_t
  {
    None,
    Export,
    Property
  };

public:
  ExportVisitor(CompilerInstance* CI)
      : Policy(PrintingPolicy(CI->getASTContext().getPrintingPolicy())), SM(CI->getASTContext().getSourceManager())
  {
  }

  // (ファイルとかに)出力
  void put() const
  {
    auto basename = input_filename.substr(input_filename.find_last_of('/') + 1);
    auto mainname = basename.substr(0, basename.find_last_of('.'));
    if (output_filename.empty())
    {
      State.putCPP(std::cout, input_filename, mainname);
    }
    else
    {
      auto outputfile = std::ofstream(output_filename);
      State.putCPP(outputfile, input_filename, mainname);
    }
  }

private:
  // アノテーションのチェック
  AnnotatePolicy checkAnnotation(Decl* aDecl)
  {
    auto ret = AnnotatePolicy::None;
    if (aDecl->hasAttrs())
    {
      AttrVec& Attrs = aDecl->getAttrs();
      for (AttrVec::const_iterator i = Attrs.begin(), e = Attrs.end(); i != e; ++i)
      {
        AnnotateAttr* AA = dyn_cast<AnnotateAttr>(*i);
        if (AA)
        {
          std::string annotation = AA->getAnnotation();
          if (annotation == "luaexport")
          {
            ret = AnnotatePolicy::Export;
          }
          else if (annotation == "luaproperty")
          {
            ret = AnnotatePolicy::Property;
          }
        }
      }
    }
    return ret;
  }

public:
  // DeclContextメンバーの1レベルの枚挙処理
  void EnumerateDecl(DeclContext* aDeclContext)
  {
    for (DeclContext::decl_iterator i = aDeclContext->decls_begin(), e = aDeclContext->decls_end(); i != e; i++)
    {
      Decl* D = *i;
      // auto  cs = State.getCurrentStruct();
      // if (cs)
      // {
      //   if (D->getKind() == Decl::Kind::Var)
      //   {
      //     llvm::errs() << "TopLevel : " << D->getDeclKindName(); // Declの型表示
      //     if (NamedDecl* N = dyn_cast<NamedDecl>(D))
      //       llvm::errs() << " " << N->getNameAsString();                       // NamedDeclなら名前表示
      //     llvm::errs() << " (" << D->getLocation().printToString(SM) << ")\n"; // ソース上の場所表示
      //   }
      // }
      Visit(D); // llvm\tools\clang\include\clang\AST\DeclVisitor.hの38行目
    }
  }

  // class/struct/unionの処理
  bool VisitCXXRecordDecl(CXXRecordDecl* aCXXRecordDecl, bool aForce = false)
  {
    // 参照用(class foo;のような宣言)なら追跡しない
    if (!aCXXRecordDecl->isCompleteDefinition())
    {
      return true;
    }

    // 名前無しなら表示しない(ただし、強制表示されたら表示する:Elaborated用)
    if (!aCXXRecordDecl->getIdentifier() && !aForce)
    {
      return true;
    }

    // アノテーション(in アトリビュート)

    if (checkAnnotation(aCXXRecordDecl) == AnnotatePolicy::Export)
    {
      // 出力struct
      State.startStruct(aCXXRecordDecl->getNameAsString());
    }

    // クラス定義の処理
    auto cs = State.getCurrentStruct();
    if (cs)
    {
      // 型の種類
      switch (aCXXRecordDecl->TagDecl::getTagKind())
      {
      case TTK_Struct:
        cs->storeType(ExportAnnotation::Struct::Type::Struct);
        break;
      case TTK_Class:
        cs->storeType(ExportAnnotation::Struct::Type::Class);
        break;
      default:
        cs->storeType(ExportAnnotation::Struct::Type::NotSupport);
        break;
      }

      // メンバーの枚挙処理
      EnumerateDecl(aCXXRecordDecl);
    }
    return true;
  }

  // メンバー変数の処理
  bool VisitFieldDecl(FieldDecl* aFieldDecl)
  {
    // 名前無しclass/struct/unionでメンバー変数が直接定義されている時の対応
    CXXRecordDecl* R = nullptr; // 名前無しの時、内容を表示するためにCXXRecordDeclをポイントする
    const Type*    T = aFieldDecl->getType().split().Ty;
    if (T->getTypeClass() == Type::Elaborated)
    {
      R = cast<ElaboratedType>(T)->getNamedType()->getAsCXXRecordDecl();
      if (R && (R->getIdentifier()))
      {
        R = nullptr;
      }
    }

    // 内容表示
    if (R)
    {
      VisitCXXRecordDecl(R, true); // 名前無しclass/struct/unionの内容表示
    }
    else
    {
      auto cs = State.getCurrentStruct();
      if (cs)
      {
        // アクセススペック(public, protected, private)
        if (aFieldDecl->getAccess() == AS_public)
        {
          // output
          cs->pushVariable(aFieldDecl->getNameAsString());
        }
      }
    }
    return true;
  }

  // 関数
  bool VisitFunctionDecl(FunctionDecl* aFunctionDecl)
  {
    // 内容表示
    auto cs = State.getCurrentStruct();
    if (cs)
    {
      if (aFunctionDecl->getAccess() == AS_public)
      {
        auto func_name = aFunctionDecl->getNameAsString();
        if (checkAnnotation(aFunctionDecl) == AnnotatePolicy::Property)
        {
          // llvm::errs() << "(property)";
          auto prop = ExportAnnotation::Property::getPropName(func_name);
          if (!prop.first.empty())
          {
            cs->pushProperty(prop);
          }
        }
        else
        {
          if (aFunctionDecl->getDeclKind() == Decl::Kind::CXXMethod)
          {
            cs->pushFunction(func_name); // 通常のメソッドのみ登録
          }
        }
      }
    }

    return true;
  }

  // Enum
  bool VisitEnumDecl(EnumDecl* aEnumDecl)
  {
    // auto cs        = State.getCurrentStruct();
    // bool to_export = checkAnnotation(aEnumDecl) == AnnotatePolicy::Export;
    // if (to_export || cs)
    // {
    //   llvm::errs() << "Enum: " << aEnumDecl->getNameAsString() << "\n";
    //   auto el = aEnumDecl->enumerators();
    //   for (auto e = el.begin(); e != el.end(); e++)
    //   {
    //     llvm::errs() << "  M: " << (*e)->getNameAsString() << "\n";
    //   }
    // }
    return true;
  }

  // クラス・テンプレート
  bool VisitClassTemplateDecl(ClassTemplateDecl* aClassTemplateDecl)
  {
    Visit(aClassTemplateDecl->getTemplatedDecl());
    return true;
  }

  // typedef
  bool VisitTypedefDecl(TypedefDecl* aTypedefDecl)
  {
    // 名前無しclass/struct/unionでメンバー変数が直接定義されている時の対応
    CXXRecordDecl* R = nullptr; // 名前無しの時、内容を表示するためにCXXRecordDeclをポイントする
    const Type*    T = aTypedefDecl->getTypeSourceInfo()->getType().split().Ty;
    if (T->getTypeClass() == Type::Elaborated)
    {
      R = cast<ElaboratedType>(T)->getNamedType()->getAsCXXRecordDecl();
      if (R && (R->getIdentifier()))
      {
        R = nullptr;
      }
    }
    // 内容表示
    auto cs = State.getCurrentStruct();
    if (cs && R)
    {
      // 名前無しclass/struct/unionの内容表示
      VisitCXXRecordDecl(R, true);
    }

    return true;
  }

  // namespaceの処理(配下を追跡する)
  bool VisitNamespaceDecl(NamespaceDecl* aNamespaceDecl)
  {
    State.pushNamespace(aNamespaceDecl->getNameAsString());
    EnumerateDecl(aNamespaceDecl);
    State.popNamespace();
    return true;
  }

  bool VisitUsingDirectiveDecl(UsingDirectiveDecl* aUsingDirectiveDecl) { return true; }

  // extern "C"/"C++"の処理(配下を追跡する)
  bool VisitLinkageSpecDecl(LinkageSpecDecl* aLinkageSpecDecl)
  {
    // switch (aLinkageSpecDecl->getLanguage())
    // {
    // case LinkageSpecDecl::lang_c: lang = "C"; break;
    // case LinkageSpecDecl::lang_cxx: lang = "C++"; break;
    // }
    EnumerateDecl(aLinkageSpecDecl);
    return true;
  }
};

// ***************************************************************************
//          定番の処理
// ***************************************************************************

class ExportASTConsumer : public ASTConsumer
{
private:
  ExportVisitor* visitor; // doesn't have to be private
public:
  explicit ExportASTConsumer(CompilerInstance* CI) : visitor(new ExportVisitor(CI)) {}
  // AST解析結果の受取
  void HandleTranslationUnit(ASTContext& Context) override
  {
    visitor->EnumerateDecl(Context.getTranslationUnitDecl());
    visitor->put();
  }
};

class ExportFrontendAction : public SyntaxOnlyAction /*ASTFrontendAction*/
{
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef file) override
  {
    return llvm::make_unique<ExportASTConsumer>(&CI); // pass CI pointer to ASTConsumer
  }
};

//
//
//
static llvm::cl::OptionCategory MyToolCategory("lua-export tool options");
int
main(int argc, const char** argv)
{
  auto                         args   = build_option(argc, argv);
  int                          n_args = args.size();
  tooling::CommonOptionsParser op(n_args, &args[0], MyToolCategory);
  tooling::ClangTool           Tool(op.getCompilations(), op.getSourcePathList());
  auto                         action = clang::tooling::newFrontendActionFactory<ExportFrontendAction>();
  return Tool.run(action.get());
}
