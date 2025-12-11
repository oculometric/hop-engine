#pragma once

#include <cstring>

namespace HopEngine
{

template<typename T>
class Ref
{
private:
	T* payload = nullptr;
	size_t* ref_counter = nullptr;

public:
	inline Ref()
	{
	}
	
	inline Ref(const Ref& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
		if (ref_counter != nullptr)
			++(*ref_counter);
	}

	inline void operator=(const Ref& other)
	{
		if (other.payload == payload)
			return;

		invalidateSelf();

		payload = other.payload;
		ref_counter = other.ref_counter;
		if (ref_counter != nullptr)
			++(*ref_counter);
	}

	inline Ref(Ref&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;

		other.payload = nullptr;
		other.ref_counter = nullptr;
	}

	inline void operator=(Ref&& other) noexcept
	{
		if (other.payload == payload)
			return;

		invalidateSelf();

		payload = other.payload;
		ref_counter = other.ref_counter;

		other.payload = nullptr;
		other.ref_counter = nullptr;
	}

	inline Ref(T* new_payload)
	{
		if (new_payload == payload)
			return;

		payload = new_payload;
		if (payload != nullptr)
		{
			ref_counter = new size_t;
			*ref_counter = 1;
		}
	}

	inline void operator=(T* new_payload)
	{
		if (new_payload == payload)
			return;

		invalidateSelf();

		payload = new_payload;
		if (payload != nullptr)
		{
			ref_counter = new size_t;
			*ref_counter = 1;
		}
	}
	
	inline ~Ref()
	{
		invalidateSelf();
	}

	inline bool isValid() { return payload != nullptr; }
	inline T* operator->() { return payload; }
	template<typename S>
	inline Ref<S> cast()
	{
		Ref<S> ref;
		memcpy((void*)&ref, (void*)this, sizeof(*this));
		(*ref_counter)++;
		return ref;
	}
	inline T* get() { return payload; }

private:
	inline void invalidateSelf()
	{
		if (ref_counter != nullptr && payload != nullptr)
		{
			--(*ref_counter);
			if (*ref_counter == 0)
			{
				delete payload;
				delete ref_counter;
				ref_counter = nullptr;
			}
		}
	}
};

};
