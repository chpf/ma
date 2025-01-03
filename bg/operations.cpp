#include "main.cpp"

    Operation saddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::FLOAT,
        .signature = "void bli_saddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
        // TODO implementation
    )",
    };

    Operation daddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::DOUBLE,
        .signature = "void bli_daddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};

    Operation caddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::SCOMPLEX,
        .signature = "void bli_caddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};

    Operation zaddv = {
        .operation_type = OperationType::ADDV,
        .operand_type = OperandType::DCOMPLEX,
        .signature = "void bli_zaddv_(conj_t conjx, dim_t n, const void* x, "
                     "inc_t incx, void* y, inc_t incy, const cntx_t* cntx)",
        .implementation = R"(
          // TODO implementation
    )"};
