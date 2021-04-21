add_llvm_library(LLektor
        MODULE llektor.cc
        DEPENDS intrinsics_gen
        PLUGIN_TOOL opt)