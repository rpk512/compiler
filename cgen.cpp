#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include "AST.h"
#include "Flags.h"

using namespace std;

struct CGenState {
    FunctionNode* function;
    ModuleNode* module;
    int labelCount = 0;
    int tempIndex = 0;
};

CGenState state;

void startAsm(ostream& out, const string& moduleName)
{
    out << "; vim: set syntax=nasm:\n";
    out << "bits 64\n";
    out << "section .text\n";
    out << "global _start\n";
    out << "_start:\n";
    out << "    call " << moduleName << ".main\n";
    out << "    mov rax, 60\n";
    out << "    mov rdi, 0\n";
    out << "    syscall\n\n\n";
}

void ModuleNode::cgen(ostream& out)
{
    state.module = this;

    out << "section .text\n\n";
    
    for (shared_ptr<FunctionNode>& func : functions) {
        func->cgen(out);
    }

    out << "section .data\n";

    for (size_t i = 0; i < strings.size(); i++) {
        out << name << ".D$" << i << ":\n";
        out << "dd " << strings[i].size() << "\n";
        out << "db '" << strings[i] << "'\n";
    }
}

void FunctionNode::cgen(ostream& out)
{
    if (block == nullptr) {
        return;
    }

    state.function = this;

    state.labelCount = 0;

    int stackSpace = stackSpaceForArgs + stackSpaceForLocals + temporarySpace;
    
    out << id.asmString() << ":\n";
    out << "push rbp\n";
    out << "mov rbp, rsp\n";
    out << "sub rsp, " << stackSpace << "\n";

    if (isTailRecursive && Flags::eliminateTailCalls) {
        out << id.asmString() << "_tail_call:\n";
    }

    block->cgen(out);

    out << ".return:\n";
    out << "mov rsp, rbp\n";
    out << "pop rbp\n";
    out << "ret\n\n";
}

void Block::cgen(ostream& out)
{
    for (unique_ptr<Statement>& statement : statements) {
        statement->cgen(out);
    }
}

void Assignment::cgen(ostream& out)
{
    int tempLocation =
        state.function->stackSpaceForArgs + state.tempIndex * 8;
    string temp = "[rsp+" + to_string(tempLocation) + "]";
    state.tempIndex++;
    
    lhs->cgen(out, true);
    out << "mov " << temp << ", rax\n";
    rhs->cgen(out);
    out << "mov rcx, " << temp << "\n";
    out << "mov [rcx], rax\n";

    state.tempIndex--;
}

void Return::cgen(ostream& out)
{
    expr->cgen(out);
    out << "jmp .return\n";
}

void FunctionCall::cgen(ostream& out, bool genAddress)
{
    assert(!genAddress);
    cgen(out);
}

void FunctionCall::cgen(ostream& out)
{
    bool tailCall = Flags::eliminateTailCalls &&
                    state.function == function.get() &&
                    state.function->isTailRecursive;
    int stackPosition = 0;

    for (unique_ptr<Expression>& argument : arguments) {
        argument->cgen(out);
        // TODO: support things bigger than 8 bytes here
        out << "mov [rsp+" << stackPosition << "], rax\n";
        stackPosition += 8;
    }

    if (tailCall) {
        int tempPosition = 0;
        int argPosition = 8 * 2;
        for (int i = 0; i < arguments.size(); i++) {
            out << "mov rax, [rsp+" << tempPosition << "]\n";
            out << "mov [rbp+" << argPosition << "], rax\n";
            tempPosition += 8;
            argPosition += 8;
        }
        out << "jmp " << id.asmString() << "_tail_call" << "\n";
    } else {
        out << "call " << id.asmString() << "\n";
    }
}

void If::cgen(ostream& out)
{
    int end = state.labelCount++;

    If* cur = this;
    int next;

    while (cur != nullptr) {
        next = state.labelCount++;
        if (cur->predicate != nullptr) {
            cur->predicate->cgen(out);
            out << "cmp rax, 1\n";
            out << "jne .L" << next << "\n";
        }
        cur->block->cgen(out);
        cur = cur->elseClause.get();
        if (cur != nullptr) {
            out << "jmp .L" << end << "\n";
        }
        out << ".L" << next << ":\n";
    }

    out << ".L" << end << ":\n";
}

void While::cgen(ostream& out)
{
    int label = state.labelCount++;

    out << ".LOOP_START_" << label << ":\n";
    expr->cgen(out);
    out << "cmp rax, 1\n";
    out << "jne .LOOP_END_" << label << "\n";
    block->cgen(out);
    out << "jmp .LOOP_START_" << label << "\n";
    out << ".LOOP_END_" << label << ":\n";
}

void RangeFor::cgen(ostream& out)
{
    int label = state.labelCount++;

    start->cgen(out);
    out << "mov [rbp+" << var->stackOffset << "], rax\n";

    out << ".LOOP_START_" << label << ":\n";
    
    end->cgen(out);
    out << "cmp [rbp+" << var->stackOffset << "], rax\n";
    out << "jg .LOOP_END_" << label << "\n";

    block->cgen(out);
    
    out << "inc QWORD [rbp+" << var->stackOffset << "]\n";
    out << "jmp .LOOP_START_" << label << "\n";
    out << ".LOOP_END_" << label << ":\n";
}

void ArrayFor::cgen(ostream& out)
{
    assert(false);
}

//
// Expressions
//

void BinaryOpExpression::cgen(ostream& out, bool genAddress)
{
    int startTempIndex = state.tempIndex;
    docgen(out, genAddress);
    state.tempIndex = startTempIndex;
}

void BinaryOpExpression::docgen(ostream& out, bool genAddress)
{
    if (genAddress) {
        assert(op == OP_ARRAY_ACCESS);
    }

    int shortCircuitLabel;
    int tempLocation =
        state.function->stackSpaceForArgs + state.tempIndex * 8;
    string temp = "[rsp+" + to_string(tempLocation) + "]";
    state.tempIndex++;

    lhs->docgen(out, op == OP_ARRAY_ACCESS);

    if (op == OP_LOGICAL_AND || op == OP_LOGICAL_OR) {
        if (op == OP_LOGICAL_AND) {
            out << "cmp rax, 0\n";
        } else {
            out << "cmp rax, 1\n";
        }
        shortCircuitLabel = state.labelCount++;
        out << "je .L" << shortCircuitLabel << "\n";
    }

    out << "mov " << temp << ", rax\n";
    rhs->docgen(out);
    
    if (op == OP_LOGICAL_AND || op == OP_LOGICAL_OR) {
        out << ".L" << shortCircuitLabel << ":\n";
    }

    string cmov;

    switch (op) {
        case OP_EQUAL:
        case OP_NOT_EQUAL:
        case OP_GREATER:
        case OP_GREATER_EQ:
        case OP_LESS:
        case OP_LESS_EQ:
            out << "cmp " << temp << ", rax\n";
            out << "mov rax, 0\n";
            out << "mov rcx, 1\n";
            switch (op) {
                case OP_EQUAL:      cmov = "cmove";  break;
                case OP_NOT_EQUAL:  cmov = "cmovne"; break;
                case OP_GREATER:    cmov = "cmovg";  break;
                case OP_GREATER_EQ: cmov = "cmovge"; break;
                case OP_LESS:       cmov = "cmovl";  break;
                case OP_LESS_EQ:    cmov = "cmovle"; break;
            }
            out << cmov << " rax, rcx\n";
            break;
        case OP_ADD:
            out << "add rax, " << temp << "\n";
            break;
        case OP_SUB:
            out << "mov rcx, rax\n";
            out << "mov rax, " << temp << "\n";
            out << "sub rax, rcx\n";
            break;
        case OP_MUL:
            out << "imul QWORD " << temp << "\n";
            break;
        case OP_DIV:
        case OP_MOD:
            out << "mov rcx, rax\n";
            out << "mov rax, " << temp << "\n";
            out << "mov rdx, rax\n";
            out << "sar rdx, 63\n"; // fill rdx with the sign bit
            out << "idiv rcx\n";
            if (op == OP_MOD) {
                out << "mov rax, rdx\n";
            }
            break;
        case OP_LOGICAL_AND:
        case OP_LOGICAL_OR:
            break;
        case OP_ARRAY_ACCESS:
            out << "mov rcx, " << type->size << "\n";
            out << "imul rcx\n";
            out << "mov rcx, " << temp << "\n";
            out << "add rax, rcx\n";
            if (!genAddress) {
                out << "mov rax, [rax]\n";
            }
            break;
        default:
            assert(false);
    }
}

void UnaryOpExpression::cgen(ostream& out, bool genAddress)
{
    if (genAddress) {
        assert(op == OP_DEREF);
    }

    if (op == OP_ADDRESS) {
        UnaryOpExpression* uopExpr =
            dynamic_cast<UnaryOpExpression*>(expr.get());
        if (uopExpr != nullptr) {
            assert(uopExpr->op == OP_DEREF);
            expr->cgen(out);
            return;
        }

        BinaryOpExpression* bopExpr =
            dynamic_cast<BinaryOpExpression*>(expr.get());
        if (bopExpr != nullptr) {
            assert(bopExpr->op == OP_ARRAY_ACCESS);
            expr->cgen(out, true);
            return;
        }

        VariableExpression* varExpr =
            dynamic_cast<VariableExpression*>(expr.get());
        if (varExpr != nullptr) {
            out << "lea rax, [rbp+" << varExpr->variable->stackOffset << "]\n";
            return;
        }

        assert(false);
    }

    expr->cgen(out);
    switch (op) {
        case OP_LOGICAL_NOT:
            out << "xor rax, 1\n";
            break;
        case OP_UNARY_MINUS:
            out << "not rax\n";
            out << "inc rax\n";
            break;
        case OP_DEREF:
            if (!genAddress) {
                out << "mov rax, [rax]\n";
            }
            break;
        default:
            assert(false);
    }
}

void VariableExpression::cgen(ostream& out, bool genAddress)
{
    if (genAddress) {
        out << "lea";
    } else {
        out << "mov";
    }
    out << " rax, [rbp+" << variable->stackOffset<< "]\n";
}

void BooleanLiteral::cgen(ostream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, " << value << "\n";
}

void NumericLiteral::cgen(ostream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, " << value << "\n";
}

void StringLiteral::cgen(ostream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, " << state.module->name << ".D$" << poolIndex << "\n";
}
