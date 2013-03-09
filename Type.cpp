#include "Type.h"

bool BasicType::isCompatible(Type* type) const
{
    return type->form == TF_BASIC && ((BasicType*)type)->typeId == typeId;
}

bool ArrayType::isCompatible(Type* type) const
{
    return type->form == TF_ARRAY
            && ((ArrayType*)type)->size == size
            && ((ArrayType*)type)->base->isCompatible(base.get());
}

bool PointerType::isCompatible(Type* type) const
{
    return type->form == TF_POINTER
            && ((PointerType*)type)->base->isCompatible(base.get());
}
