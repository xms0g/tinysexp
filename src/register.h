#ifndef REGISTER_H
#define REGISTER_H

#include <cstdint>

#define INUSE 1 << 0
#define isINUSE(status) (status & INUSE)

#define isSSE(type) ((type >> 0) & 1)
#define isSCRATCH(type) ((type >> 1) & 1)
#define isPRESERVED(type) ((type >> 2) & 1)

static constexpr int REGISTER_COUNT = 32;
static constexpr int SIZE_COUNT = 5;

struct Register {
    uint32_t id;
    uint8_t rType: 4;
    uint8_t status: 1;
};

enum RegisterID : uint32_t {
    RAX, RDI, RSI,
    RDX, RCX, R8,
    R9, R10, R11,
    RBP, RSP, RBX,
    R12, R13, R14, R15,
    xmm0, xmm1, xmm2,
    xmm3, xmm4, xmm5,
    xmm6, xmm7, xmm8,
    xmm9, xmm10, xmm11,
    xmm12, xmm13, xmm14,
    xmm15
};

enum RegisterSize: uint32_t {
    REG64, REG32, REG16, REG8H, REG8L
};

enum RegisterType : uint8_t {
    SSE = 1 << 0,
    SCRATCH = 1 << 1,
    PRESERVED = 1 << 2,
    PARAM = 1 << 3,
};

class RegisterAllocator {
public:
    Register* alloc(uint8_t rt = 0);

    void free(Register* reg);

    const char* nameFromReg(const Register* reg, uint32_t size);

    const char* nameFromID(uint32_t id, uint32_t size);

    Register* regFromName(const char* name, uint32_t size);

    Register* regFromID(uint32_t id);

private:
    Register* scan(const uint32_t* priorityOrder, int size);

    Register registers[REGISTER_COUNT] = {
        {.id = RAX, .rType = SCRATCH, .status = INUSE},
        {.id = RDI, .rType = SCRATCH | PARAM, .status = 0},
        {.id = RSI, .rType = SCRATCH | PARAM, .status = 0},
        {.id = RDX, .rType = SCRATCH | PARAM, .status = 0},
        {.id = RCX, .rType = SCRATCH | PARAM, .status = 0},
        {.id = R8, .rType = SCRATCH | PARAM, .status = 0},
        {.id = R9, .rType = SCRATCH | PARAM, .status = 0},
        {.id = R10, .rType = SCRATCH, .status = 0},
        {.id = R11, .rType = SCRATCH, .status = 0},
        {.id = RBP, .rType = PRESERVED, .status = INUSE},
        {.id = RSP, .rType = PRESERVED, .status = INUSE},
        {.id = RBX, .rType = PRESERVED, .status = 0},
        {.id = R12, .rType = PRESERVED, .status = 0},
        {.id = R13, .rType = PRESERVED, .status = 0},
        {.id = R14, .rType = PRESERVED, .status = 0},
        {.id = R15, .rType = PRESERVED, .status = 0},
        {.id = xmm0, .rType = SSE | PARAM, .status = INUSE},
        {.id = xmm1, .rType = SSE | PARAM, .status = 0},
        {.id = xmm2, .rType = SSE | PARAM, .status = 0},
        {.id = xmm3, .rType = SSE | PARAM, .status = 0},
        {.id = xmm4, .rType = SSE | PARAM, .status = 0},
        {.id = xmm5, .rType = SSE | PARAM, .status = 0},
        {.id = xmm6, .rType = SSE | PARAM, .status = 0},
        {.id = xmm7, .rType = SSE | PARAM, .status = 0},
        {.id = xmm8, .rType = SSE, .status = 0},
        {.id = xmm9, .rType = SSE, .status = 0},
        {.id = xmm10, .rType = SSE, .status = 0},
        {.id = xmm11, .rType = SSE, .status = 0},
        {.id = xmm12, .rType = SSE, .status = 0},
        {.id = xmm13, .rType = SSE, .status = 0},
        {.id = xmm14, .rType = SSE, .status = 0},
        {.id = xmm15, .rType = SSE, .status = 0},
    };

    static constexpr const char* registerNames[REGISTER_COUNT][SIZE_COUNT] = {
        {"rax", "eax", "ax", "ah", "al"},
        {"rdi", "edi", "di", "", "dil"},
        {"rsi", "esi", "si", "", "sil"},
        {"rdx", "edx", "dx", "dh", "dl"},
        {"rcx", "ecx", "cx", "ch", "cl"},
        {"r8", "r8d", "r8w", "", "r8b"},
        {"r9", "r9d", "r9w", "", "r9b"},
        {"r10", "r10d", "r10w", "", "r10b"},
        {"r11", "r11d", "r11w", "", "r11b"},
        {"rbp", "ebp", "bp", "", "bpl"},
        {"rsp", "esp", "sp", "", "spl"},
        {"rbx", "ebx", "bx", "bh", "bl"},
        {"r12", "r12d", "r12w", "", "r12b"},
        {"r13", "r13d", "r13w", "", "r13b"},
        {"r14", "r14d", "r14w", "", "r14b"},
        {"r15", "r15d", "r15w", "", "r15b"},
        {"xmm0", "", "", "", ""},
        {"xmm1", "", "", "", ""},
        {"xmm2", "", "", "", ""},
        {"xmm3", "", "", "", ""},
        {"xmm4", "", "", "", ""},
        {"xmm5", "", "", "", ""},
        {"xmm6", "", "", "", ""},
        {"xmm7", "", "", "", ""},
        {"xmm8", "", "", "", ""},
        {"xmm9", "", "", "", ""},
        {"xmm10", "", "", "", ""},
        {"xmm11", "", "", "", ""},
        {"xmm12", "", "", "", ""},
        {"xmm13", "", "", "", ""},
        {"xmm14", "", "", "", ""},
        {"xmm15", "", "", "", ""},
    };

    static constexpr uint32_t priorityOrder[3] = {SCRATCH, SCRATCH | PARAM, PRESERVED};

    static constexpr uint32_t priorityOrderSSE[2] = {SSE | PARAM, SSE};
};

#endif //REGISTER_H
