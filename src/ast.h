#ifndef __AST_H__
#define __AST_H__

#include <stdint.h>
#include "arraylist.h"
#include "prog_ir.h"

const char* AST_TYPE_NAMES[];
const char* UNARY_OP_NAMES[];
const char* BINARY_OP_NAMES[];

enum ast_type {
    AST_NODE_BODY,
    AST_NODE_FILE,
    AST_NODE_MODULE,
    AST_NODE_CLASS,
    AST_NODE_FUNC,
    AST_NODE_UNARY_POSTFIX,
    AST_NODE_UNARY,
    AST_NODE_CALL,
    AST_NODE_CALC_MEMBER,
    AST_NODE_CAST,
    AST_NODE_BINARY,
    AST_NODE_VAR_DECL,
    AST_NODE_TYPE,
    AST_NODE_INTEGER_LIT,
    AST_NODE_DECIMAL_LIT,
    AST_NODE_STRING_LIT,
    AST_NODE_CHAR_LIT,
    AST_NODE_IDENTIFIER,
    AST_NODE_TERNARY,
    AST_NODE_IF,
    AST_NODE_FOR,
    AST_NODE_WHILE,
    AST_NODE_FOR_EACH,
    AST_NODE_SWITCH,
    AST_NODE_CASE,
    AST_NODE_DEFAULT_CASE,
    AST_NODE_GOTO,
    AST_NODE_RET,
    AST_NODE_CONTINUE,
    AST_NODE_BREAK,
    AST_NODE_TRY,
    AST_NODE_THROW,
    AST_NODE_NEW,
    AST_NODE_LABEL,
    AST_NODE_EMPTY,
    AST_NODE_IMPORT,
    AST_NODE_IMP_NEW,
    AST_NODE_NULL
};

enum unary_ops {
    UNARY_OP_INC,
    UNARY_OP_DEC,
    UNARY_OP_PLUS,
    UNARY_OP_MINUS,
    UNARY_OP_LNOT,
    UNARY_OP_NOT,
    UNARY_OP_DEREF,
    UNARY_OP_REF
};

enum binary_ops {
    BINARY_OP_MEMBER,
    BINARY_OP_MUL,
    BINARY_OP_DIV,
    BINARY_OP_MOD,
    BINARY_OP_PLUS,
    BINARY_OP_MINUS,
    BINARY_OP_LSH,
    BINARY_OP_RSH,
    BINARY_OP_LT,
    BINARY_OP_LTE,
    BINARY_OP_GT,
    BINARY_OP_GTE,
    BINARY_OP_INST,
    BINARY_OP_EQ,
    BINARY_OP_NEQ,
    BINARY_OP_AND,
    BINARY_OP_XOR,
    BINARY_OP_OR,
    BINARY_OP_LAND,
    BINARY_OP_LOR,
    BINARY_OP_ASSN,
    BINARY_OP_MUL_ASSN,
    BINARY_OP_DIV_ASSN,
    BINARY_OP_MOD_ASSN,
    BINARY_OP_PLUS_ASSN,
    BINARY_OP_MINUS_ASSN,
    BINARY_OP_LSH_ASSN,
    BINARY_OP_RSH_ASSN,
    BINARY_OP_EQ_VAL,
    BINARY_OP_NEQ_VAL,
    BINARY_OP_AND_ASSN,
    BINARY_OP_XOR_ASSN,
    BINARY_OP_OR_ASSN,
    BINARY_OP_LAND_ASSN,
    BINARY_OP_LOR_ASSN,
    BINARY_OP_MUL_ASSN_PRE,
    BINARY_OP_DIV_ASSN_PRE,
    BINARY_OP_MOD_ASSN_PRE,
    BINARY_OP_PLUS_ASSN_PRE,
    BINARY_OP_MINUS_ASSN_PRE,
    BINARY_OP_LSH_ASSN_PRE,
    BINARY_OP_RSH_ASSN_PRE,
    BINARY_OP_AND_ASSN_PRE,
    BINARY_OP_XOR_ASSN_PRE,
    BINARY_OP_OR_ASSN_PRE,
    BINARY_OP_LAND_ASSN_PRE,
    BINARY_OP_LOR_ASSN_PRE,
    BINARY_OP_SEQUENCE
};

#define PROTECTION_NONE 0
#define PROTECTION_PRIV 1
#define PROTECTION_PROT 2
#define PROTECTION_PUB 3

const char* PROT_STRING[];

enum parse_error_type {
    PARSE_ERROR_TYPE_UNEXPECTED_TOKEN
};

struct ast_node;

struct ast_node_body {
    struct arraylist* children;
};

struct ast_node_file {
    char* filename;
    char* rel_path;
    struct arraylist* lines;
    struct ast_node* body;
};

struct ast_node_module {
    uint8_t prot;
    char* name;
    struct ast_node* body;
};

struct ast_node_class {
    uint8_t prot;
    uint8_t typed;
    uint8_t synch;
    uint8_t virt;
    uint8_t iface;
    struct ast_node* name;
    struct arraylist* parents;
    struct ast_node* body;
};

struct ast_node_func {
    uint8_t prot;
    uint8_t synch;
    uint8_t virt;
    uint8_t async;
    uint8_t csig;
    struct ast_node* return_type;
    char* name;
    struct arraylist* arguments;
    struct ast_node* body;
};

struct ast_node_unary_postfix {
    uint8_t unary_op;
    struct ast_node* child;
};

struct ast_node_unary {
    uint8_t unary_op;
    struct ast_node* child;
};

struct ast_node_call {
    struct ast_node* func;
    struct arraylist* parameters;
};

struct ast_node_calc_member {
    struct ast_node* parent;
    struct ast_node* calc;
};

struct ast_node_cast {
    struct ast_node* type;
    struct ast_node* expr;
};

struct ast_node_binary {
    struct ast_node* left;
    uint16_t op;
    struct ast_node* right;
};

struct ast_node_vardecl {
    uint8_t prot;
    uint8_t synch;
    uint8_t csig;
    struct ast_node* type;
    char* name;
    struct ast_node* init;
    struct arraylist* cons_init;
};

struct ast_node_type {
    char* name;
    uint8_t variadic;
    uint64_t array_pointer_bitlist;
    uint8_t array_pointer_count;
    struct arraylist* generics;
};

struct ast_node_integer_lit {
    uint64_t lit;
};

struct ast_node_decimal_lit {
    double lit;
};

struct ast_node_string_lit {
    char* lit;
};

struct ast_node_char_lit {
    char* lit;
};

struct ast_node_identifer {
    char* identifier;
};

struct ast_node_ternary {
    struct ast_node* condition;
    struct ast_node* if_true;
    struct ast_node* if_false;
};

struct ast_node_if {
    struct ast_node* condition;
    struct ast_node* expr;
    struct ast_node* elseExpr;
};

struct ast_node_for {
    struct ast_node* init;
    struct ast_node* loop;
    struct ast_node* final;
    struct ast_node* expr;
};

struct ast_node_for_each {
    struct ast_node* init;
    struct ast_node* loop;
    struct ast_node* expr;
};

struct ast_node_while {
    struct ast_node* loop;
    struct ast_node* expr;
};

struct ast_node_switch {
    struct ast_node* switch_on;
    struct arraylist* cases;
};

struct ast_node_label {
    char* name;
};

struct ast_node_case {
    struct ast_node* value;
    struct ast_node* expr;
};

struct ast_node_default_case {
    struct ast_node* expr;
};

struct ast_node_goto {
    struct ast_node* expr;
};

struct ast_node_ret {
    struct ast_node* expr;
};

struct ast_node_continue {

};

struct ast_node_break {
    
};

struct ast_node_try {
    struct ast_node* expr;
    struct ast_node* catch_var_decl;
    struct ast_node* catch_expr;
    struct ast_node* finally_expr;
};

struct ast_node_throw {
    struct ast_node* what;
};

struct ast_node_new {
    struct ast_node* type;
};

struct ast_node_import {
    struct ast_node* what;
};

struct ast_node_imp_new {
    struct arraylist* parameters;
};

struct ast_node {
    uint8_t type;
    uint64_t start_line;
    uint64_t end_line;
    uint64_t start_col;
    uint64_t end_col;
    struct prog_node* prog;
    union {
        struct ast_node_body body;
        struct ast_node_file file;
        struct ast_node_module module;
        struct ast_node_class class;
        struct ast_node_func func;
        struct ast_node_unary_postfix unary_postfix;
        struct ast_node_unary unary;
        struct ast_node_call call;
        struct ast_node_calc_member calc_member;
        struct ast_node_cast cast;
        struct ast_node_binary binary;
        struct ast_node_vardecl vardecl;
        struct ast_node_type type;
        struct ast_node_integer_lit integer_lit;
        struct ast_node_decimal_lit decimal_lit;
        struct ast_node_string_lit string_lit;
        struct ast_node_char_lit char_lit;
        struct ast_node_identifer identifier;
        struct ast_node_ternary ternary;
        struct ast_node_if _if;
        struct ast_node_for _for;
        struct ast_node_while _while;
        struct ast_node_for_each for_each;
        struct ast_node_switch _switch;
        struct ast_node_label label;
        struct ast_node_case _case;
        struct ast_node_default_case default_case;
        struct ast_node_goto _goto;
        struct ast_node_ret ret;
        struct ast_node_continue _continue;
        struct ast_node_break _break;
        struct ast_node_try try;
        struct ast_node_throw throw;
        struct ast_node_new new;
        struct ast_node_import import;
        struct ast_node_imp_new imp_new;
    } data;
};

struct parse_error {
    char* message;
    uint8_t type;
    uint64_t line;
    uint64_t col;
};

struct parse_ctx {
    struct arraylist* parse_errors;
    uint8_t sequence_disabled;
};

struct parse_intermediates {
    struct parse_ctx* ctx;
    struct ast_node* root;
};

struct parse_intermediates parse(struct arraylist* tokens_list, struct arraylist* lines);

void free_ast_node(struct ast_node* node);

struct ast_node* traverse_node(struct ast_node* root, struct ast_node* (traverser)(struct ast_node*, void*), void* arg, int pre);

#endif