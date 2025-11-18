#include <tt_library.h>


void TTLibrary::jitProgram(ttm::Program &program, const ttm::CoreRange &cores, const CompileTimeArgs &compile_time_args) {

    // create 3 source files
    std::string reader_kernel_str = "#define READER_KERNEL\n#include \"" + file + "\"\n";
    reader_kernel = ttm::CreateKernelFromString(
        program, reader_kernel_str, cores,
        ttm::DataMovementConfig{.processor = ttm::DataMovementProcessor::RISCV_0,
                        .noc = ttm::NOC::RISCV_0_default,
                        .compile_args = compile_time_args});
    std::string writer_kernel_str = "#define WRITER_KERNEL\n#include \"" + file + "\"\n";
    writer_kernel = ttm::CreateKernelFromString(
        program, writer_kernel_str, cores,
        ttm::DataMovementConfig{.processor = ttm::DataMovementProcessor::RISCV_1,
                        .noc = ttm::NOC::RISCV_1_default,
                        .compile_args = compile_time_args});
    std::string compute_kernel_str = "#define COMPUTE_KERNEL\n#include \"" + file + "\"\n";
    compute_kernel = ttm::CreateKernelFromString(
        program, compute_kernel_str, cores,
        ttm::ComputeConfig{.math_fidelity = MathFidelity::HiFi4, .compile_args = compile_time_args});
}

void TTLibrary::setupCoreRuntime(ttm::Program &program, const ttm::CoreCoord &core, const RunTimeArgs &run_time_args) {
    SetRuntimeArgs(program, reader_kernel, core, run_time_args);
    SetRuntimeArgs(program, writer_kernel, core, run_time_args);
    SetRuntimeArgs(program, compute_kernel, core, run_time_args);
}