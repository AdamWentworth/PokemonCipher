#pragma once
#include <cstddef>

// alias for std:size_t
// size_t is designed to represent sizes and indices because its unsigned and it's very large
using ComponentTypeID = std::size_t;

// when you ahve a free function and defined in a header
// each .cpp file that includes this header componenttype.h
// would get it's own definition causing a linker error
// so using inline merges them into one definition
// global counter generator. returns a unique number everytime.
// making id static, means it will keep its value through
// multiple calls of this function. 
inline ComponentTypeID getComponentTypeID() {
    static ComponentTypeID id = 0;
    return id++;
}

// template allows us to use generic types
// can overload this function because they have different signatures
// template is inlined by default
// getComponentTypeID<Position>()-> would always return 0
// getComponentTypeID<Health()-> would always return 1
// static local variable is created and initialized once
template <typename T>
ComponentTypeID getComponentTypeID() {
    static ComponentTypeID typeID = getComponentTypeID();
    return typeID;
}
