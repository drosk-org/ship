#ifndef SHIP_H
#define SHIP_H

#include "shared.h"

#define HEADER "\033[95m"
#define BLUE "\033[94m"
#define CYAN "\033[96m"
#define GREEN "\033[92m"
#define WARNING "\033[93m"
#define FAIL "\033[91m"
#define ENDC "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[90m"

#define SYM_CHECK GREEN "✔" ENDC
#define SYM_CROSS FAIL "✖" ENDC
#define SYM_ARROW CYAN "➜" ENDC
#define SYM_INFO BLUE "i" ENDC

#define CHECK SYM_CHECK
#define CROSS SYM_CROSS
#define ARROW SYM_ARROW
#define INFO SYM_INFO

/// @brief Basic string structure
typedef struct
{
    Int8* data;
    Size length;
    Size capacity;
} ShipString;

/// @brief Vector structure for generic lists
typedef struct
{
    Any* data;
    Size length;
    Size capacity;
} ShipVector;

typedef struct
{
    ShipString key;
    Any value;
} KVPair;

/// @brief Simple Map structure
typedef struct
{
    KVPair* items;
    Size count;
    Size capacity;
} ShipMap;

typedef struct
{
    ShipString stdout_str;
    ShipString stderr_str;
    Int32 returncode;
} ShipResult;

typedef enum
{
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_EQUALS,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_IDENT,
    TOKEN_CUSTOM,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_NULL,
    TOKEN_EOF
} ShipTokenType;

typedef struct
{
    ShipTokenType type;
    ShipString value;
    Int32 line;
    Float64 number_value;
    Bool bool_value;
} ShipToken;

typedef ShipResult (*ShipFunc)(ShipMap args);

typedef struct
{
    ShipString name;
    ShipString display_name;
    ShipFunc func;
} ShipRegistryEntry;

typedef struct
{
    ShipString text;
    Size pos;
    Int32 line;
    Int32 col;
} ShipLexer;

typedef struct
{
    ShipVector tokens;
    Size pos;
    ShipMap variables;
    ShipVector tasks;
    ShipString title;
} ShipParser;

typedef struct
{
    ShipFunc func;
    ShipMap args;
    ShipString task_name;
    Bool is_custom;
    ShipString custom_name;
} ShipTask;

Void string_free(ShipString* s);
ShipString string_dup(CharSeq c);
ShipString stringEmpty();
ShipString string_from(CharSeq c);

Void vectorPush(ShipVector* v, Any item);
Any vector_get(ShipVector* v, Size index);
Void vectorInit(ShipVector* v);

Void mapSet(ShipMap* m, ShipString key, Any value);
Any mapGet(ShipMap* m, ShipString key);
Void mapInit(ShipMap* m);

Void registryInit();
Void registryRegister(CharSeq name, CharSeq display_name, ShipFunc func);
ShipFunc registryGet(CharSeq name);
Bool registryExists(CharSeq name);
ShipString registryGetDisplayName(CharSeq name);

Void printHeader(CharSeq title);
ShipResult shipRun(ShipMap args);
ShipResult shipDelete(ShipMap args);
ShipResult shipMkdir(ShipMap args);
ShipResult shipCopy(ShipMap args);
ShipResult shipMove(ShipMap args);
ShipResult shipMoveAll(ShipMap args);
ShipResult shipZip(ShipMap args);
ShipResult shipList(ShipMap args);
ShipResult shipEcho(ShipMap args);

ShipVector tokenize(CharSeq content);
Void parserInit(ShipParser* p, ShipVector tokens);
Void parserParse(ShipParser* p);

Void runBuild(ShipString title, ShipVector tasks, Bool dry_run);

#endif
