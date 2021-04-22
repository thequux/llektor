//
//

#include <sys/random.h>
#include "llektor.hh"
#include <algorithm>
#include <iostream>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;

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

    std::string modid;
    uint8_t rndbuf[8];
    getrandom(rndbuf, 8, 0);
    for (unsigned char i : rndbuf) {
        const char* HEX = "0123456789ABCDEF";
        modid.append(1, HEX[(i >> 4) & (char)0xF]);
        modid.append(1, HEX[(i >> 0) & (char)0xF]);
    }

    module.getOrInsertNamedMetadata("llektor_modid")
            ->addOperand(MDNode::get(module.getContext(), {
                MDString::get(module.getContext(), modid)
            }));
    for (auto& func: module.functions()) {
        for (auto& bb: func.getBasicBlockList()) {
            auto *tag_insn = llvm::CallInst::Create(doNothingFunc->getFunctionType(), doNothingFunc);
            tag_insn->setMetadata("llektor_bbno", get_integer_md(module.getContext(), bbcount++));

            bb.getInstList().insert(bb.getFirstInsertionPt(), tag_insn);
        }
    }
}

void llektor_pass_instrument(Module& module) {
    // Find the maximum size of this module's bblist
    raw_os_ostream ll_cerr(std::cerr);
    uint64_t max_bbno = 0;

    // Create function to fetch output buffer
    auto i8ptr = Type::getInt8PtrTy(module.getContext());
    auto i32ty = Type::getInt32Ty(module.getContext());
    auto getTraceBufFn = module.getOrInsertFunction("LLEKTOR_get_trace_buf",
                               i8ptr, i8ptr, i32ty);
    auto initTraceBuf = Function::Create(FunctionType::get(i8ptr, false), GlobalValue::LinkageTypes::PrivateLinkage,"LLEKTOR_init_tbuf", module);

//    module.getNamedMetadata("llektor_modid")->getOperand(0)->getOperand(0)-->print(ll_cerr);
    auto mdstr = module.getNamedMetadata("llektor_modid")->getOperand(0)->getOperand(0).get();
    auto moduleName = cast<MDString>(mdstr)->getString();

    auto modid_cval = ConstantDataArray::getString(module.getContext(), moduleName);
    auto modid_cvar = new GlobalVariable(module, modid_cval->getType(), true, GlobalValue::PrivateLinkage, modid_cval);
    auto zero_i8 = cast<Constant>(ConstantInt::get(Type::getInt8Ty(module.getContext()), 0));

    auto modid = module.getOrInsertGlobal("LLEKTOR_modid", i8ptr, [&]() {
        return new GlobalVariable(module, i8ptr, true, GlobalValue::PrivateLinkage,
                                  ConstantExpr::getInBoundsGetElementPtr(modid_cvar->getValueType(), modid_cvar, ArrayRef<Constant*>{zero_i8, zero_i8}) );
    });
//    auto modid = ConstantDataArray::getString(module.getContext(), moduleName);
    auto trace_buf = module.getOrInsertGlobal("LLEKTOR_trbuf", i8ptr, [&]() {
        return new GlobalVariable(module, i8ptr, false, GlobalValue::PrivateLinkage, ConstantPointerNull::get(i8ptr));
    });


//    trace_buf->print(ll_cerr, true);
//    ll_cerr << "\n";
//    ll_cerr.flush();

//    initTraceBuf->print(ll_cerr, nullptr, false, true);


    for (auto &fn: module.functions()) {
        Value* tbuf_val = nullptr;
        for (auto &bb: fn.getBasicBlockList()) {
            for (auto &insn: bb.getInstList()) {
                if (insn.hasMetadata("llektor_bbno")) {
                    auto meta = insn.getMetadata("llektor_bbno");
                    auto bbno = cast<ConstantInt>(cast<ConstantAsMetadata>(meta->getOperand(0))->getValue());

                    if (bbno->getZExtValue() > max_bbno) {
                        max_bbno = bbno->getZExtValue();
                    }

                    IRBuilder<> builder(&insn);
                    if (tbuf_val == nullptr) {
                        tbuf_val = builder.CreateCall(initTraceBuf);
                    }
                    auto elptr = builder.CreateGEP(tbuf_val, bbno);
                    builder.CreateStore(ConstantInt::get(Type::getInt8Ty(module.getContext()), 1),
                                        elptr);
                    break;
                }
            }
        }
    }

    {
        // Create the initialization fn
        auto entry_bb = BasicBlock::Create(module.getContext(), "", initTraceBuf);
        auto init_bb = BasicBlock::Create(module.getContext(), "", initTraceBuf);
        auto ret_bb = BasicBlock::Create(module.getContext(), "", initTraceBuf);
        IRBuilder<> entry_b(entry_bb);
        IRBuilder<> init_b(init_bb);
        IRBuilder<> ret_b(ret_bb);
        auto oldval = entry_b.CreateLoad(trace_buf);
        auto isNull = entry_b.CreateCmp(CmpInst::ICMP_EQ, oldval, ConstantPointerNull::get(i8ptr));
        entry_b.CreateCondBr(isNull, init_bb, ret_bb);

        auto newval = init_b.CreateCall(getTraceBufFn, {
            ConstantExpr::getInBoundsGetElementPtr(modid_cvar->getValueType(), modid_cvar, ArrayRef<Constant*>{zero_i8, zero_i8}),
            ConstantInt::get(i32ty, max_bbno)
        });
        init_b.CreateStore(newval, trace_buf);
        init_b.CreateRet(newval);

        ret_b.CreateRet(oldval);
    }

}

llvm::PreservedAnalyses LLektorTagPass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    llektor_pass_tag(module);
    return llvm::PreservedAnalyses::none();
}

llvm::PreservedAnalyses LLektorInstrumentPass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    llektor_pass_instrument(module);
    return llvm::PreservedAnalyses::none();
}

llvm::PreservedAnalyses LLektorPrunePass::run(llvm::Module &module, llvm::ModuleAnalysisManager &analysisManager) {
    return llvm::PreservedAnalyses::none();
}

static llvm::RegisterPass<LLektorTagPassLegacy> registration_tag(
        "llektor-tag",
        "LLektor Tag",
        false,
        false
        );

static llvm::RegisterPass<LLektorInstrumentPassLegacy> registration_instr(
        "llektor-instr",
        "LLektor: Instrument",
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
                } else if (name == "llektor-instrument") {
                    pm.addPass(LLektorInstrumentPass());
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
char LLektorInstrumentPassLegacy::ID = 0;

bool LLektorTagPassLegacy::runOnModule(llvm::Module &M) {
    llektor_pass_tag(M);
    return true;
}

bool LLektorInstrumentPassLegacy::runOnModule(llvm::Module &M) {
    llektor_pass_instrument(M);
    return true;
}
