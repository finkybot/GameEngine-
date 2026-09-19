/////////////////////////////////
// BindlessGL.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <cstdint>
#include <glad/glad.h>
/////////////////////////////////



/////////////////////////////////
// Bindless function pointer types
typedef uint64_t (*PFN_GET_TEXTURE_HANDLE)(GLuint texture);
typedef void (*PFN_MAKE_TEXTURE_HANDLE_RESIDENT)(uint64_t handle);
/////////////////////////////////



/////////////////////////////////
// GLOBAL bindless function pointers (shared across engine)
extern PFN_GET_TEXTURE_HANDLE glGetTextureHandleARB_impl;
extern PFN_MAKE_TEXTURE_HANDLE_RESIDENT glMakeTextureHandleResidentARB_impl;
/////////////////////////////////