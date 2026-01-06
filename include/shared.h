#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

typedef int32_t Int32;
typedef int64_t Int64;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef char Int8;
typedef uint8_t UInt8;
typedef uintptr_t UPtr;
typedef size_t Size;
typedef float Float32;
typedef double Float64;
typedef int16_t Int16;
typedef uint16_t UInt16;
typedef void Void;
typedef const Int8* CharSeq;
typedef FILE CFile;
typedef bool Bool;
typedef Void* Any;

#ifdef __GNUC__
#define pure       __attribute__((pure))
#define hot        __attribute__((hot))
#define flatten    __attribute__((flatten))
#define constfx    __attribute__((const))
#define never      __attribute__((noreturn))
#define deprecated __attribute__((deprecated))
#define unused     __attribute__((unused))
#define packed     __attribute__((packed))
#else
#define pure
#define hot
#define flatten
#define constfx
#define never
#define deprecated
#define unused
#define packed
#endif

#define null NULL
#define simple static inline
#define use(x) (Void) (x)

#endif
