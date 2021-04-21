//
//

#include "llektor.hh"
#include <algorithm>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>

static llvm::MDNode* get_integer_md(llvm::LLVMContext& ctx, uint32_t value) {
    auto val = llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(ctx), value, false);
    llvm::MDNode::get(ctx, {
            llvm::ConstantAsMetadata::get(val)
    });
}

void llektor_pass_tag(llvm::Module &module) {

    // Count basic blocks
    size_t bbcount = 0;

    auto doNothingFunc = llvm::Intrinsic::getDeclaration(&module, llvm::Intrinsic::donothing);

    for (auto& func: module.functions()) {
        for (auto& bb: func.getBasicBlockList()) {
            auto *tag_insn = llvm::CallInst::Create(doNothingFunc->getFunctionType(), doNothingFunc);
            tag_insn->setMetadata("llektor_bbno", get_integer_md(module.getContext(), bbcount++));

            bb.getInstList().insert(bb.getFirstInsertionPt(), tag_insn);
        }
    }
}
llvm::PreservedAnalyses LLektorTagPass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    llektor_pass_tag(module);
    return llvm::PreservedAnalyses::none();
}

llvm::PreservedAnalyses LLektorInstrumentPass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    return llvm::PreservedAnalyses();
}

llvm::PreservedAnalyses LLektorPrunePass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    return llvm::PreservedAnalyses();
}

static llvm::RegisterPass<LLektorTagPassLegacy> registration(
        "llektor-tag",
        "LLektor Tag",
        false,
        false
        );

static void registerPasses(llvm::PassBuilder &passBuilder) {
    passBuilder.registerPipelineParsingCallback(
            [](llvm::StringRef name, llvm::ModulePassManager &pm,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement> InnerPipeline){
                if (name == "llektor-tag") {
                    pm.addPass(LLektorTagPass());
                    return true;
                } else {
                    return false;
                }
            });
}

extern "C"
llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() LLVM_ATTRIBUTE_WEAK;

llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
            .APIVersion = LLVM_PLUGIN_API_VERSION,
            .PluginName = "LLektor",
            .PluginVersion = "0.1.0",
            .RegisterPassBuilderCallbacks = registerPasses
    };
}

char LLektorTagPassLegacy::ID = 0;

bool LLektorTagPassLegacy::runOnModule(llvm::Module &M) {
    llektor_pass_tag(M);
    return true;
}
