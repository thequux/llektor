//
//

#pragma once

#include "llvm/IR/PassManager.h"

struct LLektorTagPassLegacy: public llvm::ModulePass {
    static char ID;
    LLektorTagPassLegacy(): llvm::ModulePass(ID) {}

    bool runOnModule(llvm::Module &M) override;
};

struct LLektorInstrumentPassLegacy: public llvm::ModulePass {
    static char ID;
    LLektorInstrumentPassLegacy(): llvm::ModulePass(ID) {}

    bool runOnModule(llvm::Module &M) override;
};

struct LLektorPrunePassLegacy: public llvm::ModulePass {
    static char ID;
    LLektorPrunePassLegacy(): llvm::ModulePass(ID) {}

    bool runOnModule(llvm::Module &M) override;
};

class LLektorTagPass: public llvm::PassInfoMixin<LLektorTagPass>{
public:
    static char ID;
    llvm::PreservedAnalyses run(llvm::Module& module, llvm::ModuleAnalysisManager& analysisManager);
};

class LLektorInstrumentPass: public llvm::PassInfoMixin<LLektorInstrumentPass>{
public:
    llvm::PreservedAnalyses run(llvm::Module& module, llvm::ModuleAnalysisManager& analysisManager);
};

class LLektorPrunePass: public llvm::PassInfoMixin<LLektorPrunePass>{
public:
    llvm::PreservedAnalyses run(llvm::Module& module, llvm::ModuleAnalysisManager& analysisManager);
};

