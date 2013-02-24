#include "AST.h"

struct CGenState {
    int labelCount;
    int tempIndex;
};

CGenState state;

string ModuleNode::cgen()
{
    ostringstream out;

    out << "; vim: set syntax=nasm:\n";
    out << "%include \"lib.s\"\n";
    out << "bits 64\n";
    out << "section .text\n\n";
    
    for (unique_ptr<FunctionNode>& func : functions) {
        func->cgen(out);
    }

    return out.str();
}

void FunctionNode::cgen(ostringstream& out)
{
    if (block == nullptr) {
        return;
    }

    state.labelCount = 0;

    int stackSpace = locals.size() * 8 + stackSpaceForArgs;
    
    out << id.str << ": \n";
    out << "push rbp\n";
    out << "mov rbp, rsp\n";
    out << "sub rsp, " << stackSpace << "\n";

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
    rhs->cgen(out);
    out << "mov [rbp";
    if (variable->stackOffset >= 0) {
        out << "+";
    }
    out << variable->stackOffset << "], rax\n";
}

void Return::cgen(ostringstream& out)
{
    expr->cgen(out);
    out << "jmp .return\n";
}

void FunctionCall::cgen(ostringstream& out)
{
    int stackPosition = 0;

    for (unique_ptr<Expression>& argument : arguments) {
        argument->cgen(out);
        out << "mov [rsp+" << stackPosition << "], rax\n";
        stackPosition += 8;
    }

    out << "call " << id.str << "\n";
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
        out << "jmp .L" << end << "\n";
        out << ".L" << next << ":\n";
        cur = cur->elseClause.get();
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

void BinaryOpExpression::cgen(ostringstream& out)
{
    state.tempIndex= 0;
    docgen(out);
}

void BinaryOpExpression::docgen(ostringstream& out)
{
    int tempLocation = (state.tempIndex+1) * 8;
    state.tempIndex++;

    lhs->docgen(out);
    // use "red zone" above the stack for temporary space
    // FIXME: don't assume infinite space above the stack
    out << "mov [rsp-" << tempLocation << "], rax\n";
    rhs->docgen(out);
    
    string cmov;

    switch (op) {
        case OP_LESS:
        case OP_LESS_EQ:
        case OP_GREATER:
        case OP_GREATER_EQ:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
            out << "cmp [rsp-" << tempLocation << "], rax\n";
            out << "mov rax, 0\n";
            out << "mov rcx, 1\n";
            switch (op) {
                case OP_LESS:       cmov = "cmovl";  break;
                case OP_LESS_EQ:    cmov = "cmovle"; break;
                case OP_GREATER:    cmov = "cmovg";  break;
                case OP_GREATER_EQ: cmov = "cmovge"; break;
                case OP_EQUAL:      cmov = "cmove";  break;
                case OP_NOT_EQUAL:  cmov = "cmovne"; break;
            }
            out << cmov << " rax, rcx\n";
            break;
        case OP_ADD:
            out << "add rax, [rsp-" << tempLocation << "]\n";
            break;
        case OP_SUB:
            out << "mov rcx, rax\n";
            out << "mov rax, [rsp-" << tempLocation << "]\n";
            out << "sub rax, rcx\n";
            break;
    }
}

void UnaryOpExpression::cgen(ostringstream& out)
{
    
}

void VariableExpression::cgen(ostringstream& out)
{
    out << "mov rax, [rbp";
    if (variable->stackOffset >= 0) {
        out << "+";
    }
    out << variable->stackOffset<< "]\n";
}

void BooleanLiteral::cgen(ostringstream& out)
{
    out << "mov rax, " << value << "\n";
}

void NumericLiteral::cgen(ostringstream& out)
{
    out << "mov rax, " << value << "\n";
}
