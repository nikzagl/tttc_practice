#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <map>

using namespace clang;

class VarRenamerVisitor : public RecursiveASTVisitor<VarRenamerVisitor> {
  Rewriter &rewriter_;
  SourceManager *SM_;
  std::map<VarDecl*, std::string> renamedVars_;

public:
  explicit VarRenamerVisitor(Rewriter &R) 
    : rewriter_(R), SM_(nullptr) {}

  void setSourceManager(SourceManager &SM) { SM_ = &SM; }

  bool isInMainFile(SourceLocation loc) {
    return SM_ && SM_->isWrittenInMainFile(loc);
  }

  bool VisitVarDecl(VarDecl *VD) {
    if (!VD || !isInMainFile(VD->getBeginLoc())) return true;
    if (VD->isImplicit()) return true;

    std::string prefix;
    if (isa<ParmVarDecl>(VD)) {
      prefix = "param_";
    } else if (VD->getStorageClass() == SC_Static) {
      prefix = "static_";
    } else if (VD->getDeclContext()->isTranslationUnit()) {
      prefix = "global_";
    } else {
      prefix = "local_";
    }

    std::string newName = prefix + VD->getName().str();
    renamedVars_[VD] = newName;
    
    SourceLocation nameLoc = VD->getLocation();
    rewriter_.ReplaceText(nameLoc, VD->getName().str().length(), newName);
    
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    if (!DRE || !isInMainFile(DRE->getBeginLoc())) return true;

    if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      auto it = renamedVars_.find(VD);
      if (it != renamedVars_.end()) {
        SourceLocation nameLoc = DRE->getLocation();
        rewriter_.ReplaceText(nameLoc, VD->getName().str().length(), it->second);
      }
    }
    return true;
  }
};

class VarRenamerConsumer : public ASTConsumer {
  Rewriter rewriter_;
  VarRenamerVisitor visitor_;

public:
  explicit VarRenamerConsumer(CompilerInstance &CI)
      : rewriter_(CI.getSourceManager(), CI.getLangOpts()),
        visitor_(rewriter_) {
    visitor_.setSourceManager(CI.getSourceManager());
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    visitor_.TraverseDecl(Context.getTranslationUnitDecl());
    
    FileID mainFileID = Context.getSourceManager().getMainFileID();
    
    if (const RewriteBuffer *Buffer = rewriter_.getRewriteBufferFor(mainFileID)) {
      llvm::outs() << std::string(Buffer->begin(), Buffer->end());
    } else {
      llvm::outs() << rewriter_.getSourceMgr().getBufferData(mainFileID);
    }
  }
};

class VarRenamerPlugin : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef) override {
    return std::make_unique<VarRenamerConsumer>(CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &Args) override {
    return true;
  }
};

static FrontendPluginRegistry::Add<VarRenamerPlugin>
    X("var-renamer", "Renames variables with type prefixes");