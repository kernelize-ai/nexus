#include <tt_library.h>


void TTLibrary::jitProgram(ttm::Program &program, const ttm::CoreRange &cores, const CompileTimeArgs &compile_time_args) {
    // Q: why does it need cores
    // create 3 source files
    std::string reader_kernel_str = "#define READER_KERNEL\n#include \"" + file + "\"\n";
    TT_OBJ_CHECK(reader_kernel, ttm::CreateKernelFromString,
        program, reader_kernel_str, cores,
        ttm::DataMovementConfig{.processor = ttm::DataMovementProcessor::RISCV_0,
                        .noc = ttm::NOC::RISCV_0_default,
                        .compile_args = compile_time_args});
    std::string writer_kernel_str = "#define WRITER_KERNEL\n#include \"" + file + "\"\n";
    TT_OBJ_CHECK(writer_kernel, ttm::CreateKernelFromString,
        program, writer_kernel_str, cores,
        ttm::DataMovementConfig{.processor = ttm::DataMovementProcessor::RISCV_1,
                        .noc = ttm::NOC::RISCV_1_default,
                        .compile_args = compile_time_args});
    std::string compute_kernel_str = "#define COMPUTE_KERNEL\n#include \"" + file + "\"\n";
    TT_OBJ_CHECK(compute_kernel, ttm::CreateKernelFromString,
        program, compute_kernel_str, cores,
        ttm::ComputeConfig{.math_fidelity = MathFidelity::HiFi4, .compile_args = compile_time_args});
}

void TTLibrary::setupCommonRuntime(ttm::Program &program, const RunTimeArgs &run_time_args) {
    TT_CHECK(ttm::SetCommonRuntimeArgs, program, reader_kernel, run_time_args);
    TT_CHECK(ttm::SetCommonRuntimeArgs, program, writer_kernel, run_time_args);
    TT_CHECK(ttm::SetCommonRuntimeArgs, program, compute_kernel, run_time_args);
}

void TTLibrary::setupCoreRuntime(ttm::Program &program, const ttm::CoreCoord &core, const RunTimeArgs &run_time_args) {
    TT_CHECK(ttm::SetRuntimeArgs, program, reader_kernel, core, run_time_args);
    TT_CHECK(ttm::SetRuntimeArgs, program, writer_kernel, core, run_time_args);
    TT_CHECK(ttm::SetRuntimeArgs, program, compute_kernel, core, run_time_args);
}