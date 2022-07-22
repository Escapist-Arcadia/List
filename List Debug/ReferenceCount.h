#pragma once

#include<atomic>
#include<cassert>

namespace EscapistPrivate
{
	class ReferenceCount
	{
	private:
		std::atomic<int> ref;

	public:
		ReferenceCount(const int& initialValue) :ref(initialValue) {}

		void IncrementRef() { ref.fetch_add(1, std::memory_order::memory_order_acq_rel); }
		void DecrementRef() { ref.fetch_sub(1, std::memory_order::memory_order_acq_rel); }

		const int& GetValue()const { return ref.load(std::memory_order::memory_order_acquire); }

		bool IsShared()const { return GetValue() > 1; }
	};
}