#pragma once

#include <cstring>

namespace HopEngine
{

template <typename T>
class WeakRef;

template<typename T>
class Ref
{
	friend class WeakRef<T>;
private:
	T* payload = nullptr;
	size_t* ref_counter = nullptr;

public:
	inline Ref()
	{ }
	
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

	inline Ref(WeakRef<T>& other);

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

	inline bool isValid() const { return payload != nullptr; }
	inline operator bool() const { return isValid(); }
	inline bool operator==(const Ref<T>& other) { return other.payload == payload; }
	inline T* operator->() { return payload; }
	inline T* get() { return payload; }
	template<typename S>
	inline Ref<S> cast()
	{
		Ref<S> ref;
		memcpy((void*)&ref, (void*)this, sizeof(*this));
		(*ref_counter)++;
		return ref;
	}

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

template <typename T>
class WeakRef
{
	friend class Ref<T>;
private:
	T* payload = nullptr;
	size_t* ref_counter = nullptr;

public:
	inline WeakRef()
	{ }

	inline WeakRef(const WeakRef& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline void operator=(const WeakRef& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline WeakRef(WeakRef&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline void operator=(WeakRef&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline WeakRef(const Ref<T>& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline void operator=(const Ref<T>& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	inline ~WeakRef()
	{ }

	inline bool isValid() const { return payload != nullptr; }
	inline operator bool() const { return isValid(); }
	inline bool operator==(const WeakRef<T>& other) { return other.payload == payload; }
	inline T* operator->() { return payload; }
	inline T* get() { return payload; }
	template<typename S>
	inline WeakRef<S> cast()
	{
		WeakRef<S> ref;
		memcpy((void*)&ref, (void*)this, sizeof(*this));
		return ref;
	}
};

template<typename T>
inline Ref<T>::Ref(WeakRef<T>& other)
{
	if (other.payload == payload)
		return;

	payload = other.payload;
	ref_counter = other.ref_counter;
	if (ref_counter != nullptr)
		++(*ref_counter);
}

};
