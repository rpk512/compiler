#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include "AST.h"

using namespace std;

struct CGenState {
    FunctionNode* currentFunction;
    int labelCount = 0;
    int tempIndex = 0;
    bool eliminateTailCalls;
};

CGenState state;

string ModuleNode::cgen(bool eliminateTailCalls)
{
    ostringstream out;

    state.eliminateTailCalls = eliminateTailCalls;

    out << "; vim: set syntax=nasm:\n";
    out << "%include \"lib.s\"\n";
    out << "bits 64\n";
    out << "section .text\n\n";
    
    for (shared_ptr<FunctionNode>& func : functions) {
        func->cgen(out);
    }

    out << "section .data\n";

    for (size_t i = 0; i < strings.size(); i++) {
        out << "D$" << i << ":\n";
        out << "dd " << strings[i].size() << "\n";
        out << "db '" << strings[i] << "'\n";
    }

    return out.str();
}

void FunctionNode::cgen(ostringstream& out)
{
    if (block == nullptr) {
        return;
    }

    state.currentFunction = this;

    state.labelCount = 0;

    int stackSpace = stackSpaceForArgs + temporarySpace;
    for (shared_ptr<Variable>& var : locals) {
        stackSpace += var->type->size;
    }
    
    if (id.str == "main") {
        out << "main:\n";
    } else {
        out << id.asmString() << ":\n";
    }
    out << "push rbp\n";
    out << "mov rbp, rsp\n";
    out << "sub rsp, " << stackSpace << "\n";

    if (isTailRecursive && state.eliminateTailCalls) {
        out << id.asmString() << "_tail_call:\n";
    }

    block->cgen(out);

    out << ".return:\n";
    out << "mov rsp, rbp\n";
    out << "pop rbp\n";
    out << "ret\n\n";
}

void Block::cgen(ostringstream& out)
{
    for (unique_ptr<Statement>& statement : statements) {
        statement->cgen(out);
    }
}

void Assignment::cgen(ostringstream& out)
{
    int tempLocation =
        state.currentFunction->stackSpaceForArgs + state.tempIndex * 8;
    string temp = "[rsp+" + to_string(tempLocation) + "]";
    state.tempIndex++;
    
    lhs->cgen(out, true);
    out << "mov " << temp << ", rax\n";
    rhs->cgen(out);
    out << "mov rcx, " << temp << "\n";
    out << "mov [rcx], rax\n";

    state.tempIndex--;
}

void Return::cgen(ostringstream& out)
{
    expr->cgen(out);
    out << "jmp .return\n";
}

void FunctionCall::cgen(ostringstream& out, bool genAddress)
{
    assert(!genAddress);
    cgen(out);
}

void FunctionCall::cgen(ostringstream& out)
{
    bool tailCall = state.eliminateTailCalls &&
                    state.currentFunction == function.get() &&
                    state.currentFunction->isTailRecursive;
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

void If::cgen(ostringstream& out)
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

void While::cgen(ostringstream& out)
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

//
// Expressions
//

void BinaryOpExpression::cgen(ostringstream& out, bool genAddress)
{
    int startTempIndex = state.tempIndex;
    docgen(out, genAddress);
    state.tempIndex = startTempIndex;
}

void BinaryOpExpression::docgen(ostringstream& out, bool genAddress)
{
    if (genAddress) {
        assert(op == OP_ARRAY_ACCESS);
    }

    int shortCircuitLabel;
    int tempLocation =
        state.currentFunction->stackSpaceForArgs + state.tempIndex * 8;
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
            out << "add rax," << temp << "\n";
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
            cout << "BUG: unknown op: " << op << endl;
            exit(1);
    }
}

void UnaryOpExpression::cgen(ostringstream& out, bool genAddress)
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
            cout << "BUG: unknown op: " << op << endl;
            exit(1);
    }
}

void VariableExpression::cgen(ostringstream& out, bool genAddress)
{
    if (genAddress) {
        out << "lea";
    } else {
        out << "mov";
    }
    out << " rax, [rbp+" << variable->stackOffset<< "]\n";
}

void BooleanLiteral::cgen(ostringstream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, " << value << "\n";
}

void NumericLiteral::cgen(ostringstream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, " << value << "\n";
}

void StringLiteral::cgen(ostringstream& out, bool genAddress)
{
    assert(!genAddress);
    out << "mov rax, D$" << poolIndex << "\n";
}
