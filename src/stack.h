#ifndef STACK_H
#define STACK_H

#include <string>
#include <unordered_map>
#include "parser.h"

class StackAllocator {
public:
    void alloc(uint32_t size);

    void dealloc(uint32_t size);

    int pushStackFrame(const std::string& funcName, const std::string& varName, SymbolType stype);

    [[nodiscard]] uint32_t calculateRequiredStackSize(const std::vector<ExprPtr>& args) const;

private:
    struct StackFrame {
        int currentVarOffset{8}, currentParamOffset{16};
        std::unordered_map<std::string, int> offsets;
    };

    int updateStackFrame(StackFrame* sf, const std::string& varName, SymbolType stype);

    std::unordered_map<std::string, StackFrame> stack{};
    uint32_t stackOffset{0};
};

#endif //STACK_H
