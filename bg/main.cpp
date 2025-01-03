#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include "CoreDSL2BaseListener.h"
#include "CoreDSL2BaseVisitor.h"
#include "CoreDSL2Lexer.h"
#include "CoreDSL2Parser.h"
#include "antlr4-runtime.h"

#include "bg.h"


namespace std {
    template<>
    struct hash<std::pair<OperationType, OperandType>> {
        size_t operator()(const std::pair<OperationType, OperandType>& pair) const {
            auto [opType, operandType] = pair;
            return hash<OperationType>()(opType) ^ (hash<OperandType>()(operandType) << 1);
        }
    };
}

std::unordered_map<OperandType, std::string> operand_type_name_map = {
    {OperandType::FLOAT, "float"},
    {OperandType::DOUBLE, "double"},
    {OperandType::SCOMPLEX, "scomplex"},
    {OperandType::DCOMPLEX, "dcomplex"},
};

std::unordered_map<OperandType, std::string> operand_type_code_map = {
    {OperandType::FLOAT, "s"},
    {OperandType::DOUBLE, "d"},
    {OperandType::SCOMPLEX, "c"},
    {OperandType::DCOMPLEX, "z"},
};

std::unordered_map<OperationType, std::string> operation_prototype_macro_map = {
    {OperationType::ADDV, "ADDV_KER_PROT"},
    {OperationType::AMAXV, "AMAXV_KER_PROT"},
};

std::unordered_map<OperationType, std::string> operation_function_map = {
    {OperationType::ADDV, "addv"},
    {OperationType::AMAXV, "amaxv"},
    {OperationType::AXPYV, "axpyv"},
};

std::unordered_map<OperationType, std::string> operation_signature_map = {
    {OperationType::ADDV, "void bli_{0}addv_(conj_t conjx, dim_t n, const void* x, inc_t incx, void* y, inc_t incy, const cntx_t* cntx)"},
    {OperationType::AMAXV, "void bli_{0}amaxv_(dim_t n, const void* x, inc_t incx, dim_t* index, const cntx_t* cntx)"},
    {OperationType::AXPYV, " void bli_{0}axpyv ( conj_t  conjx, dim_t   n, void*  alpha, void*  x, inc_t incx, void*  y, inc_t incy);"},
};

string to_upper(string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return str;
}

string trim_left_whitespace(string str) {
    std::istringstream iss(str);
    std::ostringstream oss;
    string line;
    size_t min_indent = string::npos;
    std::vector<string> lines;

    // min indentation
    while (std::getline(iss, line)) {
        if (!line.empty() && line.find_first_not_of(" \t") != string::npos) {
            size_t first_non_space = line.find_first_not_of(" \t");
            min_indent = (min_indent == string::npos)
                             ? first_non_space
                             : std::min(min_indent, first_non_space);
        }
        lines.push_back(line);
    }

    for (const auto &l : lines) {
        if (!l.empty()) {
            oss << (min_indent != string::npos && min_indent < l.size()
                        ? l.substr(min_indent)
                        : "")
                << '\n';
        }
    }

    return oss.str();
}


void write_file(const SourceFile &file) {
    std::filesystem::create_directories(file.file_path.parent_path());
    std::ofstream f(file.file_path);
    if (f.is_open()) {
        f << file.content;
        f.close();
    } else {
        std::cerr << "Failed to open file: " << file.file_path << std::endl;
    }
}


void generate_patches(const BlisKernel &kernel) {
    auto patch_dir = kernel.base_dir / "patches";

    SourceFile bli_arch = {
        .file_path = patch_dir / "bli_arch.c.patch",
        .content = trim_left_whitespace(format(R"(
        --- a/frame/base/bli_arch.c
        +++ b/frame/base/bli_arch.c
        @@ -292,0 +292,3 @@
        +#ifdef BLIS_FAMILY_{0}
        +id = BLIS_ARCH_{0};
        +#endif
      )",
                                               to_upper(kernel.name))),
    };
    write_file(bli_arch);

    SourceFile bli_cpuid = {
        .file_path = patch_dir / "bli_cpuid.c.patch",
        .content =
            trim_left_whitespace(format(R"(
        --- a/frame/base/bli_cpuid.c
        +++ b/frame/base/bli_cpuid.c
        @@ -158,0 +158,4 @@
        +#ifdef BLIS_CONFIG_{0}
        +    if ( bli_cpuid_is_{1} (family, model, features)))
        +        return BLIS_ARCH_{0};
        +#endif
    )",
                                        to_upper(kernel.name), kernel.name)),
    };
    write_file(bli_cpuid);

    SourceFile bli_arch_config = {
        .file_path = patch_dir / "bli_arch_config.h.patch",
        .content =
            trim_left_whitespace(format(R"(
      --- a/frame/include/bli_arch_config.h
      +++ b/frame/include/bli_arch_config.h
      @@ -283,0 +283,7 @@
      +#ifdef BLIS_FAMILY_{1}
      +    #include "bli_family_{0}.h"
      +#endif
      +
      +#ifdef BLIS_KERNELS_{1}
      +   #include "bli_kernels_{0}.h"
      +#endif
    )",
                                        kernel.name, to_upper(kernel.name))),
    };
    write_file(bli_arch_config);

    SourceFile bli_gentconf_macro_defs = {
        .file_path = patch_dir / "bli_gentconf_macro_defs.h.patch",
        .content =
            trim_left_whitespace(format(R"(
            --- a/frame/include/bli_gentconf_macro_defs.h
            +++ b/frame/include/bli_gentconf_macro_defs.h
            @@ -230,0 +230,6 @@
            + // {0} configuration ------------------
            +#ifdef BLIS_CONFIG_{1}
            +#define INSERT_GENTCONF_{1} GENTCONF ( {1}, {0})
            +#else
            +#define INSERT_GENTCONF_{1} GENTCONF
            +#endif
            @@ -284,0 +284,1 @@
            +INSERT_GENTCONF_{1} \
    )",
                                        kernel.name, to_upper(kernel.name))),
    };
    write_file(bli_gentconf_macro_defs);

    SourceFile bli_type_defs = {
        .file_path = patch_dir / "bli_type_defs.h.patch",
        .content =
            trim_left_whitespace(format(R"(
            --- a/frame/include/bli_type_defs.h
            +++ b/frame/include/bli_type_defs.h
            @@ -1005,0 +1005,1 @@
            +    BLIS_ARCH_{1},
    )",
                                        kernel.name, to_upper(kernel.name))),
    };
    write_file(bli_type_defs);

    SourceFile config_registry = {
        .file_path = patch_dir / "config_registry.patch",
        .content = trim_left_whitespace(format(R"(
            --- a/config_registry
            +++ b/config_registry
            @@ -68,0 +68,1 @@
            +{0}: {0}
    )",
                                               kernel.name)),
    };
    write_file(config_registry);
}

void generate_files(const BlisKernel &kernel) {
    std::cout << "files" << '\n';
    SourceFile family = {
        .file_path = kernel.base_dir / "config" / kernel.name /
                     format("bli_family_{}.h", kernel.name),
        .content = "// empty",
    };
    write_file(family);

    SourceFile make_defs = {
        .file_path = kernel.base_dir / "config" / kernel.name / "make_defs.mk",
        .content = trim_left_whitespace(format(R"(
            THIS_CONFIG    := {}

            #ifeq ($(CC),)
            CC             := /tmp/seal5_llvm_cli_demo/.seal5/build/release/bin/clang
            CC_VENDOR      := clang
            #endif

            CPPROCFLAGS    := --target=riscv32-unknown-elf -march=rv32ixexample -mabi=ilp32d --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/riscv32-unknown-elf/
            CMISCFLAGS     := -fno-rtti -fno-exceptions
            CPICFLAGS      := -fPIC
            CWARNFLAGS     :=

            ifneq ($(DEBUG_TYPE),off)
            CDBGFLAGS      := -g
            endif

            ifeq ($(DEBUG_TYPE),noopt)
            COPTFLAGS      := -O0
            else
            COPTFLAGS      := -O3
            endif
            $(eval $(call store-make-defs,$(THIS_CONFIG)))
        )",
                                               kernel.name))};
    write_file(make_defs);

    SourceFile cntx = {.file_path = kernel.base_dir / "config" / kernel.name /
                                    format("bli_cntx_init_{}.c", kernel.name),
                       .content = trim_left_whitespace(format(R"(
            #include "blis.h"
            void bli_cntx_init_{0}(cntx_t* cntx)
            {{
                blksz_t blkszs[ BLIS_NUM_BLKSZS ];
                bli_cntx_init_{0}_ref(cntx);
                bli_cntx_set_ukrs(
                    cntx,
                    BLIS_ADDV_KER, BLIS_FLOAT, bli_saddv_{0},
                    //BLIS_ADDV_KER, BLIS_DOUBLE, NULL,
                    BLIS_VA_END
                );
                bli_cntx_set_ukr_prefs
                (
                  cntx,

                  BLIS_GEMM_UKR_ROW_PREF,             BLIS_FLOAT,    TRUE,
                  BLIS_GEMM_UKR_ROW_PREF,             BLIS_DOUBLE,   TRUE,
                  BLIS_GEMM_UKR_ROW_PREF,             BLIS_SCOMPLEX, TRUE,
                  BLIS_GEMM_UKR_ROW_PREF,             BLIS_DCOMPLEX, TRUE,

                  BLIS_VA_END
                );
            }}
        )", kernel.name))};
    write_file(cntx);

    SourceFile kernel_defs = {
        .file_path = kernel.base_dir / "config" / kernel.name /
                                  format("bli_kernel_defs_{}.h", kernel.name),
        .content = trim_left_whitespace(R"(
            // -- REGISTER BLOCK SIZES
            // #define BLIS_MR_s   6
            // #define BLIS_MR_d   6
            // #define BLIS_MR_c   3
            // #define BLIS_MR_z   3
            //
            // #define BLIS_NR_s   16
            // #define BLIS_NR_d   8
            // #define BLIS_NR_c   8
            // #define BLIS_NR_z   4
            )")
    };
    write_file(kernel_defs);

    string definitions_content;
    for (auto &o : kernel.operations) {
        auto type_name = operand_type_name_map[o.operand_type];
        auto type_code = operand_type_code_map[o.operand_type];
        auto prototype_macro = operation_prototype_macro_map[o.operation_type];
        auto name = operation_function_map[o.operation_type];

        if (operand_type_name_map.empty() || type_code.empty() ||
            prototype_macro.empty() || name.empty()) {
            std::cerr << "Operand or operation not implemented" << '\n';
            continue;
        }
        definitions_content +=
            format("{0}({1}, {2}, {3}_{4})\n",
                prototype_macro,
                type_name,
                type_code,
                name,
                kernel.name);
    }

    SourceFile blis_kernels = {
        .file_path = kernel.base_dir / "kernels" / kernel.name /
                     format("bli_kernels_{}.h", kernel.name),
        .content = definitions_content,
    };
    write_file(blis_kernels);

    // Level 1 file
    string level1_content;
    level1_content += "#include \"blis.h\"\n";
    for (auto &o : kernel.operations) {
        level1_content += o.signature;
        level1_content += "{\n";
        level1_content += o.implementation;
        level1_content += "\n}\n";
        level1_content += "\n";
    }
    SourceFile level1 = {
        .file_path =
            kernel.base_dir / "kernels" / kernel.name / "1" / "level1.c",
        .content = level1_content,
    };
    write_file(level1);
}

void generate_kernel(BlisKernel &kernel) {
    generate_patches(kernel);
    generate_files(kernel);
}


int main(int argc, char *argv[]) {
    BlisKernel kernel = {
        .name = "test_kernel",
        .base_dir = std::filesystem::path("/app/bg/build") / "testing_output", // TODO: hardcoded path
    };
    Operation saddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::FLOAT,
        .signature = "void bli_saddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
        // TODO implementation
    )",
    };
    kernel.operations.push_back(saddv);

    Operation daddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::DOUBLE,
        .signature = "void bli_daddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};
    kernel.operations.push_back(daddv);

    Operation caddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::SCOMPLEX,
        .signature = "void bli_caddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};
    kernel.operations.push_back(caddv);

    Operation zaddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::DCOMPLEX,
        .signature = "void bli_zaddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};
    kernel.operations.push_back(zaddv);

    Operation samaxv = {
        .operation_type = OperationType::AMAXV,
        .operand_type = OperandType::FLOAT,
        .signature = "void bli_samaxv_(dim_t n, const void* x, inc_t incx, "
                     "dim_t* index, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};
    kernel.operations.push_back(samaxv);

    Operation damaxv = {
        .operation_type = OperationType::AMAXV,
        .operand_type = OperandType::DOUBLE,
        .signature = "void bli_damaxv_(dim_t n, const void* x, inc_t incx, "
                     "dim_t* index, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};

    generate_kernel(kernel);

    // auto path  = "../RISCV_ISA_CoreDSL/RISCVBase.core_desc";
    // auto path = "../RISCV_ISA_CoreDSL/RV32I.core_desc";
    // auto path  = "../RISCV_ISA_CoreDSL/RISCVEncoding.core_desc";
    // std::ifstream input_file(path, std::ios_base::in | std::ios_base::binary);
    // input_file.seekg(0, std::ios::end);
    // size_t size = input_file.tellg();
    // char buffer[size];
    // input_file.seekg(0, std::ios::beg);
    // input_file.read(buffer, sizeof(buffer));
    // input_file.close();

    // antlr4::ANTLRInputStream input(buffer);
    // CoreDSL2Lexer lexer(&input);
    // antlr4::CommonTokenStream tokens(&lexer);
    // CoreDSL2Parser parser(&tokens);
    // auto *tree = parser.description_content();
    // antlr4::tree::ParseTreeWalker walker;
    // InstructionListener extractor;
    // walker.walk(&extractor, tree);

    // for (auto &i : extractor.instructions) {
    //   std::cout << i;
    // }
    // std::cout << std::endl;

    // if (parser.getNumberOfSyntaxErrors() > 0) {
    //   std::cerr << "syntax errrors" << std::endl;
    //   return 1;
    // }
    // std::cout << tree->toStringTree(true);

    return 0;
}
