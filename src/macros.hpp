#pragma once

#define toHex(n) *reinterpret_cast<uint64_t*>(&n)
#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) mGeneratedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) mGeneratedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) mGeneratedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() mGeneratedCode += "\tret\n"
#define cqo() mGeneratedCode += "\tcqo\n"
#define syscall() mGeneratedCode += "\tsyscall\n"
#define lea(d, s) emitInstr2op("lea", d, s)
#define mov(d, s) emitInstr2op("mov", d, s)
#define movq(d, s) emitInstr2op("movq", d, s)
#define movsd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10, 0", s)
#define alignDirective(a) std::format("align {}", a)
#define memDirective(d, n) std::format("{} {}", d, n)

#define stackAlloc(size) do { if (size > 0) { emitInstr2op("sub", "rsp", size);mStackAllocator.alloc(size);}} while(0)
#define stackDealloc(size) do { if (size > 0) { emitInstr2op("add", "rsp", size);mStackAllocator.dealloc(size);}} while(0)
#define push(v) emitInstr1op("push", v);mStackAllocator.alloc(8)
#define pop(v) emitInstr1op("pop", v);mStackAllocator.dealloc(8)
#define leave() mGeneratedCode += "\tleave\n"; mStackAllocator.dealloc(8)

#define emitSet8L(op, reg) \
    emitInstr1op(op, mRegisterAllocator.nameFromReg(reg, RegisterSize::reg8l)); \
    movzx(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), mRegisterAllocator.nameFromReg(reg, RegisterSize::reg8l))

#define regAlloc() ([&]() { \
    auto* reg = mRegisterAllocator.alloc(RegisterType::scratch); \
    if (reg && reg->isPreserved()) { \
        push(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64)); \
    } \
    return reg; \
    }())

#define regFree(reg) do {\
    if (reg) { \
        mRegisterAllocator.free(reg); \
        if (reg->isPreserved()) { \
            pop(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64)); \
        } \
    } \
	} while(0)

