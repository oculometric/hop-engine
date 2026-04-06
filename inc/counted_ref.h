#pragma once

#include <cstring>
#include <typeinfo>

namespace HopEngine
{

template<typename T> class WeakRef;

// signatures for engine ref register/unregister functions

void registerCountedRef(const char* type_name, const WeakRef<void>& reference);
#define REGISTER registerCountedRef(typeid(T).name(), weak().template cast<void>())
void unregisterCountedRef(const void* ptr);
#define UNREGISTER unregisterCountedRef(payload)

template<typename T> class Ref final
{
    friend class WeakRef<T>;

private:
    T* payload          = nullptr; // internal raw pointer which this `Ref` owns
    size_t* ref_counter = nullptr; // shared reference counter reflecting the number of owners

public:
    Ref() {}

    Ref(const Ref& other)
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
        // copying increases the reference count
        if (ref_counter != nullptr) ++(*ref_counter);
    }

    void operator=(const Ref& other)
    {
        if (other.payload == payload) return;

        // clear our own contents, possibly deleting them
        invalidateSelf();

        payload     = other.payload;
        ref_counter = other.ref_counter;
        // copying increases the reference count
        if (ref_counter != nullptr) ++(*ref_counter);
    }

    Ref(Ref&& other) noexcept
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;

        // null out the other just to be sure
        other.payload     = nullptr;
        other.ref_counter = nullptr;
    }

    void operator=(Ref&& other) noexcept
    {
        if (other.payload == payload) return;

        // clear our own contents, possibly deleting them
        invalidateSelf();

        payload     = other.payload;
        ref_counter = other.ref_counter;

        // null out the other just to be sure
        other.payload     = nullptr;
        other.ref_counter = nullptr;
    }

    Ref(T* new_payload)
    {
        if (new_payload == payload) return;

        // initialise the payload from a new T pointer
        payload = new_payload;
        if (payload != nullptr)
        {
            ref_counter  = new size_t;
            *ref_counter = 1;
            REGISTER;
        }
    }

    void operator=(T* new_payload)
    {
        if (new_payload == payload) return;

        // clear our own contents, possibly deleting them
        invalidateSelf();

        // initialise the payload from a new T pointer
        payload = new_payload;
        if (payload != nullptr)
        {
            ref_counter  = new size_t;
            *ref_counter = 1;
            REGISTER;
        }
    }

    ~Ref() { invalidateSelf(); }

    /**
     * @brief creates a weakened copy of the current counted object.
     * @returns `WeakRef` observing the same internal data. can be promoted back to a strong `Ref<T>`
     * later if needed.
     */
    WeakRef<T> weak() const;
    /**
     * @brief queries the number of strong `Ref` instances which own the internal raw pointer.
     * @returns number of owning `Ref`s.
     */
    size_t getCount() const { return *ref_counter; }
    /**
     * @brief checks if the internal raw pointer is valid.
     * @returns `true` if the internal raw pointer is not `nullptr`, otherwise `false`.
     */
    bool isValid() const { return payload != nullptr; }
    /**
     * @brief bool operator, mirroring `isValid`.
     * @returns `true` if the internal raw pointer is not `nullptr`, otherwise `false`.
     */
    operator bool() const { return isValid(); }
    /**
     * @brief comparison operator to another `Ref` of the same type.
     * @param other other `Ref` to compare internal pointers with.
     * @returns `true` if `other` owns the same internal raw pointer as `this`, otherwise `false`.
     */
    bool operator==(const Ref<T>& other) const { return other.payload == payload; }
    /**
     * @brief comparison operator to a raw pointer of the same type as the instance.
     * @param other raw pointer to compare to the internal pointers.
     * @returns `true` if `other` is the same as the internal raw pointer of `this`, otherwise `false`.
     */
    bool operator==(const T* other) const { return other == payload; }
    /**
     * @brief comparison operator to a `WeakRef` of the same type as the instance.
     * @param other `WeakRef` to compare internal pointers with.
     * @returns `true` if `other` observes the same internal raw pointer as `this`, otherwise `false`.
     */
    bool operator==(const WeakRef<T>& other) const { return other.payload == payload; }
    /**
     * @brief dereference operator.
     * @returns internal raw pointer for function calls or field access.
     */
    T* operator->() { return payload; }
    T* operator->() const { return payload; }
    /**
     * @brief fetches the internal raw pointer. potentially unsafe if used improperly (e.g. can easily
     * lead to multiple `Ref`s owning the same raw pointer, which then leads to double-frees and
     * use-after-frees).
     * @returns internal raw pointer for function calls or field access.
     */
    T* get() const { return payload; }

    /**
     * @brief constructs a `Ref<S>` from the data within the current `Ref<T>`, with the same counter and
     * internal raw pointer.
     * GCC was not a fan of this function.
     */
    template<typename S> Ref<S> cast()
    {
        Ref<S> ref;
        memcpy(static_cast<void*>(&ref), static_cast<void*>(this), sizeof(*this));
        (*ref_counter)++;
        return ref;
    }

private:
    /**
     * @brief invalidates the object by decrementing the reference counter (if valid) and then
     * destructing the underlying raw pointer if the reference counter is now zero.
     */
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

template<typename T> class WeakRef final
{
    friend class Ref<T>;

private:
    T* payload          = nullptr;
    size_t* ref_counter = nullptr;

public:
    WeakRef() {}

    WeakRef(const WeakRef& other)
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    void operator=(const WeakRef& other)
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    WeakRef(WeakRef&& other) noexcept
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    void operator=(WeakRef&& other) noexcept
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    WeakRef(T* new_payload)
    {
        if (new_payload == payload) return;

        payload = new_payload;
    }

    void operator=(T* other)
    {
        if (other == payload) return;

        payload     = other;
        ref_counter = nullptr;
    }

    WeakRef(const Ref<T>& other)
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    void operator=(const Ref<T>& other)
    {
        if (other.payload == payload) return;

        payload     = other.payload;
        ref_counter = other.ref_counter;
    }

    ~WeakRef() {}

    /**
     * @brief creates a strengthened copy of the current counted object. increments the internal
     * reference counter.
     * @returns `Ref` owning the internal data.
     */
    Ref<T> strong();
    /**
     * @brief queries the number of strong `Ref` instances which own the internal raw pointer.
     * @returns number of owning `Ref`s.
     */
    size_t getCount() const { return *ref_counter; }
    /**
     * @brief checks if the internal raw pointer is valid.
     * @returns `true` if the internal raw pointer is not `nullptr`, otherwise `false`.
     */
    bool isValid() const { return payload != nullptr; }
    /**
     * @brief bool operator, mirroring `isValid`.
     * @returns `true` if the internal raw pointer is not `nullptr`, otherwise `false`.
     */
    operator bool() const { return isValid(); }
    /**
     * @brief comparison operator to another `WeakRef` of the same type.
     * @param other other `WeakRef` to compare internal pointers with.
     * @returns `true` if `other` observes the same internal raw pointer as `this`, otherwise `false`.
     */
    bool operator==(const WeakRef<T>& other) const { return other.payload == payload; }
    /**
     * @brief comparison operator to a `Ref` of the same type as the instance.
     * @param other `Ref` to compare internal pointers with.
     * @returns `true` if `other` owns the same internal raw pointer as observed by `this`, otherwise
     * `false`.
     */
    bool operator==(const Ref<T>& other) const { return other.payload == payload; }
    /**
     * @brief dereference operator.
     * @returns internal raw pointer for function calls or field access.
     */
    T* operator->() { return payload; }
    T* operator->() const { return payload; }
    /**
     * @brief fetches the internal raw pointer. potentially unsafe if used improperly (e.g. can easily
     * lead to multiple `Ref`s owning the same raw pointer, which then leads to double-frees and
     * use-after-frees).
     * @returns internal raw pointer for function calls or field access.
     */
    T* get() { return payload; }

    /**
     * @brief constructs a `WeakRef<S>` from the data within the current `WeakRef<T>`, with the same
     * counter and internal raw pointer.
     */
    template<typename S> WeakRef<S> cast()
    {
        WeakRef<S> ref;
        memcpy(static_cast<void*>(&ref), static_cast<void*>(this), sizeof(*this));
        return ref;
    }
};

template<typename T> Ref<T> WeakRef<T>::strong()
{
    if (ref_counter == nullptr) return nullptr;

    Ref<T> strong_ref;
    memcpy(static_cast<void*>(&strong_ref), static_cast<void*>(this), sizeof(*this));

    ++(*ref_counter);

    return strong_ref;
}

template<typename T> WeakRef<T> Ref<T>::weak() const
{
    WeakRef<T> weakened;
    weakened = *this;
    return weakened;
}

}; // namespace HopEngine
