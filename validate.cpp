#include "AST.h"
#include "SymbolTable.h"
#include "ErrorCollector.h"

// FIXME
FunctionNode* currentFunction;

string ModuleNode::validate(char** sourceLines)
{
    SymbolTable symbols;
    ErrorCollector errors(sourceLines);
    bool mainFound = false;

    for (unique_ptr<FunctionNode>& function : functions) {
        if (!mainFound && function->id.str == "main") {
            if (function->returnTypeSym.str != "void"
                    || !function->arguments.empty()) {
                errors.error(function->location,
                             "main must be declared as 'void main()'");
            }
            mainFound = true;
        }

        if (symbols.getFunction(function->id.str) != nullptr) {
            errors.error(function->location, 
                         "Redefinition of function " + function->id.str);
        } else {
            shared_ptr<FunctionType>
                ftype(new FunctionType(function, symbols));
            symbols.setFunction(function->id.str, ftype);
        }
    }
    
    for (unique_ptr<FunctionNode>& function : functions) {
        function->validate(symbols, errors);
    }

    return errors.getErrorString();
}

bool FunctionNode::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    currentFunction = this;

    Type returnType = symbols.getType(returnTypeSym.str);
    bool valid = true;

    if (returnType == T_UNKNOWN) {
        errors.error(returnTypeSym.location,
                     "Return type " + returnTypeSym.str +
                     " for function " + id.str +  " is undefined.");
        valid = false;
    }

    if (block == nullptr) {
        return valid;
    }

    // TODO: don't do this...
    if (block->statements.empty()) {
        return valid;
    }
    
    // this will add all arguments as local variables
    int argumentStackOffset = 8 * 2;
    for (unique_ptr<Declaration>& argument : arguments) {
        valid &= argument->validate(symbols, errors);
        
        shared_ptr<Variable> var = symbols.getVariable(argument->id.str);
        var->stackOffset = argumentStackOffset;
        argumentStackOffset += 8; // FIXME
    }
    
    valid &= block->validate(symbols, errors);

    // don't bother validating return expression types if
    // the return type is undefined TODO: fix this

    // TODO: this validation should be done in the return statement
    // validate method, however the method does not know the expected
    // return type atm
    if (returnType != T_UNKNOWN) {
        for (unique_ptr<Statement>& statement : block->statements) {
            Return* ret = dynamic_cast<Return*>(statement.get());
            if (ret != nullptr && ret->expr->validate(symbols, errors)) {
                Type exprType = ret->expr->getType();
                if (exprType != returnType) {
                    errors.unexpectedType(ret->expr->location,
                                          returnType, exprType);
                    valid = false;
                }
            }
        }
    }

    if (returnType != T_VOID &&
            dynamic_cast<Return*>(block->statements.back().get()) == nullptr) {
        errors.error(location, "Function " + id.str +
                     " does not end with a return statement");
        valid = false;
    }

    // setup local variables
    if (valid) {
        int stackOffset = -8;
        for (unique_ptr<Statement>& statement : block->statements) {
            Declaration* decl = dynamic_cast<Declaration*>(statement.get());
            if (decl != nullptr) {
                shared_ptr<Variable> var = symbols.getVariable(decl->id.str);
                var->stackOffset = stackOffset;
                stackOffset -= 8; // FIXME
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

    if (var->type != rhs->getType()) {
        errors.unexpectedType(location, var->type, rhs->getType());
        return false;
    }

    variable = var;

    return true;
}

bool Declaration::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = true;
    Type type = symbols.getType(typeSymbol.str);

    if (type == T_UNKNOWN) {
        errors.error(location, "Undefined type: " + typeSymbol.str);
        valid = false;
    }

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

bool FunctionCall::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    shared_ptr<FunctionType> ftype = symbols.getFunction(id.str);

    if (ftype == nullptr) {
        errors.undefinedFunction(Statement::location, id.str);
        return false;
    }
    
    type = ftype->returnType;

    if (ftype->argumentTypes.size() != arguments.size()) {
        errors.error(Statement::location, "Invalid number of arguments "
                     "for function call: " + id.str);
        return false;
    }

    int argsSize = 0;

    for (size_t i = 0; i < ftype->argumentTypes.size(); i++) {
        Type expected = ftype->argumentTypes[i];
        argsSize += 8;
        if (!arguments[i]->validate(symbols, errors)) {
            continue;
        }
        Type actual = arguments[i]->getType();
        if (expected != actual) {
            errors.error(Statement::location, "Argument type mismatch in call "
                         "to function: " + id.str);
            return false;
        }
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
            if (predicate->getType() != T_BOOL) {
                errors.unexpectedType(predicate->location,
                    T_BOOL, predicate->getType());
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
            if (expr->getType() != T_BOOL) {
                errors.unexpectedType(expr->location, T_BOOL, expr->getType());
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

    Type operandType = T_UNKNOWN;
    Type resultType = T_UNKNOWN;
    
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
    }

    if (valid && lhs->getType() != operandType) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + typeToString(lhs->getType()));
        valid = false;
    }
    
    if (valid && rhs->getType() != operandType) {
        errors.error(location, "Operator " + binaryOpToString(op) +
                         " cannot be applied to type "
                         + typeToString(rhs->getType()));
        valid = false;
    }

    if (valid) {
        type = resultType;
    }

    return valid;
}

bool UnaryOpExpression::validate(SymbolTable& symbols, ErrorCollector& errors)
{
    bool valid = expr->validate(symbols, errors);
    
    Type expectedType = T_UNKNOWN;

    if (op == OP_UNARY_MINUS) {
        expectedType = T_INT64;
    } else if (op == OP_UNARY_LOGICAL_NOT) {
        expectedType = T_BOOL;
    }

    if (valid && expectedType != expr->getType()) {
        errors.error(location, "Operator " + unaryOpToString(op) +
                         " cannot be applied to type "
                         + typeToString(expr->getType()));
        valid = false;
    }
    
    if (valid) {
        type = expectedType;
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
