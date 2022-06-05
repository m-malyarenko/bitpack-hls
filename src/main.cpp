#include <iostream>
#include <string>

#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

#include "config.hpp"

#include "hardware/HardwareConstraints.hpp"
#include "BitpackHls.hpp"

static const int MAIN_ARGC = 3;
static const std::string HLS_SRC_DIR(D_HLS_SRC_DIR);
static const std::string HLS_OUT_DIR(D_HLS_OUT_DIR);

static llvm::LLVMContext llvm_context;
static llvm::SMDiagnostic llvm_err;

auto* constraints = llvm::bphls::hardware::HardwareConstraints::getHardwareConstraints();

int main(int argc, char const *argv[]) {
    if (argc != MAIN_ARGC) {
        std::cerr << "Illegal number of parameters" << std::endl;
        return 1;
    }

    std::string ir_file(argv[1]);
    std::string function_name(argv[2]);

    auto module = llvm::parseIRFile(ir_file, llvm_err, llvm_context);

    if (module.get() == nullptr) {
        std::cerr << "IR module not found" << std::endl;
        return 1;
    }

    auto function = module->getFunction(function_name);

    if (function == nullptr) {
        std::cerr << "Function '" << function_name << "' not found" << std::endl;
        return 1;
    }

    llvm::bphls::BitpackHls hls(*function /*, constrains */);

    const bool hls_status = hls.run();

    if (hls_status) {
        // std::string hls_output_buffer;
        // llvm::raw_string_ostream hls_output(hls_output_buffer);

        // hls.writeOut(hls_output);

        // std::cout << hls_output_buffer;

        return 0;
    } else {
        return 1;
    }
}