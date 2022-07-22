#pragma once

#include<cassert>
#include<memory>

enum class TypeTraitPattern :short
{
	Pod,
	Generic,
	NonDefault
};

template<typename T>
constexpr bool IsComplex = !std::_Is_any_of_v<std::remove_cv_t<T>,
	bool, char, signed char, unsigned char, wchar_t, char16_t, char32_t,
	short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long,
	float, double, long double,
	void, std::nullptr_t>;

template<typename T>
struct TypeTraitPatternDefiner
{
	static const TypeTraitPattern Pattern = IsComplex<T> ? TypeTraitPattern::Generic : TypeTraitPattern::Pod;
};

template<typename T>
class TypeTrait
{
public:
	static void Copy(T* dest, const T* src, size_t size) { ::memcpy((void*)dest, (const void*)src, size * sizeof(T)); }
	static void Move(T* dest, const T* src, size_t size) { ::memmove((void*)dest, (const void*)src, size * sizeof(T)); }

	static void Assign(T* dest, const T& val) { ::memcpy((void*)dest, (const void*)&val, sizeof(T)); }
	static void Fill(T* dest, const T& val, size_t count)
	{
		for (; count > 0; --count, ++dest)
			::memcpy((void*)dest, (const void*)&val, sizeof(T));
	}

	static int Equals(const T& left, const T& right)
	{
		if (left == right)
			return 0;
		else
			if (left < right)
				return -1;
			else
				return 1;
	}
	static int Compare(const T* left, const T* right, size_t size)
	{
		int rtn;
		for (; !(rtn = TypeTrait<T>::Equals(*left, *right)) && size > 0; ++left, ++right, --size);
		return rtn;
	}

	static size_t GetSize(const T* src)
	{
		size_t count(0);
		while (!TypeTrait<T>::Equals(*src, T()))
			++count;
		return count;
	}
	static size_t GetCount(const T* src) { return GetSize(src); }
	static size_t GetLength(const T* src) { return GetSize(src); }

	static void Destroy(T* ptr) { ptr->~T(); }
	static void Destroy(T* ptr, size_t count)
	{
		for (; count > 0; ++ptr, --count)
			ptr->~T();
	}
};

namespace EscapistPrivate
{
	template<typename T>
	class PodTypeTrait :public TypeTrait<T>
	{
	public:
		static void Copy(T* dest, const T* src, size_t size) { ::memcpy((void*)dest, (const void*)src, size * sizeof(T)); }
		static void Move(T* dest, const T* src, size_t size) { ::memmove((void*)dest, (const void*)src, size * sizeof(T)); }

		static void Assign(T* dest, const T& val) { ::memcpy((void*)dest, (const void*)&val, sizeof(T)); }
		static void Fill(T* dest, const T& val, size_t count)
		{
			for (; count > 0; --count, ++dest)
				::memcpy((void*)dest, (const void*)&val, sizeof(T));
		}

		static int Equals(const T& left, const T& right) { return TypeTrait<T>::Equals(left, right); }
		static int Compare(const T* left, const T* right, size_t size) { return TypeTrait<T>::Compare(left, right, size); }

		static size_t GetSize(const T* src) { return TypeTrait<T>::GetSize(src); }
		static size_t GetCount(const T* src) { return TypeTrait<T>::GetCount(src); }
		static size_t GetLength(const T* src) { return TypeTrait<T>::GetLength(src); }

		static void Destroy(T* ptr) {}
		static void Destroy(T* ptr, size_t count) {}
	};

	template<typename T>
	class GenericTypeTrait :public TypeTrait<T>
	{
	public:
		static void Copy(T* dest, const T* src, size_t size)
		{
			for (; size > 0; ++dest, ++src)
				new(dest)T(*src);
		}
		static void Move(T* dest, const T* src, size_t size)
		{
			if (dest <= src || dest >= (src + size))
			{
				for (; size > 0; ++dest, ++src)
					new(dest)T(*src);
			}
			else
			{
				dest = dest + size - 1;
				src = src + size - 1;

				for (; size > 0; --dest, --src)
					new(dest)T(*src);
			}
		}

		static void Assign(T* dest, const T& val) { new(dest)T(val); }
		static void Fill(T* dest, const T& val, size_t count)
		{
			for (; count > 0; --count, ++dest)
				new(dest)T(val);
		}

		static int Equals(const T& left, const T& right) { return TypeTrait<T>::Equals(left, right); }
		static int Compare(const T* left, const T* right, size_t size) { return TypeTrait<T>::Compare(left, right, size); }

		static size_t GetSize(const T* src) { return TypeTrait<T>::GetSize(src); }
		static size_t GetCount(const T* src) { return TypeTrait<T>::GetCount(src); }
		static size_t GetLength(const T* src) { return TypeTrait<T>::GetLength(src); }

		static void Destroy(T* ptr) { ptr->~T(); }
		static void Destroy(T* ptr, size_t count)
		{
			for (; count > 0; ++ptr, --count)
				ptr->~T();
		}
	};
}

template<typename T, typename = void>
struct TypeTraitPatternSelector
{
	using TypeTrait = EscapistPrivate::PodTypeTrait<T>;
};

template<typename T>
struct TypeTraitPatternSelector<T,
	typename std::enable_if<(TypeTraitPatternDefiner<T>::Pattern == TypeTraitPattern::Pod)>::type>
{
	using TypeTrait = EscapistPrivate::PodTypeTrait<T>;
};

template<typename T>
struct TypeTraitPatternSelector<T,
	typename std::enable_if<(TypeTraitPatternDefiner<T>::Pattern == TypeTraitPattern::Generic)>::type>
{
	using TypeTrait = EscapistPrivate::GenericTypeTrait<T>;
};

template<typename T>
struct TypeTraitPatternSelector<T,
	typename std::enable_if<(TypeTraitPatternDefiner<T>::Pattern == TypeTraitPattern::NonDefault)>::type>
{
	using TypeTrait = TypeTrait<T>;
};

#define DefineTypeTrait(T,P) \
template<>\
struct TypeTraitPatternDefiner<T>\
{\
	static const TypeTraitPattern Pattern = P;\
}
#define DefinePodTypeTrait(T) DefineTypeTrait(T,TypeTraitPattern::Pod)
#define DefineGenericTypeTrait(T) DefineTypeTrait(T,TypeTraitPattern::Generic)
#define DefineNonDefaultTypeTrait(T) DefineTypeTrait(T,TypeTraitPattern::NonDefault)

DefinePodTypeTrait(bool);
DefinePodTypeTrait(char);
DefinePodTypeTrait(signed char);
DefinePodTypeTrait(unsigned char);
DefinePodTypeTrait(wchar_t);
DefinePodTypeTrait(char16_t);
DefinePodTypeTrait(char32_t);
DefinePodTypeTrait(short);
DefinePodTypeTrait(unsigned short);
DefinePodTypeTrait(int);
DefinePodTypeTrait(unsigned int);
DefinePodTypeTrait(long);
DefinePodTypeTrait(unsigned long);
DefinePodTypeTrait(long long);
DefinePodTypeTrait(unsigned long long);
DefinePodTypeTrait(float);
DefinePodTypeTrait(double);
DefinePodTypeTrait(long double);