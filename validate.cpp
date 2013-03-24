#include <assert.h>
#include <iostream>
#include "AST.h"
#include "SymbolTable.h"
#include "ErrorCollector.h"

// FIXME
FunctionNode* currentFunction;

string ModuleNode::validate(SymbolTable& symbols)
{
    ErrorCollector errors(sourceLines, name+".u");

    for (shared_ptr<FunctionNode>& function : functions) {
        if (symbols.getFunction(function->id) != nullptr) {
            errors.error(function->location, 
                         "Redefinition of function " + function->id.str);
        } else {
            function->validateSignature(symbols, errors);
            symbols.setFunction(function->id, function);
        }
    }
    
    bool mainFound = false;
    for (shared_ptr<FunctionNode>& function : functions) {
        function->validateBody(symbols, errors);
        
        if (!mainFound && function->id.str == "main") {
            if (!function->returnType->isVoid()
                    || !function->arguments.empty()) {
                errors.error(function->location,
                             "main must be declared as 'void main()'");
            }
            mainFound = true;
        }
    }

    return errors.getErrorString();
}

bool FunctionNode::validateSignature(SymbolTable& symbols,
                                     ErrorCollector& errors)
{
    bool valid = true;
    
    valid &= returnType->validate(symbols, errors);

    if (valid && returnType->size != 8) {
        errors.error(location, "Only 8 byte values may be returned.");
        valid = false;
    }

    // validate arguments and set them up as locals
    int argumentStackOffset = 8 * 2; // base pointer & return address
    for (unique_ptr<Declaration>& argument : arguments) {
        if (argument->type->validate(symbols, errors)) {
            shared_ptr<Variable> var(
                new Variable(argument->type,
                             argument->ids[0],
                             argumentStackOffset));
            locals.push_back(var);
            argumentStackOffset += argument->type->size;
        } else {
            valid = false;
        }
    }

    return valid;
}

bool FunctionNode::validateBody(SymbolTable& symbols,
                                ErrorCollector& errors)
{
    bool valid = true;

    if (block == nullptr) {
        return true;
    }

    // arguments are already in the locals list, put them in the symbol table
    for (shared_ptr<Variable>& var : locals) {
        symbols.setVariable(var->symbol.str, var);
    }

    for (unique_ptr<Statement>& statement : block->statements) {
        Declaration* decl = dynamic_cast<Declaration*>(statement.get());
        if (decl != nullptr) {
            decl->inOuterBlock = true;
        }
    }

    currentFunction = this;
    valid &= block->validate(symbols, errors);
    currentFunction = nullptr;

    if (!returnType->isVoid() &&
            (block->statements.empty() ||
             dynamic_cast<Return*>(block->statements.back().get()) == nullptr)) {
        errors.error(location, "Function " + id.str +
                     " does not end with a return statement");
        valid = false;
    }

    // TODO: fix this up
    // setup local variables
    if (valid) {
        int stackOffset = 0;
        for (unique_ptr<Statement>& statement : block->statements) {
            Declaration* decl = dynamic_cast<Declaration*>(statement.get());
            if (decl == nullptr) {
                continue;
            }
            for (Symbol& id : decl->ids) {
                shared_ptr<Variable> var = symbols.getVariable(id.str);
                stackOffset -= var->type->size;
                var->stackOffset = stackOffset;
                locals.push_back(var);
            }
        }
    }

    symbols.clearVariables();

    if (tailCalls.empty()) {
        isTailRecursive = false;
    }

    return valid;
}

bool Block::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;
    for (unique_ptr<Statement>& statement : statements) {
        valid &= statement->validate(symbols, errors);
    }
    return valid;
}

bool Assignment::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    if (!lhs->validate(symbols, errors)) {
        return false;
    }

    if (dynamic_cast<ArrayType*>(lhs->type.get()) != nullptr) {
        errors.error(lhs->location, "Cannot assign to an array");
        return false;
    }

    if (!lhs->isAddressable()) {
        errors.error(lhs->location, "lvalue expected");
        return false;
    }

    if (!rhs->validate(symbols, errors)) {
        return false;
    }

    if (!lhs->type->isCompatible(rhs->type.get())) {
        errors.unexpectedType(location, lhs->type.get(), rhs->type.get());
        return false;
    }

    // FIXME
    if (8 > currentFunction->temporarySpace) {
        currentFunction->temporarySpace = 8;
    }

    return true;
}

bool Declaration::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;

    if (!inOuterBlock) {
        errors.error(location, "Declarations are only allowed in outer blocks.");
        return false;
    }

    valid &= type->validate(symbols, errors);

    for (Symbol& id : ids) {
        shared_ptr<Variable> var(new Variable(type, id, 0));

        if (symbols.getVariable(id.str) != nullptr) {
            errors.error(location, "Redeclaration of variable: " + id.str);
            valid = false;
        }

        symbols.setVariable(id.str, var);
    }

    return valid;
}

bool Return::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    if (currentFunction->isTailRecursive) {
        FunctionCall* call = dynamic_cast<FunctionCall*>(expr.get());
        if (call != nullptr) {
            currentFunction->tailCalls.insert(call);
        }
    }

    bool exprIsValid = expr->validate(symbols, errors);
    if (!expr->type->isCompatible(currentFunction->returnType.get())) {
        errors.unexpectedType(location,
            currentFunction->returnType.get(), expr->type.get());
    }

    return exprIsValid;
}

bool FunctionCall::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    function = symbols.getFunction(id);

    if (function == nullptr) {
        errors.undefinedFunction(Statement::location, id.str);
        return false;
    }

    type = function->returnType;

    if (function->arguments.size() != arguments.size()) {
        errors.error(Statement::location, "Invalid number of arguments "
                     "for function call: " + id.str);
        return false;
    }

    int argsSize = 0;

    temporarySpace = 0;

    for (size_t i = 0; i < function->arguments.size(); i++) {
        shared_ptr<Type> expected = function->arguments[i]->type;
        if (!arguments[i]->validate(symbols, errors)) {
            continue;
        }

        assert(dynamic_cast<ArrayType*>(arguments[i]->type.get()) == nullptr);

        if (arguments[i]->temporarySpace > temporarySpace) {
            temporarySpace = arguments[i]->temporarySpace;
        }

        Type* actual = arguments[i]->type.get();
        if (!expected->isCompatible(actual)) {
            errors.error(Statement::location, "Argument type mismatch in call "
                         "to function: " + id.str);
            return false;
        }

        argsSize += expected->size;
    }

    if (argsSize > currentFunction->stackSpaceForArgs) {
        currentFunction->stackSpaceForArgs = argsSize;
    }

    if (currentFunction->isTailRecursive &&
            function.get() == currentFunction &&
            currentFunction->tailCalls.find(this) ==
                currentFunction->tailCalls.end()) {
        currentFunction->isTailRecursive = false;
    }

    return true;
}

bool If::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;

    if (predicate != nullptr) {
        if (predicate->validate(symbols, errors)) {
            if (!predicate->type->isBasicType(T_BOOL)) {
                errors.unexpectedType(predicate->location,
                    new BasicType(T_BOOL), predicate->type.get());
                valid = false;
            }
        } else {
            valid = false;
        }
    }

    valid &= block->validate(symbols, errors);
    
    if (elseClause != nullptr) {
        valid &= elseClause->validate(symbols, errors);
    }

    return valid;
}

bool While::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;

    if (expr != nullptr) {
        if (expr->validate(symbols, errors)) {
            if (!expr->type->isBasicType(T_BOOL)) {
                errors.unexpectedType(expr->location, new BasicType(T_BOOL),
                    expr->type.get());
                valid = false;
            }
        } else {
            valid = false;
        }
    }

    valid &= block->validate(symbols, errors);

    return valid;
}

//
// Expressions
//

bool BinaryOpExpression::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;

    valid &= lhs->validate(symbols, errors);
    valid &= rhs->validate(symbols, errors);

    if (!valid) {
        return false;
    }
    
    //             FIXME
    temporarySpace = 8 + lhs->temporarySpace + rhs->temporarySpace;
    if (temporarySpace > currentFunction->temporarySpace) {
        currentFunction->temporarySpace = temporarySpace;
    }

    if (op == OP_ARRAY_ACCESS) {
        // When LHS is a pointer to an array, automatically dereference it
        // This lets us write array_ptr[i] instead of (*array_ptr)[i]
        if (lhs->type->form == TF_POINTER &&
            ((PointerType*)lhs->type.get())->base->form == TF_ARRAY) {

            shared_ptr<Type> baseType = ((PointerType*)lhs->type.get())->base;
            lhs = unique_ptr<Expression>(new UnaryOpExpression(OP_DEREF, lhs));
            lhs->type = baseType;
        }

        if (lhs->type->form != TF_ARRAY) {
            errors.error(lhs->location, "Expected array type, found '"+
                            lhs->type->toString() +"'");
            return false;
        }
        if (!rhs->type->isBasicType(T_INT64)) {
            errors.unexpectedType(rhs->location, new BasicType(T_INT64),
                                    rhs->type.get());
            return false;
        }
        type = ((ArrayType*)lhs->type.get())->base;
        return true;
    }

    BasicTypeId operandType = T_UNKNOWN;
    BasicTypeId resultType = T_UNKNOWN;
    
    switch (op) {
        case OP_LOGICAL_OR:
        case OP_LOGICAL_AND:
            operandType = T_BOOL;
            resultType = T_BOOL;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_DIV:
        case OP_MUL:
        case OP_MOD:
            operandType = T_INT64;
            resultType = T_INT64;
            break;
        case OP_GREATER:
        case OP_GREATER_EQ:
        case OP_LESS:
        case OP_LESS_EQ:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
            operandType = T_INT64;
            resultType = T_BOOL;
            break;
        default:
            assert(false);
    }

    if (!lhs->type->isBasicType(operandType)) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + lhs->type->toString());
        return false;
    }
    
    if (!rhs->type->isBasicType(operandType)) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + rhs->type->toString());
        return false;
    }

    type.reset(new BasicType(resultType));
    return true;
}

bool UnaryOpExpression::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    if (!expr->validate(symbols, errors)) {
        return false;
    }
    
    temporarySpace = expr->temporarySpace;
    if (temporarySpace > currentFunction->temporarySpace) {
        currentFunction->temporarySpace = temporarySpace;
    }

    BasicTypeId expectedType = T_UNKNOWN;

    if (op == OP_UNARY_MINUS || op == OP_LOGICAL_NOT) {
        if (op == OP_UNARY_MINUS) {
            expectedType = T_INT64;
        } else if (op == OP_LOGICAL_NOT) {
            expectedType = T_BOOL;
        }

        if (!expr->type->isBasicType(expectedType)) {
            errors.error(location, "Operator " + unaryOpToString(op) +
                             " cannot be applied to type "
                             + expr->type->toString());
            return false;
        }
        type.reset(new BasicType(expectedType));
    } else if (op == OP_ADDRESS) {
        if (!expr->isAddressable()) {
            errors.error(expr->location, "Expression is not addressable");
            return false;
        }
        type.reset(new PointerType(expr->type));
    } else if (op == OP_DEREF) {
        if (expr->type->form != TF_POINTER) {
            errors.error(expr->location, "Expected pointer type");
            return false;
        }
        type = ((PointerType*)expr->type.get())->base;
    } else {
        assert(false);
    }

    return true;
}

bool VariableExpression::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    shared_ptr<Variable> var = symbols.getVariable(id.str);

    if (var == nullptr) {
        errors.undefinedVariable(location, id.str);
        return false;
    }

    variable = var;
    type = var->type;

    return true;
}

//
// Types
//

bool BasicType::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    typeId = symbols.getBasicTypeId(symbol.str);
    if (typeId == T_UNKNOWN) {
        errors.error(location, "Undefined type: " + symbol.str);
        return false;
    }
    return true;
}

bool ArrayType::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    return base->validate(symbols, errors);
}

bool PointerType::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    return base->validate(symbols, errors);
}
