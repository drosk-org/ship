#include "ship.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP '\\'
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEP '/'
#endif

static ShipVector global_registry;

/// @brief Create a new String from C string
ShipString stringFrom(CharSeq c)
{
    ShipString s;
    if(c == null)
    {
        s.length = 0;
        s.capacity = 1;
        s.data = (Int8*)malloc(1);
        s.data[0] = '\0';
        return s;
    }
    s.length = strlen(c);
    s.capacity = s.length + 1;
    s.data = (Int8*)malloc(s.capacity);
    strcpy(s.data, c);
    return s;
}

/// @brief Free string memory
Void stringFree(ShipString* s)
{
    if(s->data)
    {
        free(s->data);
        s->data = null;
    }
    s->length = 0;
    s->capacity = 0;
}

/// @brief definition of mapFind
KVPair* mapFind(ShipMap* m, ShipString key)
{
    for(Size i = 0; i < m->count; i++)
    {
        if(strcmp(m->items[i].key.data, key.data) == 0)
        {
            return &m->items[i];
        }
    }
    return null;
}

/// @brief Initialize registry
Void registryInit()
{
    global_registry.length = 0;
    global_registry.capacity = 0;
    global_registry.data = null;
}

/// @brief Register a function
Void registryRegister(CharSeq name, CharSeq display_name, ShipFunc func)
{
    ShipRegistryEntry* entry = (ShipRegistryEntry*)malloc(sizeof(ShipRegistryEntry));
    entry->name = stringFrom(name);
    entry->display_name = stringFrom(display_name ? display_name : name);
    entry->func = func;
    vectorPush(&global_registry, entry);
}

/// @brief Get function by name
ShipFunc registryGet(CharSeq name)
{
    for(Size i = 0; i < global_registry.length; i++)
    {
        ShipRegistryEntry* entry = (ShipRegistryEntry*)global_registry.data[i];
        if(strcmp(entry->name.data, name) == 0)
        {
            return entry->func;
        }
    }
    return null;
}

/// @brief Check if function exists
Bool registryExists(CharSeq name)
{
    return registryGet(name) != null;
}

/// @brief Get display name
ShipString registryGetDisplayName(CharSeq name)
{
    for(Size i = 0; i < global_registry.length; i++)
    {
        ShipRegistryEntry* entry = (ShipRegistryEntry*)global_registry.data[i];
        if(strcmp(entry->name.data, name) == 0)
        {
            return stringFrom(entry->display_name.data);
        }
    }
    return stringFrom(name);
}

/// @brief Vector implementations
Void vectorInit(ShipVector* v)
{
    v->length = 0;
    v->capacity = 0;
    v->data = null;
}

Void vectorPush(ShipVector* v, Any item)
{
    if(v->length >= v->capacity)
    {
        v->capacity = v->capacity == 0 ? 4 : v->capacity * 2;
        v->data = (Any*)realloc(v->data, v->capacity * sizeof(Any));
    }
    v->data[v->length++] = item;
}

/// @brief Map implementations
Void mapInit(ShipMap* m)
{
    m->count = 0;
    m->capacity = 0;
    m->items = null;
}

Void mapSet(ShipMap* m, ShipString key, Any value)
{
    KVPair* existing = mapFind(m, key);
    if(existing)
    {
        existing->value = value;
        return;
    }
    if(m->count >= m->capacity)
    {
        m->capacity = m->capacity == 0 ? 4 : m->capacity * 2;
        m->items = (KVPair*)realloc(m->items, m->capacity * sizeof(KVPair));
    }
    m->items[m->count].key = stringFrom(key.data);
    m->items[m->count].value = value;
    m->count++;
}

Any mapGet(ShipMap* m, ShipString key)
{
    KVPair* pair = mapFind(m, key);
    return pair ? pair->value : null;
}

Void lexerInit(ShipLexer* l, CharSeq content)
{
    l->text = stringFrom(content);
    l->pos = 0;
    l->line = 1;
    l->col = 1;
}

Int8 lexerPeek(ShipLexer* l, Int32 offset)
{
    Size p = l->pos + offset;
    if(p >= l->text.length)
    {
        return '\0';
    }
    return l->text.data[p];
}

Void lexerAdvance(ShipLexer* l)
{
    if(l->pos < l->text.length)
    {
        if(l->text.data[l->pos] == '\n')
        {
            l->line++;
            l->col = 1;
        }
        else
        {
            l->col++;
        }
        l->pos++;
    }
}

Void lexerSkipWhitespace(ShipLexer* l)
{
    while(l->pos < l->text.length)
    {
        Int8 c = lexerPeek(l, 0);
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            lexerAdvance(l);
        }
        else if(c == '/' && lexerPeek(l, 1) == '/')
        {
            while(l->pos < l->text.length && lexerPeek(l, 0) != '\n')
            {
                lexerAdvance(l);
            }
        }
        else if(c == '/' && lexerPeek(l, 1) == '*')
        {
            lexerAdvance(l);
            lexerAdvance(l);
            while(l->pos < l->text.length)
            {
                if(lexerPeek(l, 0) == '*' && lexerPeek(l, 1) == '/')
                {
                    lexerAdvance(l);
                    lexerAdvance(l);
                    break;
                }
                lexerAdvance(l);
            }
        }
        else if(c == '#')
        {
            while(l->pos < l->text.length && lexerPeek(l, 0) != '\n')
            {
                lexerAdvance(l);
            }
        }
        else
        {
            break;
        }
    }
}

ShipString lexerReadString(ShipLexer* l, Int8 quote)
{
    lexerAdvance(l);
    Size start = l->pos;
    Size len = 0;
    Size temp_pos = l->pos;
    while(temp_pos < l->text.length)
    {
        Int8 c = l->text.data[temp_pos];
        if(c == quote)
        {
            break;
        }
        if(c == '\\')
        {
            temp_pos++;
        }
        temp_pos++;
        len++;
    }

    ShipString s;
    s.capacity = len + 100;
    s.data = (Int8*)malloc(s.capacity);
    Size out_idx = 0;

    while(l->pos < l->text.length)
    {
        Int8 c = lexerPeek(l, 0);
        if(c == quote)
        {
            lexerAdvance(l);
            break;
        }
        else if(c == '\\')
        {
            lexerAdvance(l);
            Int8 escaped = lexerPeek(l, 0);
            if(escaped == 'n') s.data[out_idx++] = '\n';
            else if(escaped == 't') s.data[out_idx++] = '\t';
            else s.data[out_idx++] = escaped;
            lexerAdvance(l);
        }
        else
        {
            s.data[out_idx++] = c;
            lexerAdvance(l);
        }
    }
    s.data[out_idx] = '\0';
    s.length = out_idx;
    return s;
}

ShipString lexerReadIdent(ShipLexer* l)
{
    Size start = l->pos;
    Size len = 0;
    while(l->pos < l->text.length)
    {
        Int8 c = lexerPeek(l, 0);
        if(isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/')
        {
            len++;
            lexerAdvance(l);
        }
        else
        {
            break;
        }
    }
    ShipString s;
    s.length = len;
    s.capacity = len + 1;
    s.data = (Int8*)malloc(s.capacity);
    strncpy(s.data, l->text.data + start, len);
    s.data[len] = '\0';
    return s;
}

ShipToken lexerNext(ShipLexer* l)
{
    lexerSkipWhitespace(l);
    ShipToken t;
    t.line = l->line;
    t.value = stringFrom("");
    t.bool_value = false;
    t.number_value = 0;

    if(l->pos >= l->text.length)
    {
        t.type = TOKEN_EOF;
        return t;
    }

    Int8 c = lexerPeek(l, 0);
    Int8 n = lexerPeek(l, 1);

    if(c == '=' && n == '=')
    {
        t.type = TOKEN_EQ;
        t.value = stringFrom("==");
        lexerAdvance(l);
        lexerAdvance(l);
    }
    else if(c == '!' && n == '=')
    {
        t.type = TOKEN_NE;
        t.value = stringFrom("!=");
        lexerAdvance(l);
        lexerAdvance(l);
    }
    else if(c == '<' && n == '=') { t.type = TOKEN_LE; t.value = stringFrom("<="); lexerAdvance(l); lexerAdvance(l); }
    else if(c == '>' && n == '=') { t.type = TOKEN_GE; t.value = stringFrom(">="); lexerAdvance(l); lexerAdvance(l); }
    else if(c == '&' && n == '&') { t.type = TOKEN_AND; t.value = stringFrom("&&"); lexerAdvance(l); lexerAdvance(l); }
    else if(c == '|' && n == '|')
    {
        t.type = TOKEN_OR; t.value = stringFrom("||"); lexerAdvance(l); lexerAdvance(l); }
    else if(c == '{') { t.type = TOKEN_LBRACE; t.value = stringFrom("{"); lexerAdvance(l); }
    else if(c == '}') { t.type = TOKEN_RBRACE; t.value = stringFrom("}"); lexerAdvance(l); }
    else if(c == '(') { t.type = TOKEN_LPAREN; t.value = stringFrom("("); lexerAdvance(l); }
    else if(c == ')') { t.type = TOKEN_RPAREN; t.value = stringFrom(")"); lexerAdvance(l); }
    else if(c == ':') { t.type = TOKEN_COLON; t.value = stringFrom(":"); lexerAdvance(l); }
    else if(c == ',') { t.type = TOKEN_COMMA; t.value = stringFrom(","); lexerAdvance(l); }
    else if(c == '=') { t.type = TOKEN_EQUALS; t.value = stringFrom("="); lexerAdvance(l); }
    else if(c == '<') { t.type = TOKEN_LT; t.value = stringFrom("<"); lexerAdvance(l); }
    else if(c == '>') { t.type = TOKEN_GT; t.value = stringFrom(">"); lexerAdvance(l); }
    else if(c == '!') { t.type = TOKEN_NOT; t.value = stringFrom("!"); lexerAdvance(l); }
    else if(c == '"' || c == '\'')
    {
        t.type = TOKEN_STRING;
        stringFree(&t.value);
        t.value = lexerReadString(l, c);
    }
    else if(c == '$')
    {
        lexerAdvance(l);
        t.type = TOKEN_CUSTOM;
        stringFree(&t.value);
        t.value = lexerReadIdent(l);
    }
    else if(isdigit(c) || (c == '-' && isdigit(n)))
    {
        t.type = TOKEN_NUMBER;
        Bool neg = false;
        if(c == '-') { neg = true; lexerAdvance(l); }
        Size start = l->pos;
        while(l->pos < l->text.length)
        {
            Int8 ch = lexerPeek(l, 0);
            if(isdigit(ch) || ch == '.') lexerAdvance(l);
            else break;
        }
        Size len = l->pos - start;
        Int8 buf[64];
        strncpy(buf, l->text.data + start, len < 63 ? len : 63);
        buf[len < 63 ? len : 63] = 0;
        t.number_value = atof(buf);
        if(neg) t.number_value = -t.number_value;
    }
    else if(isalpha(c) || c == '_')
    {
        ShipString id = lexerReadIdent(l);
        if(strcmp(id.data, "true") == 0 || strcmp(id.data, "True") == 0)
        {
            t.type = TOKEN_BOOL; t.bool_value = true;
        }
        else if(strcmp(id.data, "false") == 0 || strcmp(id.data, "False") == 0)
        {
            t.type = TOKEN_BOOL; t.bool_value = false;
        }
        else if(strcmp(id.data, "null") == 0 || strcmp(id.data, "none") == 0)
        {
            t.type = TOKEN_NULL;
        }
        else
        {
            t.type = TOKEN_IDENT;
            stringFree(&t.value);
            t.value = id;
            return t;
        }
        stringFree(&id);
    }
    else
    {
        lexerAdvance(l);
    }
    return t;
}

ShipVector tokenize(CharSeq content)
{
    ShipVector tokens;
    vectorInit(&tokens);
    ShipLexer l;
    lexer_init(&l, content);
    while(true)
    {
        ShipToken t = lexerNext(&l);
        ShipToken* tp = (ShipToken*)malloc(sizeof(ShipToken));
        *tp = t;
        vectorPush(&tokens, tp);
        if(t.type == TOKEN_EOF)
        {
            break;
        }
    }
    return tokens;
}

Void parserInit(ShipParser* p, ShipVector tokens)
{
    p->tokens = tokens;
    p->pos = 0;
    mapInit(&p->variables);
    vectorInit(&p->tasks);
    p->title = stringFrom("Ship Build");
}

ShipToken* parserPeek(ShipParser* p, Int32 offset)
{
    Size idx = p->pos + offset;
    if(idx >= p->tokens.length)
    {
        return (ShipToken*)p->tokens.data[p->tokens.length - 1];
    }
    return (ShipToken*)p->tokens.data[idx];
}

ShipToken* parserCurrent(ShipParser* p)
{
    return parser_peek(p, 0);
}

Void parserAdvance(ShipParser* p)
{
    p->pos++;
}

Void parserExpect(ShipParser* p, ShipTokenType type)
{
    ShipToken* t = parserCurrent(p);
    if(t->type != type)
    {
        fprintf(stderr, FAIL "Syntax Error: Expected token type %d but got %d at line %d\n" ENDC, type, t->type, t->line);
        exit(1);
    }
    parseAdvance(p);
}

Any parserParseExpression(ShipParser* p);
Void parserSkipBlock(ShipParser* p);
ShipVector parserParseBlockBody(ShipParser* p);

Any parser_resolve(ShipParser* p, ShipString name)
{
    Any val = mapGet(&p->variables, name);
    if(val) return val;
    ShipString* s = (ShipString*)malloc(sizeof(ShipString));
    *s = stringFrom(name.data);
    return s;
}

Any parserParsePrimary(ShipParser* p)
{
    ShipToken* t = parserCurrent(p);
    parseAdvance(p);
    if(t->type == TOKEN_STRING)
    {
        ShipString* s = (ShipString*)malloc(sizeof(ShipString));
        *s = stringFrom(t->value.data);
        return s;
    }
    if(t->type == TOKEN_NUMBER)
    {
        Float64* f = (Float64*)malloc(sizeof(Float64));
        *f = t->number_value;
        return f;
    }
    if(t->type == TOKEN_BOOL)
    {
        Bool* b = (Bool*)malloc(sizeof(Bool));
        *b = t->bool_value;
        return b;
    }
    if(t->type == TOKEN_NULL) return null;
    if(t->type == TOKEN_IDENT)
    {
        Any v = mapGet(&p->variables, t->value);
        if(v)
        {
            return v;
        }
        ShipString* s = (ShipString*)malloc(sizeof(ShipString));
        *s = stringFrom(t->value.data);
        return s;
    }
    if(t->type == TOKEN_LPAREN)
    {
        Any val = parserParseExpression(p);
        parserExpect(p, TOKEN_RPAREN);
        return val;
    }
    return null;
}

Bool toBool(Any val)
{
    return !val ? false : true;
}

Any parserParseExpression(ShipParser* p)
{
    return parserParsePrimary(p);
}

ShipMap parserParseFuncArgs(ShipParser* p)
{
    ShipMap args;
    mapInit(&args);
    parserExpect(p, TOKEN_LBRACE);
    while(parserCurrent(p)->type != TOKEN_RBRACE)
    {
        ShipToken* key_tok = parserCurrent(p);
        if(key_tok->type != TOKEN_IDENT && key_tok->type != TOKEN_STRING)
        {
            fprintf(stderr, "Expected arg name\n");
            exit(1);
        }
        ShipString key = stringFrom(key_tok->value.data);
        parseAdvance(p);
        if(parserCurrent(p)->type != TOKEN_COLON)
        {
            fprintf(stderr, "Expected : after arg name\n");
            exit(1);
        }
        parseAdvance(p);
        Any val = parserParseExpression(p);
        mapSet(&args, key, val);
        stringFree(&key);
        if(parserCurrent(p)->type == TOKEN_COMMA)
        {
            parseAdvance(p);
        }
    }
    parserExpect(p, TOKEN_RBRACE);
    return args;
}

Void parserParseVarBlock(ShipParser* p)
{
    parserExpect(p, TOKEN_LBRACE);
    while(parserCurrent(p)->type != TOKEN_RBRACE)
    {
        ShipToken* name_tok = parserCurrent(p);
        if(name_tok->type != TOKEN_IDENT)
        {
            parseAdvance(p);
            continue;
        }
        ShipString name = stringFrom(name_tok->value.data);
        parseAdvance(p);

        if(parserCurrent(p)->type != TOKEN_EQUALS)
        {
            parseAdvance(p);
            continue;
        }
        parseAdvance(p);

        Any val = parserParseExpression(p);
        mapSet(&p->variables, name, val);
        if(parserCurrent(p)->type == TOKEN_COMMA)
        {
            parseAdvance(p);
        }
    }
    parserExpect(p, TOKEN_RBRACE);
}

Void parserSkipBlock(ShipParser* p)
{
    Int32 depth = 1;
    while(depth > 0 && parserCurrent(p)->type != TOKEN_EOF)
    {
        ShipToken* t = parserCurrent(p);
        if(t->type == TOKEN_LBRACE)
        {
            depth++;
        }
        else if(t->type == TOKEN_RBRACE)
        {
            depth--;
        }
        parseAdvance(p);
    }
}

ShipVector parserParseBlockBody(ShipParser* p)
{
    ShipVector tasks;
    vectorInit(&tasks);
    while(parserCurrent(p)->type != TOKEN_RBRACE && parserCurrent(p)->type != TOKEN_EOF)
    {
        ShipToken* t = parserCurrent(p);
        if(t->type == TOKEN_IDENT)
        {
            ShipString ident = t->value;
            parseAdvance(p);

            if(strcmp(ident.data, "title") == 0)
            {
                if(parserCurrent(p)->type == TOKEN_COLON) parseAdvance(p);
                Any val = parserParseExpression(p);
                if(val) p->title = stringFrom(((ShipString*)val)->data);
            }
            else if(strcmp(ident.data, "var") == 0)
            {
                parserParseVarBlock(p);
            }
            else if(strcmp(ident.data, "if") == 0)
            {
                Any cond = parserParseExpression(p);
                parserExpect(p, TOKEN_LBRACE);
                if(toBool(cond))
                {
                    ShipVector block = parserParseBlockBody(p);
                    for(Size i=0; i<block.length; i++)
                    {
                        vectorPush(&tasks, block.data[i]);
                    }
                }
                else
                {
                    parserSkipBlock(p);
                }
                if(parserCurrent(p)->type == TOKEN_RBRACE) parseAdvance(p);
            }
            else if(registryExists(ident.data))
            {
                ShipFunc func = registryGet(ident.data);
                ShipMap args = parserParseFuncArgs(p);
                ShipTask* tsk = (ShipTask*) malloc(sizeof(ShipTask));
                tsk->func = func;
                tsk->args = args;
                tsk->task_name = stringFrom(ident.data);
                tsk->is_custom = false;
                vectorPush(&tasks, tsk);
            }
            else
            {
                if(parserCurrent(p)->type == TOKEN_LBRACE)
                {
                    parseAdvance(p);
                    parserSkipBlock(p);
                }
            }
        }
        else if(t->type == TOKEN_CUSTOM)
        {
             ShipString name = t->value;
             parseAdvance(p);
             if(parserCurrent(p)->type == TOKEN_LBRACE)
             {
                 parseAdvance(p);
                 parserSkipBlock(p);
             }
             printf(DIM "Custom task: $%s\n" ENDC, name.data);
        }
        else
        {
            parseAdvance(p);
        }
    }
    return tasks;
}

Void parserParse(ShipParser* p)
{
    ShipToken* t = parserCurrent(p);
    if(t->type == TOKEN_IDENT && strcmp(t->value.data, "ship") == 0)
    {
        parseAdvance(p);
        parserExpect(p, TOKEN_LBRACE);
        p->tasks = parserParseBlockBody(p);
        parserExpect(p, TOKEN_RBRACE);
    }
    else
    {
        p->tasks = parserParseBlockBody(p);
    }
}

ShipResult shipRun(ShipMap args)
{
    ShipString* cmd = (ShipString*) mapGet(&args, stringFrom("command"));
    ShipResult res;
    res.stdout_str = stringEmpty();
    res.stderr_str = stringEmpty();
    if(!cmd)
    {
        res.returncode = -1;
        return res;
    }
    res.returncode = system(cmd->data);
    return res;
}

ShipResult shipDelete(ShipMap args)
{
    ShipString* path = (ShipString*) mapGet(&args, stringFrom("path"));
    ShipResult res;
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(path)
    {
        Int8 buffer[1024];
        snprintf(buffer, 1024, "rm -rf \"%s\"", path->data);
        res.returncode = system(buffer);
    }
    return res;
}

ShipResult shipMkdir(ShipMap args)
{
    ShipString* path = (ShipString*) mapGet(&args, stringFrom("path"));
    ShipResult res;
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(path)
    {
        Int8 buffer[1024];
        snprintf(buffer, 1024, "mkdir -p \"%s\"", path->data);
        res.returncode = system(buffer);
    }
    return res;
}

ShipResult shipCopy(ShipMap args)
{
    ShipString* src = (ShipString*) mapGet(&args, stringFrom("src"));
    ShipString* dst = (ShipString*) mapGet(&args, stringFrom("dst"));
    ShipResult res = {0};
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(src && dst)
    {
        Int8 buffer[2048];
        snprintf(buffer, 2048, "cp -r \"%s\" \"%s\"", src->data, dst->data);
        res.returncode = system(buffer);
    }
    return res;
}
ShipResult shipMove(ShipMap args)
{
    ShipString* src = (ShipString*) mapGet(&args, stringFrom("src"));
    ShipString* dst = (ShipString*) mapGet(&args, stringFrom("dst"));
    ShipResult res = {0};
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(src && dst)
    {
        Int8 buffer[2048];
        snprintf(buffer, 2048, "mv \"%s\" \"%s\"", src->data, dst->data);
        res.returncode = system(buffer);
    }
    return res;
}

ShipResult shipZip(ShipMap args)
{
    ShipResult res = {0};
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    return res;
}

ShipResult shipList(ShipMap args)
{
    ShipString* path = (ShipString*) mapGet(&args, stringFrom("path"));
    ShipResult res = {0};
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(path)
    {
        Int8 buffer[1024];
        snprintf(buffer, 1024, "ls \"%s\"", path->data);
        res.returncode = system(buffer);
    }
    return res;
}

ShipResult shipMoveAll(ShipMap args)
{
    ShipString* src = (ShipString*)mapGet(&args, stringFrom("src"));
    ShipString* dst = (ShipString*)mapGet(&args, stringFrom("dst"));
    ShipResult res = {0};
    res.returncode = 0;
    res.stdout_str = stringFrom("");
    res.stderr_str = stringFrom("");
    if(src && dst)
    {
        Int8 buffer[2048];
        snprintf(buffer, 2048, "mv \"%s\"/* \"%s\"", src->data, dst->data);
        res.returncode = system(buffer);
    }
    return res;
}

ShipResult shipEcho(ShipMap args)
{
    ShipString* msg = (ShipString*)mapGet(&args, stringFrom("message"));
    if(msg)
    {
        printf("  " CYAN ">" ENDC " %s\n", msg->data);
    }
    ShipResult r;
    r.returncode = 0;
    r.stdout_str = stringFrom(msg ? msg->data : "");
    r.stderr_str = stringFrom("");
    return r;
}

ShipString stringEmpty()
{
    return stringFrom("");
}

Void runBuild(ShipString title, ShipVector tasks, Bool dry_run)
{
    printHeader(title.data);
    printf(BOLD "Plan: %lu steps to execute." ENDC "\n\n", (UInt64)tasks.length);
    for(Size i = 0; i < tasks.length; i++)
    {
        ShipTask* t = (ShipTask*)tasks.data[i];
        // Fix for potential NULL task_name
        Int8* tname = t->task_name.data;
        if (!tname) tname = "Unknown Task";
        printf(DIM "[%lu/%lu]" ENDC " " INFO " %s...\n", (UInt64)(i+1), (UInt64)tasks.length, tname);

        if(!dry_run)
        {
            ShipResult res = t->func(t->args);
            if(res.returncode == 0)
            {
                printf(DIM "[%lu/%lu]" ENDC " " CHECK " %s " DIM "(Done)" ENDC "\n", (UInt64)(i+1), (UInt64)tasks.length, tname);
            }
            else
            {
                printf(FAIL "Failed!\n" ENDC);
                break;
            }
        }
    }
}

Void printHeader(CharSeq title)
{
    printf("\n" HEADER BOLD "============================================================" ENDC "\n");
    printf(HEADER BOLD "   %s" ENDC "\n", title);
    printf(HEADER BOLD "============================================================" ENDC "\n\n");
}

int main(int argc, Int8** argv)
{
    registryInit();
    registryRegister("run", "Run Command", shipRun);
    registryRegister("delete", "Delete", shipDelete);
    registryRegister("mkdir", "Create Directory", shipMkdir);
    registryRegister("copy", "Copy", shipCopy);
    registryRegister("move", "Move", shipMove);
    registryRegister("move_all", "Move Contents", shipMoveAll);
    registryRegister("zip", "Create ZIP", shipZip);
    registryRegister("list", "List Directory", shipList);
    registryRegister("echo", "Echo", shipEcho);
    CharSeq script_path = "Shipfile";
    Bool dry_run = false;
    if(argc > 1)
    {
        script_path = argv[1];
    }
    FILE* f = fopen(script_path, "rb");
    if(!f)
    {
        Int8 buf[256];
        sprintf(buf, "%s.ship", script_path);
        f = fopen(buf, "rb");
        if(!f)
        {
            printf(FAIL "Error: Script not found: %s\n" ENDC, script_path);
            return 1;
        }
    }
    fseek(f, 0, SEEK_END);
    Int64 fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    Int8* content = (Int8*)malloc(fsize + 1);
    fread(content, 1, fsize, f);
    fclose(f);
    content[fsize] = 0;
    ShipVector tokens = tokenize(content);
    ShipParser parser;
    parser_init(&parser, tokens);
    parserParse(&parser);
    runBuild(parser.title, parser.tasks, dry_run);
    return 0;
}
