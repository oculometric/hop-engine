#pragma once

#include <cstring>
#include <typeinfo>

namespace HopEngine
{

#define REGISTER registerCountedRef(typeid(T).name(), weak().template cast<void>())
#define UNREGISTER unregisterCountedRef(payload)

template <typename T>
class WeakRef;

void registerCountedRef(const char* type_name, WeakRef<void> reference);
void unregisterCountedRef(void* ptr);

template<typename T>
class Ref
{
	friend class WeakRef<T>;
private:
	T* payload = nullptr;
	size_t* ref_counter = nullptr;

public:
	Ref() { }
	
	Ref(const Ref& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
		if (ref_counter != nullptr)
			++(*ref_counter);
	}

	void operator=(const Ref& other)
	{
		if (other.payload == payload)
			return;

		invalidateSelf();

		payload = other.payload;
		ref_counter = other.ref_counter;
		if (ref_counter != nullptr)
			++(*ref_counter);
	}

	Ref(Ref&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;

		other.payload = nullptr;
		other.ref_counter = nullptr;
	}

	void operator=(Ref&& other) noexcept
	{
		if (other.payload == payload)
			return;

		invalidateSelf();

		payload = other.payload;
		ref_counter = other.ref_counter;

		other.payload = nullptr;
		other.ref_counter = nullptr;
	}

	Ref(WeakRef<T>& other);

	Ref(T* new_payload)
	{
		if (new_payload == payload)
			return;

		payload = new_payload;
		if (payload != nullptr)
		{
			ref_counter = new size_t;
			*ref_counter = 1;
			REGISTER;
		}
	}

	void operator=(T* new_payload)
	{
		if (new_payload == payload)
			return;

		invalidateSelf();

		payload = new_payload;
		if (payload != nullptr)
		{
			ref_counter = new size_t;
			*ref_counter = 1;
			REGISTER;
		}
	}
	
	~Ref()
	{ invalidateSelf(); }

	WeakRef<T> weak() const;
	size_t getCount() const { return *ref_counter; }
	bool isValid() const { return payload != nullptr; }
	operator bool() const { return isValid(); }
	bool operator==(const Ref<T>& other) const { return other.payload == payload; }
	bool operator==(const WeakRef<T>& other) const { return other.payload == payload; }
	T* operator->() { return payload; }
	T* operator->() const { return payload; }
	T* get() const { return payload; }

	template<typename S>
	Ref<S> cast()
	{
		Ref<S> ref;
		memcpy(static_cast<void*>(&ref), static_cast<void*>(this), sizeof(*this));
		(*ref_counter)++;
		return ref;
	}

private:
	void invalidateSelf()
	{
		if (ref_counter != nullptr && payload != nullptr)
		{
			--(*ref_counter);
			if (*ref_counter == 0)
			{
				UNREGISTER;
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
	WeakRef() { }

	WeakRef(const WeakRef& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	void operator=(const WeakRef& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	WeakRef(WeakRef&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	void operator=(WeakRef&& other) noexcept
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	WeakRef(const Ref<T>& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	void operator=(const Ref<T>& other)
	{
		if (other.payload == payload)
			return;

		payload = other.payload;
		ref_counter = other.ref_counter;
	}

	~WeakRef() { }

	size_t getCount() const { return *ref_counter; }
	bool isValid() const { return payload != nullptr; }
	operator bool() const { return isValid(); }
	bool operator==(const WeakRef<T>& other) const { return other.payload == payload; }
	bool operator==(const Ref<T>& other) const { return other.payload == payload; }
	T* operator->() { return payload; }
	T* operator->() const { return payload; }
	T* get() { return payload; }
	template<typename S>
	WeakRef<S> cast()
	{
		WeakRef<S> ref;
		memcpy(static_cast<void*>(&ref), static_cast<void*>(this), sizeof(*this));
		return ref;
	}
};

template<typename T>
Ref<T>::Ref(WeakRef<T>& other)
{
	if (other.payload == payload)
		return;

	payload = other.payload;
	ref_counter = other.ref_counter;
	if (ref_counter != nullptr)
		++(*ref_counter);
}

template<typename T>
WeakRef<T> Ref<T>::weak() const
{
    return WeakRef<T>(*this);
}

};
