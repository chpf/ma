#pragma once

using std::format;
using std::string;

enum class OperationType {
    //Level 1
    ADDV,
    AMAXV,
    AXPBYV,
    AXPYV,
    COPYV,
    DOTV,
    DOTXV,
    INVERTV,
    INVSCALV,
    SCALV,
    SCAL2V,
    SETV,
    SUBV,
    SWAPV,
    XPBYV,

    // Level 1d
    ADDD,
    AXPYD,
    COPYD,
    INVERTD,
    INVSCALD,
    SCALD,
    SCAL2D,
    SETD,
    SETIT,
    SHIFTD,
    SUBD,
    XPBYD,

    // Level 1m
    ADDM,
    AXPYM,
    COPYM,
    INVSCALM,
    SCALM,
    SCAL2M,
    SETM,
    SUBM,

    // Level 1f
    AXPY2V,
    AXPYF,
    DOTXF,
    DOTAXPYV,
    DOTXAXPYF,

    // Level 3
    GEMM,
    GEMMTRSM,
    HEMM,
    HERK,
    HER2K,
    SYMM,
    SYRK,
    SYR2K,
    TRMM,
    TRSM,
    TRMM3,
};

enum class OperandType {
    FLOAT,    // s
    DOUBLE,   // d
    SCOMPLEX, // c
    DCOMPLEX, // z
};

union Implementation {
    string c;
    string assembly;
    string tensor_ir;
};

struct Operation {
    // A BLAS Operation like ADDV, GEMM, GEMV, etc
    OperationType operation_type;
    OperandType operand_type;
    string signature;
    string implementation;
    // Implementation implementation;
};

struct BlisKernel {
    const string name;
    const std::filesystem::path base_dir;
    // std::array<Operation, 4> operations;
    std::vector<Operation> operations;
};

struct SourceFile {
    const std::filesystem::path file_path;
    const string content;
};

class InstructionListener : public CoreDSL2BaseListener {
  public:
    struct Instruction {
        string name;
        string encoding;
        string assembler;
        string behavior;
    };

    std::vector<Instruction> instructions;

    friend std::ostream &operator<<(std::ostream &os, const Instruction &ins) {
        os << ins.name << "\n";
        // os << "\t" << ins.encoding << "\n";
        // os << "\t" << ins.assembler << "\n";
        // os << "\t" << ins.behavior << "\n";
        return os;
    }

    void enterInstruction(CoreDSL2Parser::InstructionContext *ctx) override {
        Instruction inst;
        inst.name = ctx->name->getText();
        if (ctx->assembly) {
            inst.assembler = ctx->assembly->getText();
        };
        inst.behavior = ctx->behavior->getText();
        instructions.push_back(inst);
    };
};