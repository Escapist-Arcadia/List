#pragma once

#include<cassert>
#include<memory>
#include"TypeTrait.h"

template<typename Type>
class Allocator
{
public:
	static Type* Allocate()
	{
		Type* pointer = (Type*)::malloc(sizeof(Type));
		assert(pointer);
		return pointer;
	}

	static void Allocate(Type*& pointer)
	{
		pointer = (Type*)::malloc(sizeof(Type));
		assert(pointer);
	}

	static void Allocate(Type*& pointer, size_t capacity)
	{
		pointer = (Type*)::malloc(capacity);
		assert(pointer);
	}

	static Type* Allocate(size_t capacity)
	{
		Type* pointer = (Type*)::malloc(capacity);
		assert(pointer);
		return pointer;
	}

	static void Reallocate(Type*& pointer, size_t capacity)
	{
		pointer = (Type*)::realloc((void*)pointer, capacity);
		assert(pointer);
	}

	static Type* ReallocateNew(Type* input, size_t capacity)
	{
		Type* pointer = (Type*)::realloc((void*)input, capacity);
		assert(pointer);
		return pointer;
	}

	static void TypedReallocate(Type*& pointer, size_t capacity)
	{
		pointer = (Type*)::realloc((void*)pointer, capacity * sizeof(Type));
		assert(pointer);
	}

	static Type* TypedReallocateNew(Type* input, size_t capacity)
	{
		Type* pointer = (Type*)::realloc((void*)input, capacity * sizeof(Type));
		assert(pointer);
		return pointer;
	}

	static void TypedAllocate(Type*& pointer, size_t capacity)
	{
		pointer = (Type*)::malloc(capacity * sizeof(Type));
		assert(pointer);
	}

	static Type* TypedAllocate(size_t capacity)
	{
		Type* pointer = (Type*)::malloc(capacity * sizeof(Type));
		assert(pointer);
		return pointer;
	}

	static void DefaultConstruct(Type* pointer)
	{
		new(pointer)Type();
	}

	static void CopyConstruct(Type* pointer, const Type& value)
	{
		new(pointer)Type(value);
	}

	template<typename... Parameters>
	static void ParameterConstruct(Type* pointer, Parameters&&... params)
	{
		new(pointer)Type(std::forward<Parameters>(params)...);
	}

	static void Destroy(Type* pointer)
	{
		pointer->~Type();
	}

	static void Free(Type* pointer)
	{
		::free((void*)pointer);
	}
};