#include <assert.h>
#include "AST.h"
#include "SymbolTable.h"
#include "ErrorCollector.h"

// FIXME
FunctionNode* currentFunction;

string ModuleNode::validate(char** sourceLines)
{
    SymbolTable symbols;
    ErrorCollector errors(sourceLines);

    for (shared_ptr<FunctionNode>& function : functions) {
        if (symbols.getFunction(function->id.str) != nullptr) {
            errors.error(function->location, 
                         "Redefinition of function " + function->id.str);
        } else {
            function->validateSignature(symbols, errors);
            symbols.setFunction(function->id.str, function);
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

    // validate arguments and set them up as locals
    int argumentStackOffset = 8 * 2; // base pointer & return address
    for (unique_ptr<Declaration>& argument : arguments) {
        if (argument->validate(symbols, errors)) {
            shared_ptr<Variable> var = symbols.getVariable(argument->id.str);
            var->stackOffset = argumentStackOffset;
            argumentStackOffset += argument->type->size;
        } else {
            valid = false;
        }
    }

    if (block == nullptr) {
        symbols.clearVariables();
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

    currentFunction = this;
    valid &= block->validate(symbols, errors);
    currentFunction = nullptr;

    if (!returnType->isVoid() &&
            dynamic_cast<Return*>(block->statements.back().get()) == nullptr) {
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
            if (decl != nullptr) {
                shared_ptr<Variable> var = symbols.getVariable(decl->id.str);
                stackOffset -= var->type->size;
                var->stackOffset = stackOffset;
                locals.push_back(var);
            }
        }
    }

    // TODO: ensure declarations only occur in the outermost block

    symbols.clearVariables();

    return valid;
}

bool Block::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;
    for (const unique_ptr<Statement>& statement : statements) {
        valid &= statement->validate(symbols, errors);
    }
    return valid;
}

bool Assignment::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    shared_ptr<Variable> var = symbols.getVariable(id.str);

    if (var == nullptr) {
        errors.undefinedVariable(location, id.str);
        return false;
    }

    if (!rhs->validate(symbols, errors)) {
        return false;
    }

    if (!var->type->isCompatible(rhs->type.get())) {
        errors.unexpectedType(location, var->type.get(), rhs->type.get());
        return false;
    }

    variable = var;

    return true;
}

bool Declaration::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;

    valid &= type->validate(symbols, errors);

    shared_ptr<Variable> var(new Variable());
    var->type = type;

    if (symbols.getVariable(id.str) != nullptr) {
        errors.error(location, "Redeclaration of variable: " + id.str);
        return false;
    }

    symbols.setVariable(id.str, var);

    // TODO: inital expression validation once the syntax is implemented

    return valid;
}

bool Return::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool exprIsValid = expr->validate(symbols, errors);
    if (!expr->type->isCompatible(currentFunction->returnType.get())) {
        errors.unexpectedType(location,
            currentFunction->returnType.get(), expr->type.get());
    }
    return exprIsValid;
}

bool FunctionCall::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    shared_ptr<FunctionNode> function = symbols.getFunction(id.str);

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

    //             FIXME
    temporarySpace = 8 + lhs->temporarySpace + rhs->temporarySpace;
    if (temporarySpace > currentFunction->temporarySpace) {
        currentFunction->temporarySpace = temporarySpace;
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

    if (valid && !lhs->type->isBasicType(operandType)) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + lhs->type->toString());
        valid = false;
    }
    
    if (valid && !rhs->type->isBasicType(operandType)) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + rhs->type->toString());
        valid = false;
    }

    if (valid) {
        type.reset(new BasicType(resultType));
    }

    return valid;
}

bool UnaryOpExpression::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = expr->validate(symbols, errors);
    
    temporarySpace = expr->temporarySpace;
    if (temporarySpace > currentFunction->temporarySpace) {
        currentFunction->temporarySpace = temporarySpace;
    }

    BasicTypeId expectedType = T_UNKNOWN;

    if (op == OP_UNARY_MINUS) {
        expectedType = T_INT64;
    } else if (op == OP_UNARY_LOGICAL_NOT) {
        expectedType = T_BOOL;
    }

    if (valid && !expr->type->isBasicType(expectedType)) {
        errors.error(location, "Operator " + unaryOpToString(op) +
                         " cannot be applied to type "
                         + expr->type->toString());
        valid = false;
    }
    
    if (valid) {
        type.reset(new BasicType(expectedType));
    }

    return valid;
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
