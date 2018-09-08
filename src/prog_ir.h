#ifndef __PROG_IR_H__
#define __PROG_IR_H__

#include "ast.h"
#include "hash.h"

struct prog_file {
    char* filename;
    char* rel_path;
    struct arraylist* lines;
};

struct prog_module {
    char* name;
    struct prog_file* file;
    uint8_t prot;
    struct prog_module* parent;
    struct hashmap* submodules;
    struct hashmap* classes;
    struct hashmap* funcs;
    struct hashmap* vars;
    struct hashmap* types;
    struct hashmap* node_map;
    struct arraylist* imported_modules;
};

#define PROG_NODE_AST_NODE 0
#define PROG_NODE_GLOBAL_REF 1
#define PROG_NODE_TYPE 2
#define PROG_NODE_PARAM_REF 3
#define PROG_NODE_LOCAL_REF 4
#define PROG_NODE_CLASS_REF 5
#define PROG_NODE_CAPTURED_REF 6
#define PROG_NODE_THIS_REF 7
#define PROG_NODE_LOCAL_DECL 8
#define PROG_NODE_EXTRACTED_FUNC_REF 9

struct prog_node {
    uint8_t prog_type;
    struct ast_node* ast_node;
    union {
        struct prog_var* global;
        struct prog_var* param;
        struct prog_var* local;
        struct prog_var* clas;
        struct prog_var* captured;
        struct prog_var* _this;
        struct prog_type* type;
        struct prog_func* func;
    } data;
};

struct prog_class {
    struct prog_module* module;
    struct prog_type* type;
    struct prog_file* file;
    uint8_t prot;
    uint8_t typed;
    uint8_t synch;
    uint8_t virt;
    uint8_t iface;
    uint8_t pure;
    char* name;
    struct arraylist* parents;
    struct hashmap* funcs;
    struct hashmap* vars;
    struct hashmap* node_map;
};

struct prog_func {
    struct prog_module* module;
    struct prog_class* clas; // VScode thinks class is a keyword in C...
    struct prog_func* closing;
    struct prog_file* file;
    char* name;
    uint64_t uid;
    uint8_t anonymous;
    uint8_t prot;
    uint8_t synch;
    uint8_t virt;
    uint8_t async;
    uint8_t csig;
    uint8_t stat;
    uint8_t pure;
    struct prog_type* return_type;
    struct hashmap* arguments;
    struct hashmap* node_map;
    struct arraylist* closures;
    struct {
        struct ast_node* body;
        struct ast_node* root;
    } proc;

};

struct prog_var { // only interfunc vars
    struct prog_module* module;
    struct prog_class* clas; // VScode thinks class is a keyword in C...
    struct prog_func* func;
    struct prog_file* file;
    char* name;
    uint64_t uid;
    uint8_t prot;
    uint8_t synch;
    uint8_t csig;
    uint8_t stat;
    uint8_t cons;
    struct prog_type* type;
    struct {
        struct ast_node* init;
        struct arraylist* cons_init;
    } proc;
    struct prog_func* pre_alloc_func;
};

#define PROG_TYPE_UNKNOWN 0
#define PROG_TYPE_PRIMITIVE 1
#define PROG_TYPE_CLASS 2
#define PROG_TYPE_FUNC 3

enum prim_type {
    PRIM_V,
    PRIM_VOID = PRIM_V,
    PRIM_U8,
    PRIM_B = PRIM_U8,
    PRIM_BYTE = PRIM_U8,
    PRIM_C = PRIM_U8,
    PRIM_CHAR = PRIM_U8,
    PRIM_UINT8 = PRIM_U8,
    PRIM_I8,
    PRIM_INT8 = PRIM_I8,
    PRIM_U16,
    PRIM_USH = PRIM_U16,
    PRIM_USHORT = PRIM_U16,
    PRIM_UINT16 = PRIM_U16,
    PRIM_I16,
    PRIM_SH = PRIM_I16,
    PRIM_SHORT = PRIM_I16,
    PRIM_INT16 = PRIM_I16,
    PRIM_U32,
    PRIM_U = PRIM_U32,
    PRIM_UINT = PRIM_U32,
    PRIM_UINT32 = PRIM_U32,
    PRIM_I32,
    PRIM_I = PRIM_I32,
    PRIM_INT = PRIM_I32,
    PRIM_INT32 = PRIM_I32,
    PRIM_U64,
    PRIM_UL = PRIM_U64,
    PRIM_ULONG = PRIM_U64,
    PRIM_UINT64 = PRIM_U64,
    PRIM_I64,
    PRIM_L = PRIM_I64,
    PRIM_LONG = PRIM_I64,
    PRIM_INT64 = PRIM_I64,
    PRIM_F,
    PRIM_FLOAT = PRIM_F,
    PRIM_D,
    PRIM_DOUBLE = PRIM_D
};

struct prog_type {
    uint8_t type;
    char* name;
    uint64_t ptr_array_count;
    uint64_t ptr_array;
    uint8_t variadic;
    uint8_t is_master;
    struct prog_type* master_type;
    struct ast_node* ast;
    struct prog_file* file;
    struct hashmap* generics;
    union {
        struct {
            struct prog_class* clas;
        } clas;
        struct {
            uint8_t prim_type;
        } prim;
        struct {
            struct prog_type* return_type;
            struct arraylist* arg_types;
        } func;
    } data;
};

struct prog_state {
    struct hashmap* imports; // external modules of prog_modules
    struct hashmap* modules; // does not include submodules
    struct hashmap* extracted_funcs; // all program funcs
    struct arraylist* errors;
    struct hashmap* node_map;
    uint64_t next_var_id;
};

struct prog_state* gen_prog(struct arraylist* files);


#endif