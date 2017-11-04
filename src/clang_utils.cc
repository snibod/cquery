#include "clang_utils.h"

#include "platform.h"

namespace {

lsRange GetLsRangeForFixIt(const CXSourceRange& range) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, nullptr, &start_line, &start_column,
                            nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return lsRange(lsPosition(start_line - 1, start_column - 1) /*start*/,
                 lsPosition(end_line - 1, end_column) /*end*/);
}

}  // namespace

optional<lsDiagnostic> BuildAndDisposeDiagnostic(CXDiagnostic diagnostic,
                                                 const std::string& path) {
  // Get diagnostic location.
  CXFile file;
  unsigned int line, column;
  clang_getSpellingLocation(clang_getDiagnosticLocation(diagnostic), &file,
                            &line, &column, nullptr);

  // Only report diagnostics in the same file. Using
  // clang_Location_isInSystemHeader causes crashes for some reason.
  if (path != FileName(file)) {
    clang_disposeDiagnostic(diagnostic);
    return nullopt;
  }

  // Build diagnostic.
  lsDiagnostic ls_diagnostic;

  // TODO: consider using clang_getDiagnosticRange
  // TODO: ls_diagnostic.range is lsRange, we have Range. We should only be
  // storing Range types when inside the indexer so that index <-> buffer
  // remapping logic is applied.
  ls_diagnostic.range =
      lsRange(lsPosition(line - 1, column), lsPosition(line - 1, column));

  ls_diagnostic.message = ToString(clang_getDiagnosticSpelling(diagnostic));

  // Append the flag that enables this diagnostic, ie, [-Wswitch]
  std::string enabling_flag =
      ToString(clang_getDiagnosticOption(diagnostic, nullptr));
  if (!enabling_flag.empty())
    ls_diagnostic.message += " [" + enabling_flag + "]";

  ls_diagnostic.code = clang_getDiagnosticCategory(diagnostic);

  switch (clang_getDiagnosticSeverity(diagnostic)) {
    case CXDiagnostic_Ignored:
    case CXDiagnostic_Note:
      ls_diagnostic.severity = lsDiagnosticSeverity::Information;
      break;
    case CXDiagnostic_Warning:
      ls_diagnostic.severity = lsDiagnosticSeverity::Warning;
      break;
    case CXDiagnostic_Error:
    case CXDiagnostic_Fatal:
      ls_diagnostic.severity = lsDiagnosticSeverity::Error;
      break;
  }

  // Report fixits
  unsigned num_fixits = clang_getDiagnosticNumFixIts(diagnostic);
  for (unsigned i = 0; i < num_fixits; ++i) {
    CXSourceRange replacement_range;
    CXString text = clang_getDiagnosticFixIt(diagnostic, i, &replacement_range);

    lsTextEdit edit;
    edit.newText = ToString(text);
    edit.range = GetLsRangeForFixIt(replacement_range);
    ls_diagnostic.fixits_.push_back(edit);
  }

  clang_disposeDiagnostic(diagnostic);

  return ls_diagnostic;
}

std::string FileName(CXFile file) {
  CXString cx_name = clang_getFileName(file);
  std::string name = ToString(cx_name);
  return NormalizePath(name);
}

std::string ToString(CXString cx_string) {
  std::string string;
  if (cx_string.data != nullptr) {
    string = clang_getCString(cx_string);
    clang_disposeString(cx_string);
  }
  return string;
}

std::string ToString(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_UnexposedDecl:
      return "UnexposedDecl";
    case CXCursor_StructDecl:
      return "StructDecl";
    case CXCursor_UnionDecl:
      return "UnionDecl";
    case CXCursor_ClassDecl:
      return "ClassDecl";
    case CXCursor_EnumDecl:
      return "EnumDecl";
    case CXCursor_FieldDecl:
      return "FieldDecl";
    case CXCursor_EnumConstantDecl:
      return "EnumConstantDecl";
    case CXCursor_FunctionDecl:
      return "FunctionDecl";
    case CXCursor_VarDecl:
      return "VarDecl";
    case CXCursor_ParmDecl:
      return "ParmDecl";
    case CXCursor_ObjCInterfaceDecl:
      return "ObjCInterfaceDecl";
    case CXCursor_ObjCCategoryDecl:
      return "ObjCCategoryDecl";
    case CXCursor_ObjCProtocolDecl:
      return "ObjCProtocolDecl";
    case CXCursor_ObjCPropertyDecl:
      return "ObjCPropertyDecl";
    case CXCursor_ObjCIvarDecl:
      return "ObjCIvarDecl";
    case CXCursor_ObjCInstanceMethodDecl:
      return "ObjCInstanceMethodDecl";
    case CXCursor_ObjCClassMethodDecl:
      return "ObjCClassMethodDecl";
    case CXCursor_ObjCImplementationDecl:
      return "ObjCImplementationDecl";
    case CXCursor_ObjCCategoryImplDecl:
      return "ObjCCategoryImplDecl";
    case CXCursor_TypedefDecl:
      return "TypedefDecl";
    case CXCursor_CXXMethod:
      return "CXXMethod";
    case CXCursor_Namespace:
      return "Namespace";
    case CXCursor_LinkageSpec:
      return "LinkageSpec";
    case CXCursor_Constructor:
      return "Constructor";
    case CXCursor_Destructor:
      return "Destructor";
    case CXCursor_ConversionFunction:
      return "ConversionFunction";
    case CXCursor_TemplateTypeParameter:
      return "TemplateTypeParameter";
    case CXCursor_NonTypeTemplateParameter:
      return "NonTypeTemplateParameter";
    case CXCursor_TemplateTemplateParameter:
      return "TemplateTemplateParameter";
    case CXCursor_FunctionTemplate:
      return "FunctionTemplate";
    case CXCursor_ClassTemplate:
      return "ClassTemplate";
    case CXCursor_ClassTemplatePartialSpecialization:
      return "ClassTemplatePartialSpecialization";
    case CXCursor_NamespaceAlias:
      return "NamespaceAlias";
    case CXCursor_UsingDirective:
      return "UsingDirective";
    case CXCursor_UsingDeclaration:
      return "UsingDeclaration";
    case CXCursor_TypeAliasDecl:
      return "TypeAliasDecl";
    case CXCursor_ObjCSynthesizeDecl:
      return "ObjCSynthesizeDecl";
    case CXCursor_ObjCDynamicDecl:
      return "ObjCDynamicDecl";
    case CXCursor_CXXAccessSpecifier:
      return "CXXAccessSpecifier";
    // case CXCursor_FirstDecl: return "FirstDecl";
    // case CXCursor_LastDecl: return "LastDecl";
    // case CXCursor_FirstRef: return "FirstRef";
    case CXCursor_ObjCSuperClassRef:
      return "ObjCSuperClassRef";
    case CXCursor_ObjCProtocolRef:
      return "ObjCProtocolRef";
    case CXCursor_ObjCClassRef:
      return "ObjCClassRef";
    case CXCursor_TypeRef:
      return "TypeRef";
    case CXCursor_CXXBaseSpecifier:
      return "CXXBaseSpecifier";
    case CXCursor_TemplateRef:
      return "TemplateRef";
    case CXCursor_NamespaceRef:
      return "NamespaceRef";
    case CXCursor_MemberRef:
      return "MemberRef";
    case CXCursor_LabelRef:
      return "LabelRef";
    case CXCursor_OverloadedDeclRef:
      return "OverloadedDeclRef";
    case CXCursor_VariableRef:
      return "VariableRef";
    // case CXCursor_LastRef: return "LastRef";
    // case CXCursor_FirstInvalid: return "FirstInvalid";
    case CXCursor_InvalidFile:
      return "InvalidFile";
    case CXCursor_NoDeclFound:
      return "NoDeclFound";
    case CXCursor_NotImplemented:
      return "NotImplemented";
    case CXCursor_InvalidCode:
      return "InvalidCode";
    // case CXCursor_LastInvalid: return "LastInvalid";
    // case CXCursor_FirstExpr: return "FirstExpr";
    case CXCursor_UnexposedExpr:
      return "UnexposedExpr";
    case CXCursor_DeclRefExpr:
      return "DeclRefExpr";
    case CXCursor_MemberRefExpr:
      return "MemberRefExpr";
    case CXCursor_CallExpr:
      return "CallExpr";
    case CXCursor_ObjCMessageExpr:
      return "ObjCMessageExpr";
    case CXCursor_BlockExpr:
      return "BlockExpr";
    case CXCursor_IntegerLiteral:
      return "IntegerLiteral";
    case CXCursor_FloatingLiteral:
      return "FloatingLiteral";
    case CXCursor_ImaginaryLiteral:
      return "ImaginaryLiteral";
    case CXCursor_StringLiteral:
      return "StringLiteral";
    case CXCursor_CharacterLiteral:
      return "CharacterLiteral";
    case CXCursor_ParenExpr:
      return "ParenExpr";
    case CXCursor_UnaryOperator:
      return "UnaryOperator";
    case CXCursor_ArraySubscriptExpr:
      return "ArraySubscriptExpr";
    case CXCursor_BinaryOperator:
      return "BinaryOperator";
    case CXCursor_CompoundAssignOperator:
      return "CompoundAssignOperator";
    case CXCursor_ConditionalOperator:
      return "ConditionalOperator";
    case CXCursor_CStyleCastExpr:
      return "CStyleCastExpr";
    case CXCursor_CompoundLiteralExpr:
      return "CompoundLiteralExpr";
    case CXCursor_InitListExpr:
      return "InitListExpr";
    case CXCursor_AddrLabelExpr:
      return "AddrLabelExpr";
    case CXCursor_StmtExpr:
      return "StmtExpr";
    case CXCursor_GenericSelectionExpr:
      return "GenericSelectionExpr";
    case CXCursor_GNUNullExpr:
      return "GNUNullExpr";
    case CXCursor_CXXStaticCastExpr:
      return "CXXStaticCastExpr";
    case CXCursor_CXXDynamicCastExpr:
      return "CXXDynamicCastExpr";
    case CXCursor_CXXReinterpretCastExpr:
      return "CXXReinterpretCastExpr";
    case CXCursor_CXXConstCastExpr:
      return "CXXConstCastExpr";
    case CXCursor_CXXFunctionalCastExpr:
      return "CXXFunctionalCastExpr";
    case CXCursor_CXXTypeidExpr:
      return "CXXTypeidExpr";
    case CXCursor_CXXBoolLiteralExpr:
      return "CXXBoolLiteralExpr";
    case CXCursor_CXXNullPtrLiteralExpr:
      return "CXXNullPtrLiteralExpr";
    case CXCursor_CXXThisExpr:
      return "CXXThisExpr";
    case CXCursor_CXXThrowExpr:
      return "CXXThrowExpr";
    case CXCursor_CXXNewExpr:
      return "CXXNewExpr";
    case CXCursor_CXXDeleteExpr:
      return "CXXDeleteExpr";
    case CXCursor_UnaryExpr:
      return "UnaryExpr";
    case CXCursor_ObjCStringLiteral:
      return "ObjCStringLiteral";
    case CXCursor_ObjCEncodeExpr:
      return "ObjCEncodeExpr";
    case CXCursor_ObjCSelectorExpr:
      return "ObjCSelectorExpr";
    case CXCursor_ObjCProtocolExpr:
      return "ObjCProtocolExpr";
    case CXCursor_ObjCBridgedCastExpr:
      return "ObjCBridgedCastExpr";
    case CXCursor_PackExpansionExpr:
      return "PackExpansionExpr";
    case CXCursor_SizeOfPackExpr:
      return "SizeOfPackExpr";
    case CXCursor_LambdaExpr:
      return "LambdaExpr";
    case CXCursor_ObjCBoolLiteralExpr:
      return "ObjCBoolLiteralExpr";
    case CXCursor_ObjCSelfExpr:
      return "ObjCSelfExpr";
    // case CXCursor_LastExpr: return "LastExpr";
    // case CXCursor_FirstStmt: return "FirstStmt";
    case CXCursor_UnexposedStmt:
      return "UnexposedStmt";
    case CXCursor_LabelStmt:
      return "LabelStmt";
    case CXCursor_CompoundStmt:
      return "CompoundStmt";
    case CXCursor_CaseStmt:
      return "CaseStmt";
    case CXCursor_DefaultStmt:
      return "DefaultStmt";
    case CXCursor_IfStmt:
      return "IfStmt";
    case CXCursor_SwitchStmt:
      return "SwitchStmt";
    case CXCursor_WhileStmt:
      return "WhileStmt";
    case CXCursor_DoStmt:
      return "DoStmt";
    case CXCursor_ForStmt:
      return "ForStmt";
    case CXCursor_GotoStmt:
      return "GotoStmt";
    case CXCursor_IndirectGotoStmt:
      return "IndirectGotoStmt";
    case CXCursor_ContinueStmt:
      return "ContinueStmt";
    case CXCursor_BreakStmt:
      return "BreakStmt";
    case CXCursor_ReturnStmt:
      return "ReturnStmt";
    case CXCursor_GCCAsmStmt:
      return "GCCAsmStmt";
    // case CXCursor_AsmStmt: return "AsmStmt";
    case CXCursor_ObjCAtTryStmt:
      return "ObjCAtTryStmt";
    case CXCursor_ObjCAtCatchStmt:
      return "ObjCAtCatchStmt";
    case CXCursor_ObjCAtFinallyStmt:
      return "ObjCAtFinallyStmt";
    case CXCursor_ObjCAtThrowStmt:
      return "ObjCAtThrowStmt";
    case CXCursor_ObjCAtSynchronizedStmt:
      return "ObjCAtSynchronizedStmt";
    case CXCursor_ObjCAutoreleasePoolStmt:
      return "ObjCAutoreleasePoolStmt";
    case CXCursor_ObjCForCollectionStmt:
      return "ObjCForCollectionStmt";
    case CXCursor_CXXCatchStmt:
      return "CXXCatchStmt";
    case CXCursor_CXXTryStmt:
      return "CXXTryStmt";
    case CXCursor_CXXForRangeStmt:
      return "CXXForRangeStmt";
    case CXCursor_SEHTryStmt:
      return "SEHTryStmt";
    case CXCursor_SEHExceptStmt:
      return "SEHExceptStmt";
    case CXCursor_SEHFinallyStmt:
      return "SEHFinallyStmt";
    case CXCursor_MSAsmStmt:
      return "MSAsmStmt";
    case CXCursor_NullStmt:
      return "NullStmt";
    case CXCursor_DeclStmt:
      return "DeclStmt";
    case CXCursor_LastStmt:
      return "LastStmt";
    case CXCursor_TranslationUnit:
      return "TranslationUnit";
    // case CXCursor_FirstAttr: return "FirstAttr";
    case CXCursor_UnexposedAttr:
      return "UnexposedAttr";
    case CXCursor_IBActionAttr:
      return "IBActionAttr";
    case CXCursor_IBOutletAttr:
      return "IBOutletAttr";
    case CXCursor_IBOutletCollectionAttr:
      return "IBOutletCollectionAttr";
    case CXCursor_CXXFinalAttr:
      return "CXXFinalAttr";
    case CXCursor_CXXOverrideAttr:
      return "CXXOverrideAttr";
    case CXCursor_AnnotateAttr:
      return "AnnotateAttr";
    case CXCursor_AsmLabelAttr:
      return "AsmLabelAttr";
    case CXCursor_LastAttr:
      return "LastAttr";
    case CXCursor_PreprocessingDirective:
      return "PreprocessingDirective";
    case CXCursor_MacroDefinition:
      return "MacroDefinition";
    case CXCursor_MacroExpansion:
      return "MacroExpansion";
    // case CXCursor_MacroInstantiation: return "MacroInstantiation";
    case CXCursor_InclusionDirective:
      return "InclusionDirective";
    // case CXCursor_FirstPreprocessing: return "FirstPreprocessing";
    // case CXCursor_LastPreprocessing: return "LastPreprocessing";
    case CXCursor_ModuleImportDecl:
      return "ModuleImportDecl";
    // case CXCursor_FirstExtraDecl: return "FirstExtraDecl";
    case CXCursor_LastExtraDecl:
      return "LastExtraDecl";
    default:
      break;
  }
  return "<unknown kind>";
}
