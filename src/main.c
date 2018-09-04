
#include "lexer.h"
#include "ast.h"
#include "xstring.h"
#include "streams.h"
#include "info.h"
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define FLEX_HELP "See flexc -h for more information.\n"

#define CLI_ERROR(msg) {fprintf(stderr, msg "\n" FLEX_HELP); return 1;}
#define INVALID_ARG(arg) {fprintf(stderr, "Invalid Argument: '%s'\n" FLEX_HELP, arg); return 1;}
#define MISSING_ARG(arg) {fprintf(stderr, "Missing Argument After: '%s'\n" FLEX_HELP, arg); return 1;}
#define IO_ERROR(filename) {fprintf(stderr, "IO Error: '%s' '%s'\n", strerror(errno), filename); return 1;}
#define COMMA ,
#define CORRUPT_FILE_ERROR(msg, args) {fprintf(stderr, "File Corruption Error: " msg "\n", args); return 1;}
#define LEX_ERROR(msg, args) {fprintf(stderr, "Lex Error: " msg "\n", args);}

struct input_file {
    char* filename;
    char* rel_path;
    struct arraylist* lines;
    struct arraylist* tokens;
    struct ast_node* root;
    struct parse_ctx* parse_ctx;
};

struct ast_node* print_ast(struct ast_node* node, int fd) {
    dprintf(fd, "%lu<%lu>-%lu<%lu> %s:\n", node->start_line, node->start_col, node->end_line, node->end_col, AST_TYPE_NAMES[node->type]);
    switch (node->type) {
        case AST_NODE_BINARY:
        dprintf(fd, "OP = %s\n", BINARY_OP_NAMES[node->data.binary.op]);
        break;
        case AST_NODE_BODY:
        dprintf(fd, "expr# = %lu\n", node->data.body.children == NULL ? 0 : node->data.body.children->entry_count);
        break;
        case AST_NODE_CALC_MEMBER:
        break;
        case AST_NODE_CALL:
        dprintf(fd, "arg# = %lu\n", node->data.call.parameters == NULL ? 0 : node->data.call.parameters->entry_count);
        break;
        case AST_NODE_CASE:
        break;
        case AST_NODE_CAST:
        break;
        case AST_NODE_CLASS:
        dprintf(fd, "prot = %s\n", PROT_STRING[node->data.class.prot]);
        dprintf(fd, "synch = %u\n", node->data.class.synch);
        dprintf(fd, "iface = %u\n", node->data.class.iface);
        dprintf(fd, "typed = %u\n", node->data.class.typed);
        dprintf(fd, "virt = %u\n", node->data.class.virt);
        dprintf(fd, "name = %s\n", node->data.class.name);
        dprintf(fd, "extends# = %lu\n", node->data.class.parents == NULL ? 0 : node->data.class.parents->entry_count);
        break;
        case AST_NODE_DEFAULT_CASE:
        break;
        case AST_NODE_FILE:
        break;
        case AST_NODE_FOR:
        break;
        case AST_NODE_FOR_EACH:
        break;
        case AST_NODE_FUNC:
        dprintf(fd, "prot = %s\n", PROT_STRING[node->data.func.prot]);
        dprintf(fd, "synch = %u\n", node->data.func.synch);
        dprintf(fd, "virt = %u\n", node->data.func.virt);
        dprintf(fd, "name = %s\n", node->data.func.name);
        dprintf(fd, "arg# = %lu\n", node->data.func.arguments == NULL ? 0 : node->data.func.arguments->entry_count);
        break;
        case AST_NODE_GOTO:
        break;
        case AST_NODE_IF:
        break;
        case AST_NODE_MODULE:
        dprintf(fd, "prot = %s\n", PROT_STRING[node->data.module.prot]);
        dprintf(fd, "name = %s\n", node->data.module.name);
        break;
        case AST_NODE_NEW:
        break;
        case AST_NODE_RET:
        dprintf(fd, "has ret = %s\n", node->data.ret.expr == NULL ? "false" : "true");
        break;
        case AST_NODE_SWITCH:
        dprintf(fd, "case# = %lu\n", node->data._switch.cases == NULL ? 0 : node->data._switch.cases->entry_count);
        break;
        case AST_NODE_TERNARY:
        break;
        case AST_NODE_THROW:
        break;
        case AST_NODE_TRY:
        break;
        case AST_NODE_TYPE:
        dprintf(fd, "ptr# = %lu\n", node->data.type.array_pointer_count);
        dprintf(fd, "ptr = %o\n", node->data.type.array_pointer_bitlist);
        dprintf(fd, "generic# = %lu\n", node->data.type.generics == NULL ? 0 : node->data.type.generics->entry_count);
        dprintf(fd, "name = %s\n", node->data.type.name);
        break;
        case AST_NODE_INTEGER_LIT:
        dprintf(fd, "int = %lu\n", node->data.integer_lit.lit);
        break;
        case AST_NODE_DECIMAL_LIT:
        dprintf(fd, "double = %f\n", node->data.decimal_lit.lit);
        break;
        case AST_NODE_STRING_LIT:
        dprintf(fd, "string = \"%s\"\n", node->data.string_lit.lit);
        break;
        case AST_NODE_CHAR_LIT:
        dprintf(fd, "char = '%s'\n", node->data.char_lit.lit);
        break;
        case AST_NODE_IDENTIFIER:
        dprintf(fd, "ident = %s\n", node->data.identifier.identifier);
        break;
        case AST_NODE_UNARY:
        dprintf(fd, "OP = %s\n", UNARY_OP_NAMES[node->data.unary.unary_op]);
        break;
        case AST_NODE_UNARY_POSTFIX:
        dprintf(fd, "OP = %s\n", UNARY_OP_NAMES[node->data.unary_postfix.unary_op]);
        break;
        case AST_NODE_VAR_DECL:
        dprintf(fd, "prot = %s\n", PROT_STRING[node->data.vardecl.prot]);
        dprintf(fd, "synch = %u\n", node->data.vardecl.synch);
        dprintf(fd, "csig = %u\n", node->data.vardecl.csig);
        dprintf(fd, "name = %s\n", node->data.vardecl.name);
        break;
        case AST_NODE_WHILE:
        break;
        case AST_NODE_LABEL:
        dprintf(fd, "label = %s\n", node->data.identifier.identifier);
        break;
        case AST_NODE_CONTINUE:
        break;
        case AST_NODE_BREAK:;
        break;
        case AST_NODE_IMP_NEW:;
        dprintf(fd, "arg# = %lu\n", node->data.imp_new.parameters == NULL ? 0 : node->data.imp_new.parameters->entry_count);
        break;
        case AST_NODE_IMPORT:;
    }
    return node;
}

int main(int argc, char* argv[]) {
    char* outputPE = NULL;
    char* outputLex = NULL;
    char* outputAST = NULL;
    char* outputIR = NULL;
    char* input_files[argc];
    int input_file_count = 0;
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if (arg[0] == '-') {
            arg++;
            if (arg[0] == 0) INVALID_ARG(arg - 1);
            if (str_eq(arg, "h") || str_eq(arg, "-help")) {
                printf("flexc v%s\nCommands:\n", FLEXC_VERSION);
                //TODO:
            } else if (str_eq(arg, "o") || str_eq(arg, "-out")) {
                if (i >= argc - 1) {
                    MISSING_ARG(arg - 1);
                }
                char* arg2 = argv[++i];
                outputPE = arg2;
            } else if (str_eq(arg, "olex") || str_eq(arg, "-out-lex")) {
                if (i >= argc - 1) {
                    MISSING_ARG(arg - 1);
                }
                char* arg2 = argv[++i];
                outputLex = arg2;
            } else if (str_eq(arg, "oast") || str_eq(arg, "-out-ast")) {
                if (i >= argc - 1) {
                    MISSING_ARG(arg - 1);
                }
                char* arg2 = argv[++i];
                outputAST = arg2;
            } else if (str_eq(arg, "oir") || str_eq(arg, "-out-ir")) {
                if (i >= argc - 1) {
                    MISSING_ARG(arg - 1);
                }
                char* arg2 = argv[++i];
                outputIR = arg2;
            } else {
                INVALID_ARG(arg - 1);
            }
        } else {
            input_files[input_file_count++] = arg;
        }
    }
    if (input_file_count == 0) {
        CLI_ERROR("No input specified.");
    }
    if (outputPE == NULL && outputLex == NULL && outputAST == NULL && outputIR == NULL) {
        CLI_ERROR("No output specified.");
    }
    struct {
        char* filename;
        void* content;
        size_t length;
    } input_contents[input_file_count];

    for (int i = 0; i < input_file_count; i++) {
        char* file = input_files[i];
        int fd = open(file, O_RDONLY);
        if (fd < 0) {
            IO_ERROR(file);
        }
        void* content = NULL;
        ssize_t content_len = readUntilEnd(fd, &content);
        close(fd);
        if (content_len < 0) {
            IO_ERROR(file);
        }
        input_contents[i].filename = file;
        input_contents[i].content = content;
        input_contents[i].length = content_len;
    }

    struct input_file input_data[input_file_count];
    int lex_error_count = 0;

    for (int i = 0; i < input_file_count; i++) {
        struct input_file* input = &input_data[i];
        input->filename = strrchr(input_contents[i].filename, '/');
        if (input->filename == NULL) {
            input->filename = input_contents[i].filename;
        } else {
            input->filename++;
        }
        input->rel_path = input_contents[i].filename;
        char* data = input_contents[i].content;
        size_t data_len = input_contents[i].length;
        uint32_t line = 1;
        uint32_t column = 0;
        input->lines = arraylist_new(128, sizeof(char*));
        input->tokens = arraylist_new(128, sizeof(struct token*));
        arraylist_addptr(input->lines, data);
        for (size_t i = 0; i < data_len; i++) {
            column++;
            if (data[i] == 0 || data[i] > 0x7F) {
                CORRUPT_FILE_ERROR("Invalid character at %s:%u:%u: 0x%02X", input->rel_path COMMA line COMMA column COMMA data[i]);
            } else if (data[i] == '\n') {
                data[i] = 0;
                line++;
                column = 0;
                arraylist_addptr(input->lines, data + i + 1);
            }
        }
        tokenize(data, data_len, input->tokens);
        for (size_t j = 0; j < input->tokens->entry_count; j++) {
            struct token* token = arraylist_getptr(input->tokens, j);
            if (token->type == TOKEN_UNKNOWN) {
                LEX_ERROR("Invalid symbol @ %s:%u<%u-%u>: %s", input->rel_path COMMA token->line COMMA token->start_col COMMA token->end_col COMMA token->value);
                lex_error_count++;
            }
        }
        if (lex_error_count > 0) continue;
        struct parse_intermediates immed = parse(input->tokens, input->lines);
        input->parse_ctx = immed.ctx;
        input->root = immed.root;
        if (input->parse_ctx->parse_errors->entry_count > 0) {
            fprintf(stderr, "%lu errors found in file %s.\n", input->parse_ctx->parse_errors->entry_count, input->rel_path);
            for (size_t i = 0; i < input->parse_ctx->parse_errors->entry_count; i++) {
                struct parse_error* error = arraylist_getptr(input->parse_ctx->parse_errors, i);
                fprintf(stderr, "%s\n", error->message);
            }
        }
    }
    if (lex_error_count > 0) {
        LEX_ERROR("You have %u invalid symbol(s), compilation terminated.", lex_error_count);
        return 1;
    }

    if (outputLex != NULL) {
        int fd = open(outputLex, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (fd < 0) {
            IO_ERROR(outputLex);
        }
        char linebuf[4096];
        size_t line_ct = 0;
        for (int i = 0; i < input_file_count; i++) {
            struct input_file* input = &input_data[i];
            line_ct = snprintf(linebuf, 4096, "File: %s, Line# %lu", input->filename, input->lines->entry_count);
            if (line_ct < 0) line_ct = 4096;
            writeLine(fd, linebuf, line_ct);
            for (size_t j = 0; j < input->tokens->entry_count; j++) {
                struct token* token = arraylist_getptr(input->tokens, j);
                line_ct = snprintf(linebuf, 4096, "%lu<%lu-%lu>, %u: %s", token->line, token->start_col, token->end_col, token->type, token->value);
                if (line_ct < 0) line_ct = 4096;
                writeLine(fd, linebuf, line_ct);
            }
        }
        close(fd);
    }

    if (outputAST != NULL) {
        int fd = open(outputAST, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (fd < 0) {
            IO_ERROR(outputAST);
        }
        for (int i = 0; i < input_file_count; i++) {
            struct input_file* input = &input_data[i];
            dprintf(fd, "File: %s, Line#: %lu\n", input->filename, input->lines->entry_count);
            traverse_node(input->root, print_ast, fd, 1);
        }
        close(fd);
    }


}