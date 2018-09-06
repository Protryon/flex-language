#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdint.h>
#include <unistd.h>
#include "arraylist.h"

typedef enum {
    TOKEN_UNKNOWN,
    TOKEN_IDENTIFIER,
    TOKEN_MODULE,
    TOKEN_TYPED,
    TOKEN_CLASS,
    TOKEN_FUNC,
    TOKEN_PUB,
    TOKEN_PRIV,
    TOKEN_PROT,
    TOKEN_SYNCH,
    TOKEN_VIRT,
    TOKEN_IFACE,
    TOKEN_CSIG,
    TOKEN_ASYNC,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_SWITCH,
    TOKEN_GOTO,
    TOKEN_COLON,
    TOKEN_QMARK,
    TOKEN_SEMICOLON,
    TOKEN_RCURLY,
    TOKEN_LCURLY,
    TOKEN_RPAREN,
    TOKEN_LPAREN,
    TOKEN_RBRACK,
    TOKEN_LBRACK,
    TOKEN_COMMA,
    TOKEN_PERIOD,
    TOKEN_ARROW,
    TOKEN_NEW,
    TOKEN_NULL,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIVIDE,
    TOKEN_MODULUS,
    TOKEN_EXP,
    TOKEN_OR,
    TOKEN_AND, // AMPERSAND
    TOKEN_XOR,
    TOKEN_NOT,
    TOKEN_LNOT,
    TOKEN_LAND,
    TOKEN_LOR,
    TOKEN_EQUALS,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_MUL_EQUALS,
    TOKEN_DIVIDE_EQUALS,
    TOKEN_MODULUS_EQUALS,
    TOKEN_EXP_EQUALS,
    TOKEN_OR_EQUALS,
    TOKEN_AND_EQUALS,
    TOKEN_XOR_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_LAND_EQUALS,
    TOKEN_LOR_EQUALS,
    TOKEN_PLUS_EQUALS_PRE,
    TOKEN_MINUS_EQUALS_PRE,
    TOKEN_MUL_EQUALS_PRE,
    TOKEN_DIVIDE_EQUALS_PRE,
    TOKEN_MODULUS_EQUALS_PRE,
    TOKEN_EXP_EQUALS_PRE,
    TOKEN_OR_EQUALS_PRE,
    TOKEN_AND_EQUALS_PRE,
    TOKEN_XOR_EQUALS_PRE,
    TOKEN_NOT_EQUALS_PRE,
    TOKEN_LAND_EQUALS_PRE,
    TOKEN_LOR_EQUALS_PRE,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_VAL_EQUAL,
    TOKEN_VAL_NOT_EQUAL,
    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_GT,
    TOKEN_GTE,
    TOKEN_LSH,
    TOKEN_RSH,
    TOKEN_LSH_EQUALS,
    TOKEN_RSH_EQUALS,
    TOKEN_LSH_EQUALS_PRE,
    TOKEN_RSH_EQUALS_PRE,
    TOKEN_STRING_LIT,
    TOKEN_CHAR_LIT,
    TOKEN_NUMERIC_LIT,
    TOKEN_NUMERIC_DECIMAL_LIT,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_RET,
    TOKEN_THROW,
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_FINALLY,
    TOKEN_INST,
    TOKEN_IMPORT,
    TOKEN_ELLIPSIS,
    TOKEN_PROTOFUNC
} token_type;

struct token {
    uint8_t type;
    char* value;
    uint64_t line;
    uint64_t start_col;
    uint64_t end_col;
};

void tokenize(char* source, size_t src_len, struct arraylist* tokens);

#endif