#pragma once

#define DELETE_CONSTRUCTORS(name) name##() = delete;\
    name##(const name##& other) = delete;\
    name##(name##&& other) = delete;\
    name##& operator=(const name##& other) = delete;\
    name##& operator=(name##&& other) = delete

#define DELETE_NOT_ALL_CONSTRUCTORS(name) name##(const name##& other) = delete;\
    name##(name##&& other) = delete;\
    name##& operator=(const name##& other) = delete;\
    name##& operator=(name##&& other) = delete

#include "counted_ref.h"
