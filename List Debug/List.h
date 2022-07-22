#pragma once

#include<cassert>
#include"ReferenceCount.h"
#include"Allocator.h"
#include"TypeTrait.h"


namespace EscapistPrivate
{
	template<typename Type>
	class ListCore
	{
	public:
		using Self = typename ListCore<Type>;
		using TypeTrait = typename TypeTraitPatternSelector<Type>::TypeTrait;

		static constexpr size_t MinimumCapacity = (32 / sizeof(Type));
		static constexpr bool EnableMinimumCapacity = MinimumCapacity;

		/// <summary>
		/// <para>From size, speculate the capacity of the new buffer.</para>
		/// <para>If the input size is too small, every capacity-growth might cause copy and reallcation.</para>
		/// <para>To prevent it, for objects possessing small size, there's a minimun capacity for initial buffer.</para>
		/// </summary>
		/// <param name="initialSize">input size</param>
		/// <returns></returns>
		static constexpr size_t CalculateCapacity(size_t initialSize)
		{
			if (!initialSize) // The data is nullptr.
				return 0;

			if (EnableMinimumCapacity && // If the [sizeof] is too large, it cannot have minimun capacity.
				initialSize < MinimumCapacity) // If the size is so small, every growth cause the copy and reallocation, to prevent them!!
				return MinimumCapacity;

			return initialSize * 1.5;
		}

		/// <summary>
		/// <para>Allocate the data, the capacity is only related to the parameter, rather than the member variable.</para>
		/// <para>The allocated data contains a reference count pointer, and certain continuous sized buffer.</para>
		/// </summary>
		/// <param name="initialCapacity">input capacity</param>
		void AllocateData(size_t initialCapacity)
		{
			ref = Allocator<ReferenceCount*>::Allocate(sizeof(ReferenceCount*) + initialCapacity * sizeof(Type));

			*ref = nullptr; // For new object, the reference count is unused. So must assign them as nullptr.
			data = (Type*)(ref + 1); // Ensuring the data points to the correct place.

			// PS: MUST ensure this class is always relocatable!
		}

		void InitializeCore(size_t initialSize, size_t initialCapacity)
		{
			assert(initialCapacity >= initialSize); // Check the validity.

			// Assignments
			size = initialSize;
			capacity = initialCapacity;

			AllocateData(initialCapacity);
		}

		void InitializeCore(size_t initialSize) { return InitializeCore(initialSize, CalculateCapacity(initialSize)); }

	public:
		ReferenceCount** ref;
		Type* data;
		size_t size;
		size_t capacity;

		ListCore()
			:ref(nullptr), data(nullptr), size(0), capacity(0)
		{}

		ListCore(size_t initialSize)
		{
			InitializeCore(initialSize);
		}

		ListCore(size_t initialSize, size_t initialCapacity)
		{
			InitializeCore(initialSize, initialCapacity);
		}

		ListCore(const Type* initialData, size_t initialSize)
		{
			if (initialData && initialSize)
			{
				InitializeCore(initialSize);
				TypeTrait::Copy(data, initialData, initialSize);
			}
			else
				new(this)Self();
		}

		ListCore(const Self& other)
		{
			if (other.ref && other.data && other.size)
			{
				ref = other.ref;
				data = other.data;
				size = other.size;
				capacity = other.capacity;

				if ((*ref))
				{
					(*ref)->IncrementRef();
				}
				else
				{
					Allocator<ReferenceCount>::Allocate((*ref));
					Allocator<ReferenceCount>::ParameterConstruct<const int&>((*ref), 2);
				}
			}
			else
				new(this)Self();
		}

		ListCore(const Self& other, size_t size)
		{
			if (size == other.size)
				new(this)Self(other);

			if (size && other.ref && other.data && other.size)
				new(this)Self(other.data, size);
			else
				new(this)Self();
		}

		~ListCore()
		{
			if (ref && data)
			{
				if (*ref)
				{
					if ((*ref)->IsShared())
					{
						(*ref)->DecrementRef();
						return;
					}
					else
					{
						Allocator<ReferenceCount>::Free((*ref));
					}
				}

				TypeTrait::Destroy(data, size);
				Allocator<ReferenceCount*>::Free(ref);
			}
		}

		void Detach(bool copyData)
		{
			if (ref && data && size)
			{
				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();

					Type* oldData = data;
					capacity = size * 1.5;

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
					if (copyData)
						TypeTrait::Copy(data, oldData, size);
				}
			}
		}

		void EnsureCapacity(size_t newCapacity)
		{
			if (capacity < newCapacity)
			{
				if (ref && data && size)
				{
					if ((*ref) && (*ref)->IsShared())
					{
						(*ref)->DecrementRef();

						Type* oldData = data;
						capacity = newCapacity;

						Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
						(*ref) = nullptr;
						data = (Type*)(ref + 1);
						TypeTrait::Copy(data, oldData, size);
					}
					else
					{
						ReferenceCount** old = ref;
						capacity = newCapacity;

						Allocator<ReferenceCount*>::Reallocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
						if (ref != old)
							data = (Type*)(ref + 1);
					}
				}
				else
				{
					capacity = newCapacity;
					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
				}
			}
		}

		Type* GrowthAppend(size_t growthSize)
		{
			if (!growthSize)
				return data + size;

			if (ref && data && size)
			{
				size_t oldSize = size;
				size += growthSize;

				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();

					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
					TypeTrait::Copy(data, oldData, oldSize);
				}
				else if (size > capacity)
				{
					ReferenceCount** old = ref;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Reallocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					if (ref != old)
						data = (Type*)(ref + 1);

					return data + oldSize;
					//::memcpy((void*)ref, (const void*)old, sizeof(ReferenceCount**) + size * sizeof(Type));
				}
			}
			else
			{
				size = growthSize;
				capacity = CalculateCapacity(size);
				Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
				(*ref) = nullptr;
				data = (Type*)(ref + 1);
				return data;
			}
		}

		void GrowthPrepend(size_t growthSize)
		{
			if (!growthSize)
				return;

			if (ref && data && size)
			{
				size_t oldSize = size;
				size += growthSize;

				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();

					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
					TypeTrait::Copy(data + growthSize, oldData, oldSize);
				}
				else if (size > capacity)
				{
					ReferenceCount** old = ref;
					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = (*old);
					data = (Type*)(ref + 1);
					::memcpy((void*)(data + growthSize), (const void*)oldData, oldSize * sizeof(Type));

					::free((void*)old);
				}
				else
					::memmove((void*)(data + growthSize), (const void*)data, oldSize * sizeof(Type));
			}
			else
			{
				size = growthSize;
				capacity = CalculateCapacity(size);
				Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
				(*ref) = nullptr;
				data = (Type*)(ref + 1);
			}
		}

		void GrowthInsert(size_t growthIndex, size_t growthSize)
		{
			if (!growthSize)
				return;

			if (ref && data && size)
			{
				size_t oldSize = size;
				size += growthSize;

				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();

					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
					TypeTrait::Copy(data, oldData, growthIndex);
					TypeTrait::Copy(data + growthIndex + growthSize, oldData + growthIndex, oldSize - growthIndex);
				}
				else if (size > capacity)
				{
					ReferenceCount** old = ref;
					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
					(*ref) = (*old);
					data = (Type*)(ref + 1);
					::memcpy((void*)(data + growthIndex), (const void*)oldData, growthIndex * sizeof(Type));
					::memcpy((void*)(data + growthIndex + growthSize), (const void*)oldData, (oldSize - growthIndex) * sizeof(Type));

					::free((void*)old);
				}
				else
					::memmove((void*)(data + growthIndex + growthSize), (const void*)(data + growthIndex), (oldSize - growthIndex) * sizeof(Type));
			}
			else
			{
				size = growthSize;
				capacity = CalculateCapacity(size);
				Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount**) + capacity * sizeof(Type));
				(*ref) = nullptr;
				data = (Type*)(ref + 1);
			}
		}

		void Delete(size_t index, size_t count)
		{
			if (!count)
				return;

			size_t oldSize = size;
			size -= count;

			if (ref && data && size)
			{
				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();

					Type* oldData = data;
					capacity = CalculateCapacity(size);

					Allocator<ReferenceCount*>::Allocate(ref, sizeof(ReferenceCount*) + capacity * sizeof(Type));
					(*ref) = nullptr;
					data = (Type*)(ref + 1);
					TypeTrait::Copy(data, oldData, index);
					TypeTrait::Copy(data + index, oldData + index + count, oldSize - index - count);
				}
				else
				{
					TypeTrait::Destroy(data + index, count);
					::memcpy(data + index, data + index + count, oldSize - index - count);
				}
			}
		}

		void Empty()
		{
			if (ref && data && size)
			{
				if ((*ref) && (*ref)->IsShared())
				{
					(*ref)->DecrementRef();
					new(this)Self();
				}
				else
				{
					TypeTrait::Destroy(data, size);
					size = 0; 
				}
			}
		}

		size_t IndexOf(const Type& value)const
		{
			for (size_t index = 0; index < size; ++index)
				if (TypeTrait::Equals(data[index], value))
					return index;

			return -1;
		}

		size_t LastIndexOf(const Type& value)const
		{
			for (size_t index = size - 1; index > 0; --index)
				if (TypeTrait::Equals(data[index], value))
					return index;

			return -1;
		}

		size_t IsExist(const Type& value)const { return IndexOf(value) != -1; }

		bool IsShared()const { return (*(ref))->IsShared(); }
		bool IsSharingWith(const Self& other)const { return ref == other.ref; }

		bool IsEmpty()const { return !(ref && data && size); }
		bool IsNull()const { return !(ref && data && capacity); }
		bool IsEmptyOrNull()const { return !(ref && data && size && capacity); }
	};
}

template<typename Type>
class List
{
private:
	using Self = typename List<Type>;
	using Core = typename EscapistPrivate::ListCore<Type>;
	using TypeTrait = typename TypeTraitPatternSelector<Type>::TypeTrait;

	Core core;

#ifdef _DEBUG
public:
#else
private:
#endif
	Core& GetCore() { return core; }

public:
	List() :core() {}

	List(size_t initialSize) :core(initialSize) {}

	List(size_t initialSize, size_t initialCapacity) :core(initialSize, initialCapacity) {}

	List(const Type* initialData, size_t initialSize) :core(initialData, initialSize) {}

	List(const Core& other) :core(other) {}

	List(const Core& other, size_t size) :core(other, size) {}

	List(const Self& other) :core(other.core) {}

	List(const Self& other, size_t size) :core(other.core, size) {}

	Self& Append(const Type& appendValue)
	{
		TypeTrait::Assign(core.GrowthAppend(1), appendValue);
		return *this;
	}

	Self& Append(const Type& appendValue, size_t appendCount)
	{
		TypeTrait::Fill(core.GrowthAppend(appendCount), appendValue, appendCount);
		return *this;
	}

	Self& Append(const Type* appendData, size_t dataSize)
	{
		if (appendData && dataSize)
			TypeTrait::Copy(core.GrowthAppend(dataSize), appendData, dataSize);
		return *this;
	}

	Self& Append(const Self& appendList)
	{
		if (appendList.core.ref && appendList.core.data && appendList.core.size)
		{
			if (!(core.ref && core.data && core.size))
			{
				new(this)Self(appendList);
				return *this;
			}

			TypeTrait::Copy(core.GrowthAppend(appendList.core.size), appendList.core.data, appendList.core.size);
		}
		return *this;
	}

	Self& Append(const Self& appendList, size_t listSize)
	{
		if (listSize == appendList.core.size)
			return Append(appendList);

		if (appendList.core.ref && appendList.core.data && listSize)
		{
			if (!(core.ref && core.data && core.size))
			{
				new(this)Self(appendList);
				return *this;
			}

			TypeTrait::Copy(core.GrowthAppend(listSize), appendList.core.data, listSize);
		}
		return *this;
	}

	Self& Prepend(const Type& prependValue)
	{
		core.GrowthPrepend(1);

		TypeTrait::Assign(core.data, prependValue);
		return *this;
	}

	Self& Prepend(const Type& prependValue, size_t prependCount)
	{
		core.GrowthPrepend(prependCount);

		TypeTrait::Fill(core.data, prependValue, prependCount);
		return *this;
	}

	Self& Prepend(const Type* prependData, size_t dataSize)
	{
		if (prependData && dataSize)
		{
			core.GrowthPrepend(dataSize);
			TypeTrait::Copy(core.data, prependData, dataSize);
		}
		return *this;
	}

	Self& Prepend(const Self& prependList)
	{
		if (prependList.core.ref && prependList.core.data && prependList.core.size)
		{
			if (!(core.ref && core.data && core.size))
			{
				new(this)Self(prependList);
				return *this;
			}

			core.GrowthPrepend(prependList.core.size);
			TypeTrait::Copy(core.data, prependList.core.data, prependList.core.size);
		}
		return *this;
	}

	Self& Prepend(const Self& prependList, size_t listSize)
	{
		if (listSize >= prependList.core.size)
			return Prepend(prependList);

		if (prependList.core.ref && prependList.core.data && listSize)
		{
			if (!(core.ref && core.data && core.size))
			{
				new(this)Self(prependList);
				return *this;
			}

			core.GrowthPrepend(listSize);
			TypeTrait::Copy(core.data, prependList.core.data, listSize);
		}
		return *this;
	}

	Self& Insert(size_t index, const Type& insertValue)
	{
		core.GrowthInsert(index, 1);
		TypeTrait::Assign(core.data + index, insertValue);
		return *this;
	}

	Self& Insert(size_t index, const Type& insertValue, size_t insertCount)
	{
		core.GrowthInsert(index, insertCount);
		TypeTrait::Fill(core.data + index, insertValue, insertCount);
		return *this;
	}

	Self& Insert(size_t index, const Type* insertData, size_t dataSize)
	{
		if (insertData && dataSize)
		{
			core.GrowthInsert(index, dataSize);
			TypeTrait::Copy(core.data + index, insertData, dataSize);
		}
		return *this;
	}

	Self& Insert(size_t index, const Self& insertList)
	{
		if (insertList.core.ref && insertList.core.data && insertList.core.size)
		{
			if (!(core.ref && core.data && core.size))
			{
				return *this;
			}

			core.GrowthInsert(index, insertList.core.size);
			TypeTrait::Copy(core.data + index, insertList.core.data, insertList.core.size);
		}
		return *this;
	}

	Self& Insert(size_t index, const Self& insertList, size_t listSize)
	{
		if (listSize >= insertList.core.size)
			return Insert(index, insertList);

		if (insertList.core.ref && insertList.core.data && listSize)
		{
			if (!(core.ref && core.data && core.size))
			{
				return *this;
			}

			core.GrowthInsert(index, listSize);
			TypeTrait::Copy(core.data + index, insertList.core.data, listSize);
		}
		return *this;
	}

	Self& Delete(size_t index, size_t count)
	{
		core.Delete(index, count);
		return *this;
	}

	Self& Empty()
	{
		core.Empty();
		return *this;
	}

	size_t IndexOf(const Type& findValue)const { return core.IndexOf(findValue); }
	size_t LastIndexOf(const Type& findValue)const { return core.LastIndexOf(findValue); }
	size_t IsExist(const Type& findValue)const { return core.IsExist(findValue); }

	bool IsShared()const { return core.IsShared(); }
	bool IsSharingWith(const Self& other)const { return core.IsSharingWith(other.core); }

	bool IsEmpty()const { return core.IsEmpty(); }
	bool IsNull()const { return core.IsNull(); }
	bool IsEmptyOrNull()const { return core.IsEmptyOrNull(); }

	size_t GetSize()const { return core.size; }
	size_t GetCount()const { return core.size; }
	size_t GetLength()const { return core.size; }
	size_t GetCapacity()const { return core.capacity; }

	Type* GetData()
	{
		core.Detach(true);
		return core.data;
	}
	const Type* GetConstData()const { return core.data; }

	// GetRange, Left, Right, GetLeft, GetRight, GetMiddle
};