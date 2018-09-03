
#include "lexer.h"
#include "ast.h"
#include "smem.h"
#include "xstring.h"
#include "arraylist.h"
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define PARSE_ERROR_UNEXPECTED_TOKEN(token, expecting) {struct parse_error* perr = smalloc(sizeof(struct parse_error)); size_t bs = strlen((token)->value) + strlen(expecting) + 128; perr->message = smalloc(bs); snprintf(perr->message, bs, "Unexpected token: '%s' @ %lu:%lu. Expecting " expecting ".\n", (token)->value, (token)->line, (token)->start_col); perr->line = (token)->line; perr->col = (token)->start_col; perr->type = PARSE_ERROR_TYPE_UNEXPECTED_TOKEN; arraylist_addptr(ctx->parse_errors, perr);free_ast_node(node);return NULL;}
#define INIT_PARSE_FUNC() struct token* ttok = NULL;
#define ALLOC_NODE(typex) struct ast_node* node = scalloc(sizeof(struct ast_node)); node->type = typex;
#define DUMMY_NODE() struct ast_node* node = NULL; struct ast_node dummy_node; struct ast_node* dummy_node_ptr = &dummy_node;
#define ALLOC_NODE_DUMMY(typex) node = scalloc(sizeof(struct ast_node)); node->type = typex;
#define START_NODE(nodex) if( nodex != NULL && *token_count > 0) { nodex->start_line = nodex->end_line = (*tokens)[0]->line; nodex->start_col = (*tokens)[0]->start_col; nodex->end_col = (*tokens)[0]->start_col; }
#define END_NODE(nodex) if ( nodex != NULL){ nodex->end_line = (*tokens)[-1]->line; nodex->end_col = (*tokens)[-1]->end_col; }
#define START_DUMMY_NODE() START_NODE(dummy_node_ptr)
#define COPY_DUMMY_TO_REAL(nodex) {nodex->start_line = nodex->end_line = dummy_node_ptr->end_line; nodex->start_col = dummy_node_ptr->start_col; nodex->end_col = dummy_node_ptr->end_col;}
#define EOF_ERROR_TOKEN (*token_count == 0 ? &((struct token) {TOKEN_UNKNOWN, "EOF", 0, 0, 0}) : **tokens)
#define EXPECT_TOKEN(tokenx, expecting) if (!(ttok = eat_token(tokenx, tokens, token_count))) {PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "'" expecting "'");}
#define EXPECT_SOME_TOKEN() if (*token_count == 0) {PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, " not EOF");}
#define BACKTRACK() (*tokens)--; (*token_count)++;
// obviously, backtracking should only be done when it is known to not underflow.
#define SKIP_TOKEN() if (*token_count > 0) { (*tokens)++; (*token_count)--; }
#define CHECK_EXPR(node) if (node == NULL) { return NULL; };
#define CHECK_EXPR_AND(node, and) if (node == NULL) { and; return NULL;};
#define STORE_TOKEN_STATE(state) struct token** tokensState_##state = *tokens; size_t token_count_##state = *token_count; size_t error_entry_count_##state = ctx->parse_errors->entry_count;
#define STORE2_TOKEN_STATE(state) tokensState_##state = *tokens; token_count_##state = *token_count; error_entry_count_##state = ctx->parse_errors->entry_count;
#define RESTORE_TOKEN_STATE(state) *tokens = tokensState_##state; *token_count = token_count_##state; ctx->parse_errors->entry_count = error_entry_count_##state;

const char* AST_TYPE_NAMES[] = {"BODY", "FILE", "MODULE", "CLASS", "FUNC", "UNARY_POSTFIX", "UNARY", "CALL", "CALC_MEMBER", "CAST", "BINARY", "VAR_DECL", "TYPE", "INTEGER_LIT", "DECIMAL_LIT", "STRING_LIT", "CHAR_LIT", "IDENTIFIER", "TERNARY", "IF", "FOR", "WHILE", "FOR_EACH", "SWITCH", "CASE", "DEFAULT_CASE", "GOTO", "RET", "CONTINUE", "BREAK", "TRY", "THROW", "NEW", "LABEL", "EMPTY", "IMPORT", "IMP_NEW", "NULL"};
const char* UNARY_OP_NAMES[] = {"++", "--", "+", "-", "!", "~", "*", "&"};
const char* BINARY_OP_NAMES[] = {".", "*", "/", "%", "+", "-", "<<", ">>", "<", "<=", ">", ">=", "inst", "==", "!=", "&", "^", "|", "&&", "||", "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "===", "!==", "&=", "^=", "|=", "&&=", "||=", "*== ", "/==", "%==", "+==", "-==", "<<==", ">>==", "&==", "^==", "|==", "&&==", "||==", ","};
const char* PROT_STRING[] = {"NONE", "PRIV", "PROT", "PUB"};

struct token* eat_token(uint16_t token_type, struct token*** tokens, size_t* token_count) {
    if (*token_count == 0) return NULL;
    if ((*tokens)[0]->type == token_type) {
        struct token* tok = **tokens;
        (*tokens)++;
        (*token_count)--;
        return tok;
    }
    return NULL;
}

#define EAT(x) eat_token(x, tokens, token_count)

int match_token(uint16_t token_type, struct token*** tokens, size_t* token_count) {
    if (*token_count == 0) return 0;
    if ((*tokens)[0]->type == token_type) {
        return 1;
    }
    return 0;
}

#define MATCH(x) match_token(x, tokens, token_count)

int match_type(struct token*** tokens, size_t* token_count) {
    return MATCH(TOKEN_IDENTIFIER);
}

#define MATCH_TYPE() match_type(tokens, token_count)

uint8_t maybe_protection(struct token*** tokens, size_t* token_count) {
    if (EAT(TOKEN_PUB)) {
        return PROTECTION_PUB;
    } else if (EAT(TOKEN_PROT)) {
        return PROTECTION_PROT;
    } else if (EAT(TOKEN_PRIV)) {
        return PROTECTION_PROT;
    } else {
        return PROTECTION_NONE;
    }
}

#define TRAVERSE(item) item = traverse_node(item, traverser, arg, pre);
#define TRAVERSE_ARRAYLIST(list) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* node = arraylist_getptr(list, i); struct ast_node* new_node = traverse_node(node, traverser, arg, pre); if (new_node != node) { arraylist_setptr(list, i, new_node); } } }

struct ast_node* traverse_node(struct ast_node* root, struct ast_node* (traverser)(struct ast_node*, void*), void* arg, int pre) {
    if (root == NULL) return NULL;
    if (pre) root = traverser(root, arg);
    if (root == NULL) return NULL;
    switch (root->type) {
        case AST_NODE_BINARY:
        TRAVERSE(root->data.binary.left);
        TRAVERSE(root->data.binary.right);
        break;
        case AST_NODE_BODY:
        TRAVERSE_ARRAYLIST(root->data.body.children);
        break;
        case AST_NODE_CALC_MEMBER:
        TRAVERSE(root->data.calc_member.calc);
        TRAVERSE(root->data.calc_member.parent);
        break;
        case AST_NODE_CALL:
        TRAVERSE(root->data.call.func);
        TRAVERSE_ARRAYLIST(root->data.call.parameters);
        break;
        case AST_NODE_CASE:
        TRAVERSE(root->data._case.value);
        TRAVERSE(root->data._case.expr);
        break;
        case AST_NODE_CAST:
        TRAVERSE(root->data.cast.type);
        TRAVERSE(root->data.cast.expr);
        break;
        case AST_NODE_CLASS:
        TRAVERSE_ARRAYLIST(root->data.class.parents);
        TRAVERSE(root->data.class.body);
        break;
        case AST_NODE_DEFAULT_CASE:
        TRAVERSE(root->data.default_case.expr);
        break;
        case AST_NODE_FILE:
        TRAVERSE(root->data.file.body);
        break;
        case AST_NODE_FOR:
        TRAVERSE(root->data._for.init);
        TRAVERSE(root->data._for.loop);
        TRAVERSE(root->data._for.final);
        TRAVERSE(root->data._for.expr);
        break;
        case AST_NODE_FOR_EACH:
        TRAVERSE(root->data.for_each.init);
        TRAVERSE(root->data.for_each.loop);
        TRAVERSE(root->data.for_each.expr);
        break;
        case AST_NODE_FUNC:
        TRAVERSE(root->data.func.return_type);
        TRAVERSE_ARRAYLIST(root->data.func.arguments);
        TRAVERSE(root->data.func.body);
        break;
        case AST_NODE_GOTO:
        TRAVERSE(root->data._goto.expr);
        break;
        case AST_NODE_IF:
        TRAVERSE(root->data._if.condition);
        TRAVERSE(root->data._if.expr);
        TRAVERSE(root->data._if.elseExpr);
        break;
        case AST_NODE_MODULE:
        TRAVERSE(root->data.module.body);
        break;
        case AST_NODE_NEW:
        TRAVERSE(root->data.new.type);
        break;
        case AST_NODE_RET:
        TRAVERSE(root->data.ret.expr);
        break;
        case AST_NODE_SWITCH:
        TRAVERSE(root->data._switch.switch_on);
        TRAVERSE_ARRAYLIST(root->data._switch.cases);
        break;
        case AST_NODE_TERNARY:
        TRAVERSE(root->data.ternary.condition);
        TRAVERSE(root->data.ternary.if_true);
        TRAVERSE(root->data.ternary.if_false);
        break;
        case AST_NODE_THROW:
        TRAVERSE(root->data.throw.what);
        break;
        case AST_NODE_TRY:
        TRAVERSE(root->data.try.expr);
        TRAVERSE(root->data.try.catch_var_decl);
        TRAVERSE(root->data.try.catch_expr);
        TRAVERSE(root->data.try.finally_expr);
        break;
        case AST_NODE_TYPE:
        TRAVERSE_ARRAYLIST(root->data.type.generics);
        break;
        case AST_NODE_UNARY:
        TRAVERSE(root->data.unary.child);
        break;
        case AST_NODE_UNARY_POSTFIX:
        TRAVERSE(root->data.unary_postfix.child);
        break;
        case AST_NODE_VAR_DECL:
        TRAVERSE(root->data.vardecl.type);
        TRAVERSE(root->data.vardecl.init);
        TRAVERSE_ARRAYLIST(root->data.vardecl.cons_init);
        break;
        case AST_NODE_WHILE:
        TRAVERSE(root->data._while.loop);
        TRAVERSE(root->data._while.expr);
        break;
        case AST_NODE_IMPORT:
        TRAVERSE(root->data.import.what);
        break;
        case AST_NODE_IMP_NEW:
        TRAVERSE_ARRAYLIST(root->data.imp_new.parameters);
        break;
    }
    if (!pre) root = traverser(root, arg);
    return root;
}

struct ast_node* _free_ast_node(struct ast_node* node, void* arg) {
    switch (node->type) {
        case AST_NODE_BODY:
        arraylist_free(node->data.body.children);
        break;
        case AST_NODE_CALL:
        arraylist_free(node->data.call.parameters);
        break;
        case AST_NODE_CLASS:
        arraylist_free(node->data.class.parents);
        break;
        case AST_NODE_FUNC:
        arraylist_free(node->data.func.arguments);
        break;
        case AST_NODE_SWITCH:
        arraylist_free(node->data._switch.cases);
        break;
        case AST_NODE_TYPE:
        arraylist_free(node->data.type.generics);
        break;
        case AST_NODE_VAR_DECL:
        arraylist_free(node->data.vardecl.cons_init);
    }
    free(node);
    return NULL;
}

void free_ast_node(struct ast_node* node) {
    if (node == NULL) return;
    traverse_node(node, _free_ast_node, NULL, 0);
}

struct ast_node* parse_lambda_func(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t virt, uint8_t async, uint8_t csig);
struct ast_node* parse_func(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t virt, uint8_t async, uint8_t csig);

struct ast_node* parse_identifier(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_IDENTIFIER);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_IDENTIFIER, "identifier");
    node->data.identifier.identifier = ttok->value;
    END_NODE(node);
    return node;
}

struct ast_node* parse_expression_maybe_semicolon(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count);
struct ast_node* parse_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count);
struct ast_node* parse_vardecl(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t csig, uint8_t can_variadic, uint8_t can_init, uint8_t can_semi);
struct ast_node* parse_assignment_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count);

struct ast_node* parse_type(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t can_variadic, uint8_t can_ptrarr, uint8_t can_generic) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_TYPE);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_IDENTIFIER, "identifier");
    node->data.type.name = ttok->value;
    if (can_generic && EAT(TOKEN_LT)) {
        node->data.type.generics = arraylist_new(1, sizeof(struct ast_node*));
        while (MATCH_TYPE()) {
            struct ast_node* subtype = parse_type(ctx, tokens, token_count, 0, 1, 1);
            CHECK_EXPR_AND(subtype, free_ast_node(node));
            arraylist_addptr(node->data.type.generics, subtype);
            if (!EAT(TOKEN_COMMA)) {
                break;
            }
        }
        EXPECT_TOKEN(TOKEN_GT, "'>'");
    }
    if (can_ptrarr)
        while (MATCH(TOKEN_LBRACK) || MATCH(TOKEN_MUL)) {
            if (node->data.type.array_pointer_count >= 64) {
                break;
            }
            if (EAT(TOKEN_LBRACK)) {
                EXPECT_TOKEN(TOKEN_RBRACK, "]");
                node->data.type.array_pointer_bitlist << 1;
                node->data.type.array_pointer_count++;
            } else if (EAT(TOKEN_MUL)) {
                node->data.type.array_pointer_bitlist << 1 | 1;
                node->data.type.array_pointer_count++;
            }
        }
    if (can_variadic && EAT(TOKEN_ELLIPSIS)) {
        node->data.type.variadic = 1;
    }
    END_NODE(node);
    return node;
}

struct ast_node* parse_body(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_BODY);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_LCURLY, "{");
    if (!MATCH(TOKEN_RCURLY)) {
        node->data.body.children = arraylist_new(4, sizeof(struct ast_node*));
        while (!MATCH(TOKEN_RCURLY) && *token_count > 0) {
            struct ast_node* child = parse_expression_maybe_semicolon(ctx, tokens, token_count);
            CHECK_EXPR_AND(child, free_ast_node(node));
            arraylist_addptr(node->data.body.children, child);
        }
    }
    EXPECT_TOKEN(TOKEN_RCURLY, "}");
    END_NODE(node);
    return node;
}

struct ast_node* parse_if_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_IF);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_IF, "if");
    EXPECT_TOKEN(TOKEN_LPAREN, "(");
    node->data._if.condition = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._if.condition, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_RPAREN, ")");
    node->data._if.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._if.expr, free_ast_node(node));
    if (EAT(TOKEN_ELSE)) {
        node->data._if.elseExpr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data._if.elseExpr, free_ast_node(node));
    }
    END_NODE(node);
    return node;
}

struct ast_node* parse_for_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    START_DUMMY_NODE();
    EXPECT_TOKEN(TOKEN_FOR, "for");
    EXPECT_TOKEN(TOKEN_LPAREN, "(");
    struct ast_node* init = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR(init);
    if (MATCH(TOKEN_COLON)) {
        ALLOC_NODE_DUMMY(AST_NODE_FOR_EACH);
        COPY_DUMMY_TO_REAL(node);
        node->data.for_each.init = init;
        EAT(TOKEN_COLON);
        node->data.for_each.loop = parse_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.for_each.loop, free_ast_node(node));
        EXPECT_TOKEN(TOKEN_RPAREN, ")");
        node->data.for_each.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.for_each.expr, free_ast_node(node));
        END_NODE(node);
    } else {
        ALLOC_NODE_DUMMY(AST_NODE_FOR);
        COPY_DUMMY_TO_REAL(node);
        node->data._for.init = init;
        EXPECT_TOKEN(TOKEN_SEMICOLON, ";");
        node->data._for.loop = parse_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data._for.loop, free_ast_node(node));
        EXPECT_TOKEN(TOKEN_SEMICOLON, ";");
        node->data._for.final = parse_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data._for.final, free_ast_node(node));
        EXPECT_TOKEN(TOKEN_RPAREN, ")");
        node->data._for.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data._for.expr, free_ast_node(node));
        END_NODE(node);
    }
    return node;
}

struct ast_node* parse_while_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_WHILE);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_WHILE, "while");
    EXPECT_TOKEN(TOKEN_LPAREN, "(");
    node->data._while.loop = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._while.loop, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_RPAREN, ")");
    node->data._while.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._while.expr, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_switch_case(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_CASE);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_CASE, "case");
    node->data._case.value = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._case.value, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_COLON, ":");
    node->data._case.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._case.expr, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_switch_default_case(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_DEFAULT_CASE);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_DEFAULT, "default");
    EXPECT_TOKEN(TOKEN_COLON, ":");
    node->data.default_case.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data.default_case.expr, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_switch_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_SWITCH);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_SWITCH, "switch");
    EXPECT_TOKEN(TOKEN_LPAREN, "(");
    node->data._switch.switch_on = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data._switch.switch_on, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_RPAREN, ")");
    EXPECT_TOKEN(TOKEN_LCURLY, "{");
    node->data._switch.cases = arraylist_new(8, sizeof(struct ast_node*));
    uint8_t has_default = 0;
    while (MATCH(TOKEN_CASE) || (!has_default && MATCH(TOKEN_DEFAULT))) {
        if (!has_default && MATCH(TOKEN_DEFAULT)) {
            has_default = 1;
            struct ast_node* def = parse_switch_default_case(ctx, tokens, token_count);
            CHECK_EXPR_AND(def, free_ast_node(node));
            arraylist_addptr(node->data._switch.cases, def);
        } else {
            struct ast_node* cas = parse_switch_case(ctx, tokens, token_count);
            CHECK_EXPR_AND(cas, free_ast_node(node));
            arraylist_addptr(node->data._switch.cases, cas);
        }
    }
    EXPECT_TOKEN(TOKEN_RCURLY, "}");
    END_NODE(node);
    return node;
}

struct ast_node* parse_try_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_TRY);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_TRY, "try");
    node->data.try.expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data.try.expr, free_ast_node(node));
    if (MATCH(TOKEN_CATCH)) {
        EAT(TOKEN_CATCH);
        EXPECT_TOKEN(TOKEN_LPAREN, "(");
        node->data.try.catch_var_decl = parse_vardecl(ctx, tokens, token_count, 0, 0, 0, 0, 0, 0);
        CHECK_EXPR_AND(node->data.try.catch_var_decl, free_ast_node(node));
        EXPECT_TOKEN(TOKEN_RPAREN, ")");
        node->data.try.catch_expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.try.catch_expr, free_ast_node(node));
    } else {
        EXPECT_TOKEN(TOKEN_FINALLY, "finally");
        goto finally;
    }
    if (EAT(TOKEN_FINALLY)) {
        finally:;
        node->data.try.finally_expr = parse_expression_maybe_semicolon(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.try.finally_expr, free_ast_node(node));
    }
    END_NODE(node);
    return node;
}

struct ast_node* parse_new_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_NEW);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_NEW, "new");
    node->data.new.type = parse_type(ctx, tokens, token_count, 0, 1, 1);
    CHECK_EXPR_AND(node->data.new.type, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_primary_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    switch ((*tokens)[0]->type) {
        case TOKEN_IF:
        return parse_if_expression(ctx, tokens, token_count);
        case TOKEN_FOR:
        return parse_for_expression(ctx, tokens, token_count);
        case TOKEN_WHILE:
        return parse_while_expression(ctx, tokens, token_count);
        case TOKEN_SWITCH:
        return parse_switch_expression(ctx, tokens, token_count);
        case TOKEN_TRY:
        return parse_try_expression(ctx, tokens, token_count);
        case TOKEN_NEW:
        return parse_new_expression(ctx, tokens, token_count);
        case TOKEN_NULL:
        ALLOC_NODE_DUMMY(AST_NODE_NULL);
        START_NODE(node);
        EAT(TOKEN_NULL);
        END_NODE(node);
        return node;
        case TOKEN_LBRACK:
        ALLOC_NODE_DUMMY(AST_NODE_IMP_NEW);
        START_NODE(node);
        EXPECT_TOKEN(TOKEN_LBRACK, "[");
        if (!MATCH(TOKEN_RBRACK)) {
            node->data.imp_new.parameters = arraylist_new(4, sizeof(struct ast_node*));
            do {
                struct ast_node* child = parse_assignment_expression(ctx, tokens, token_count);
                CHECK_EXPR_AND(child, free_ast_node(node));
                arraylist_addptr(node->data.imp_new.parameters, child);
            } while (EAT(TOKEN_COMMA));
        }
        EXPECT_TOKEN(TOKEN_RBRACK, "]");
        END_NODE(node);
        return node;
        case TOKEN_THROW:
        ALLOC_NODE_DUMMY(AST_NODE_THROW);
        START_NODE(node);
        EAT(TOKEN_THROW);
        node->data.throw.what = parse_expression(ctx, tokens, token_count);
        END_NODE(node);
        return node;
        case TOKEN_GOTO:
        ALLOC_NODE_DUMMY(AST_NODE_GOTO);
        START_NODE(node);
        EAT(TOKEN_GOTO);
        node->data._goto.expr = parse_expression(ctx, tokens, token_count);
        END_NODE(node);
        return node;
        case TOKEN_RET:
        ALLOC_NODE_DUMMY(AST_NODE_RET);
        START_NODE(node);
        EAT(TOKEN_RET);
        node->data.ret.expr = EAT(TOKEN_SEMICOLON) ? NULL : parse_expression(ctx, tokens, token_count);
        END_NODE(node);
        return node;
        case TOKEN_CONTINUE:
        ALLOC_NODE_DUMMY(AST_NODE_CONTINUE);
        START_NODE(node);
        EAT(TOKEN_CONTINUE);
        END_NODE(node);
        return node;
        case TOKEN_BREAK:
        ALLOC_NODE_DUMMY(AST_NODE_BREAK);
        START_NODE(node);
        EAT(TOKEN_BREAK);
        END_NODE(node);
        return node;
        case TOKEN_LCURLY:
        return parse_body(ctx, tokens, token_count);
        case TOKEN_SYNCH:
        case TOKEN_ASYNC:
        case TOKEN_LT:
        case TOKEN_FUNC:;
        uint8_t async = EAT(TOKEN_ASYNC);
        uint8_t synch = EAT(TOKEN_SYNCH);
        if (MATCH(TOKEN_FUNC)) goto func;
        return parse_lambda_func(ctx, tokens, token_count, PROTECTION_PRIV, synch, 0, async, 0);
        func:;
        return parse_func(ctx, tokens, token_count, PROTECTION_PRIV, synch, 0, async, 0);
        case TOKEN_LPAREN:
        EXPECT_TOKEN(TOKEN_LPAREN, "(");
        struct ast_node* ret = parse_expression(ctx, tokens, token_count);
        EXPECT_TOKEN(TOKEN_RPAREN, ")");
        return ret;
        case TOKEN_NUMERIC_LIT:
        ALLOC_NODE_DUMMY(AST_NODE_INTEGER_LIT);
        START_NODE(node);
        char* int_str = EAT(TOKEN_NUMERIC_LIT)->value;
        if (str_startsWithCase(int_str, "0b")) {
            node->data.integer_lit.lit = strtoull(int_str + 2, NULL, 2);
        } else {
            node->data.integer_lit.lit = strtoull(int_str, NULL, 0);
        }
        if (node->data.integer_lit.lit == 0 && errno != 0) {
            PARSE_ERROR_UNEXPECTED_TOKEN(**tokens, "valid integer literal");
        }
        END_NODE(node);
        return node;
        case TOKEN_NUMERIC_DECIMAL_LIT:
        ALLOC_NODE_DUMMY(AST_NODE_DECIMAL_LIT);
        START_NODE(node);
        node->data.decimal_lit.lit = strtod(EAT(TOKEN_NUMERIC_DECIMAL_LIT)->value, NULL);
        END_NODE(node);
        return node;
        case TOKEN_STRING_LIT:
        ALLOC_NODE_DUMMY(AST_NODE_STRING_LIT);
        START_NODE(node);
        node->data.string_lit.lit = EAT(TOKEN_STRING_LIT)->value;
        END_NODE(node);
        return node;
        case TOKEN_CHAR_LIT:
        ALLOC_NODE_DUMMY(AST_NODE_CHAR_LIT);
        START_NODE(node);
        node->data.char_lit.lit = EAT(TOKEN_CHAR_LIT)->value;
        END_NODE(node);
        return node;
        case TOKEN_IDENTIFIER:
        START_DUMMY_NODE();
        STORE_TOKEN_STATE(state1);
        char* v1 = EAT(TOKEN_IDENTIFIER)->value;
        if (MATCH_TYPE()) {
            STORE_TOKEN_STATE(state2);
            RESTORE_TOKEN_STATE(state1);
            node = parse_vardecl(ctx, tokens, token_count, 0, 0, 0, 0, 1, 0);
            if (node != NULL) {
                return node;
            }
            RESTORE_TOKEN_STATE(state2);
        } else if (MATCH(TOKEN_COLON)) {
            ALLOC_NODE_DUMMY(AST_NODE_LABEL);
            COPY_DUMMY_TO_REAL(node);
            node->data.label.name = v1;
            END_NODE(node);
            return node;
        }
        ALLOC_NODE_DUMMY(AST_NODE_IDENTIFIER);
        COPY_DUMMY_TO_REAL(node);
        node->data.identifier.identifier = v1;
        END_NODE(node);
        return node;
    }
    PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
}

struct ast_node* parse_postfix_unary_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_primary_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (EAT(TOKEN_INC)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY_POSTFIX);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary_postfix.child = base;
            node->data.unary_postfix.unary_op = UNARY_OP_INC;
            END_NODE(node);
            base = node;
        } else if (EAT(TOKEN_DEC)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY_POSTFIX);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary_postfix.child = base;
            node->data.unary_postfix.unary_op = UNARY_OP_DEC;
            END_NODE(node);
            base = node;
        } else if (EAT(TOKEN_LPAREN)) {
            ALLOC_NODE_DUMMY(AST_NODE_CALL);
            COPY_DUMMY_TO_REAL(node);
            node->data.call.func = base;
            if (!MATCH(TOKEN_RPAREN)) {
                node->data.call.parameters = arraylist_new(4, sizeof(struct ast_node*));
                do {
                    struct ast_node* child = parse_assignment_expression(ctx, tokens, token_count);
                    CHECK_EXPR_AND(child, free_ast_node(node));
                    arraylist_addptr(node->data.call.parameters, child);
                } while (EAT(TOKEN_COMMA));
            }
            EXPECT_TOKEN(TOKEN_RPAREN, ")");
            END_NODE(node);
            base = node;
        } else if (EAT(TOKEN_LBRACK)) {
            ALLOC_NODE_DUMMY(AST_NODE_CALC_MEMBER);
            COPY_DUMMY_TO_REAL(node);
            node->data.calc_member.parent = base;
            node->data.calc_member.calc = parse_expression(ctx, tokens, token_count);
            EXPECT_TOKEN(TOKEN_RBRACK, "]");
            END_NODE(node);
            base = node;
        } else if (EAT(TOKEN_PERIOD)) { // not unary, but same precedence
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_identifier(ctx, tokens, token_count);
            node->data.binary.op = BINARY_OP_MEMBER;
            END_NODE(node);
            base = node;
        } else {
            break;
        }
    }
    return base;
}

struct ast_node* parse_unary_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    struct ast_node* base = NULL;
    struct ast_node** next_child = NULL;
    while (1) {
        START_DUMMY_NODE();
        if (EAT(TOKEN_INC)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_INC;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_DEC)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_DEC;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_PLUS)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_PLUS;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_MINUS)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_PLUS;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_LNOT)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_LNOT;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_NOT)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_NOT;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_MUL)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_DEREF;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_AND)) {
            ALLOC_NODE_DUMMY(AST_NODE_UNARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.unary.unary_op = UNARY_OP_REF;
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.unary.child;
        } else if (EAT(TOKEN_LPAREN)) {
            ALLOC_NODE_DUMMY(AST_NODE_CAST);
            STORE_TOKEN_STATE(state1);
            COPY_DUMMY_TO_REAL(node);
            node->data.cast.type = parse_type(ctx, tokens, token_count, 0, 1, 1);
            if (node->data.cast.type == NULL || !EAT(TOKEN_RPAREN)) {
                RESTORE_TOKEN_STATE(state1);
                BACKTRACK();
                break;
            }
            if (base == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.cast.expr;
        } else {
            break;
        }
    }
    struct ast_node* end = parse_postfix_unary_expression(ctx, tokens, token_count);
    if (base == NULL) {
        return end;
    } else if (end == NULL) {
        free_ast_node(base);
        return end;
    } else {
        for (struct ast_node* tbase = base; tbase != NULL; tbase = tbase->type == AST_NODE_UNARY ? tbase->data.unary.child : tbase->data.cast.expr) {
            END_NODE(tbase);
        }
        *next_child = end;
        return base;
    }
}

struct ast_node* parse_multiplicative_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_unary_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_MUL:
            op = BINARY_OP_MUL;
            goto output_binary;
            case TOKEN_DIVIDE:
            op = BINARY_OP_DIV;
            goto output_binary;
            case TOKEN_MODULUS:
            op = BINARY_OP_MOD;
            output_binary:;
            SKIP_TOKEN();
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_unary_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_additive_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_multiplicative_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_PLUS:
            op = BINARY_OP_PLUS;
            goto output_binary;
            case TOKEN_MINUS:
            op = BINARY_OP_MINUS;
            output_binary:;
            SKIP_TOKEN();
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_multiplicative_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_shift_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_additive_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_LSH:
            op = BINARY_OP_LSH;
            goto output_binary;
            case TOKEN_RSH:
            op = BINARY_OP_RSH;
            output_binary:;
            SKIP_TOKEN();
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_additive_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_comparison_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_shift_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_LT:
            op = BINARY_OP_LT;
            goto output_binary;
            case TOKEN_LTE:
            op = BINARY_OP_LTE;
            goto output_binary;
            case TOKEN_GT:
            op = BINARY_OP_GT;
            goto output_binary;
            case TOKEN_GTE:
            op = BINARY_OP_GTE;
            goto output_binary;
            case TOKEN_INST:
            op = BINARY_OP_INST;
            output_binary:;
            SKIP_TOKEN();
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_shift_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_equality_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_comparison_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_EQUAL:
            op = BINARY_OP_EQ;
            goto output_binary;
            case TOKEN_NOT_EQUAL:
            op = BINARY_OP_NEQ;
            goto output_binary;
            case TOKEN_VAL_EQUAL:
            op = BINARY_OP_EQ_VAL;
            goto output_binary;
            case TOKEN_VAL_NOT_EQUAL:
            op = BINARY_OP_NEQ_VAL;
            output_binary:;
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_comparison_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_and_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_equality_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_AND)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_equality_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_AND;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_xor_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_and_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_XOR)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_and_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_XOR;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_or_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_xor_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_OR)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_xor_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_OR;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_land_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_or_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_LAND)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_or_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_LAND;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_lor_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_land_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_LOR)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_land_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_LOR;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_mod_assignment_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_lor_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (*token_count <= 0) break;
        uint16_t op = -1;
        switch ((*tokens)[0]->type) {
            case TOKEN_LOR_EQUALS:
            op = BINARY_OP_LOR_ASSN;
            goto output_binary;
            case TOKEN_LOR_EQUALS_PRE:
            op = BINARY_OP_LOR_ASSN_PRE;
            goto output_binary;
            case TOKEN_LAND_EQUALS:
            op = BINARY_OP_LAND_ASSN;
            goto output_binary;
            case TOKEN_LAND_EQUALS_PRE:
            op = BINARY_OP_LAND_ASSN_PRE;
            goto output_binary;
            case TOKEN_OR_EQUALS:
            op = BINARY_OP_OR_ASSN;
            goto output_binary;
            case TOKEN_OR_EQUALS_PRE:
            op = BINARY_OP_OR_ASSN_PRE;
            goto output_binary;
            case TOKEN_XOR_EQUALS:
            op = BINARY_OP_XOR_ASSN;
            goto output_binary;
            case TOKEN_XOR_EQUALS_PRE:
            op = BINARY_OP_XOR_ASSN_PRE;
            goto output_binary;
            case TOKEN_AND_EQUALS:
            op = BINARY_OP_AND_ASSN;
            goto output_binary;
            case TOKEN_AND_EQUALS_PRE:
            op = BINARY_OP_AND_ASSN_PRE;
            goto output_binary;
            case TOKEN_LSH_EQUALS:
            op = BINARY_OP_LSH_ASSN;
            goto output_binary;
            case TOKEN_RSH_EQUALS:
            op = BINARY_OP_RSH_ASSN;
            goto output_binary;
            case TOKEN_LSH_EQUALS_PRE:
            op = BINARY_OP_LSH_ASSN_PRE;
            goto output_binary;
            case TOKEN_RSH_EQUALS_PRE:
            op = BINARY_OP_RSH_ASSN_PRE;
            goto output_binary;
            case TOKEN_PLUS_EQUALS:
            op = BINARY_OP_PLUS_ASSN;
            goto output_binary;
            case TOKEN_MINUS_EQUALS:
            op = BINARY_OP_MINUS_ASSN;
            goto output_binary;
            case TOKEN_PLUS_EQUALS_PRE:
            op = BINARY_OP_PLUS_ASSN_PRE;
            goto output_binary;
            case TOKEN_MINUS_EQUALS_PRE:
            op = BINARY_OP_MINUS_ASSN_PRE;
            goto output_binary;
            case TOKEN_MUL_EQUALS:
            op = BINARY_OP_MUL_ASSN;
            goto output_binary;
            case TOKEN_DIVIDE_EQUALS:
            op = BINARY_OP_DIV_ASSN;
            goto output_binary;
            case TOKEN_MODULUS_EQUALS:
            op = BINARY_OP_MOD_ASSN;
            goto output_binary;
            case TOKEN_MUL_EQUALS_PRE:
            op = BINARY_OP_MUL_ASSN_PRE;
            goto output_binary;
            case TOKEN_DIVIDE_EQUALS_PRE:
            op = BINARY_OP_DIV_ASSN_PRE;
            goto output_binary;
            case TOKEN_MODULUS_EQUALS_PRE:
            op = BINARY_OP_MOD_ASSN_PRE;
            output_binary:;
            SKIP_TOKEN();
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.left = base;
            node->data.binary.right = parse_lor_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
            node->data.binary.op = op;
            base = node;
            default:
            goto ret;
        }
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->end_col == tbase->start_col; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_ternary_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_mod_assignment_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (1) {
        if (!EAT(TOKEN_QMARK)) break;
        ALLOC_NODE_DUMMY(AST_NODE_TERNARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.ternary.condition = base;
        node->data.ternary.if_true = parse_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.ternary.if_true, free_ast_node(base));
        EXPECT_TOKEN(TOKEN_COLON, ":");
        node->data.ternary.if_false = parse_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.ternary.if_false, free_ast_node(base));
        base = node;
    }
    ret:;
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_TERNARY; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}


struct ast_node* parse_assignment_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_ternary_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    struct ast_node** next_child = NULL;
    struct ast_node* pre = base;
    while (1) {
        // all modified assignments are now equal precedence to their non-assignment counterparts, and left-to-right associative
        if (EAT(TOKEN_EQUALS)) {
            ALLOC_NODE_DUMMY(AST_NODE_BINARY);
            COPY_DUMMY_TO_REAL(node);
            node->data.binary.op = BINARY_OP_ASSN;
            node->data.binary.left = pre;
            if (next_child == NULL) {
                base = node;
            } else {
                *next_child = node;
            }
            next_child = &node->data.binary.right;
            pre = parse_ternary_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(pre, free_ast_node(base));
        } else {
            break;
        }
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->data.binary.op == BINARY_OP_ASSN; tbase = tbase->data.binary.right) {
        END_NODE(tbase);
    }
    if (next_child != NULL) {
        *next_child = pre;
    }
    return base;
}

struct ast_node* parse_sequence_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    DUMMY_NODE();
    if (*token_count == 0) PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "expression");
    START_DUMMY_NODE();
    struct ast_node* base = parse_assignment_expression(ctx, tokens, token_count);
    CHECK_EXPR(base);
    while (EAT(TOKEN_COMMA)) {
        ALLOC_NODE_DUMMY(AST_NODE_BINARY);
        COPY_DUMMY_TO_REAL(node);
        node->data.binary.left = base;
        node->data.binary.right = parse_assignment_expression(ctx, tokens, token_count);
        CHECK_EXPR_AND(node->data.binary.right, free_ast_node(base));
        node->data.binary.op = BINARY_OP_SEQUENCE;
        base = node;
    }
    for (struct ast_node* tbase = base; tbase != NULL && tbase->type == AST_NODE_BINARY && tbase->data.binary.op == BINARY_OP_SEQUENCE; tbase = tbase->data.binary.left) {
        END_NODE(tbase);
    }
    return base;
}

struct ast_node* parse_expression(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    return parse_sequence_expression(ctx, tokens, token_count);
}

struct ast_node* parse_expression_maybe_semicolon(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    if (EAT(TOKEN_SEMICOLON)) {
        INIT_PARSE_FUNC();
        ALLOC_NODE(AST_NODE_EMPTY);
        return node;
    }
    struct ast_node* node = parse_expression(ctx, tokens, token_count);
    CHECK_EXPR(node);
    while (EAT(TOKEN_SEMICOLON));
    return node;
}

struct ast_node* parse_vardecl(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t csig, uint8_t can_variadic, uint8_t can_init, uint8_t can_semi) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_VAR_DECL);
    START_NODE(node);
    node->data.vardecl.prot = prot;
    node->data.vardecl.synch = synch;
    node->data.vardecl.csig = csig;
    node->data.vardecl.type = parse_type(ctx, tokens, token_count, can_variadic, 1, 1);
    CHECK_EXPR_AND(node->data.vardecl.type, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_IDENTIFIER, "identifier");
    node->data.vardecl.name = ttok->value;
    if (!node->data.vardecl.type->data.type.variadic && can_init) {
        if (EAT(TOKEN_EQUALS)) {
            node->data.vardecl.init = parse_assignment_expression(ctx, tokens, token_count);
            CHECK_EXPR_AND(node->data.vardecl.init, free_ast_node(node));
        } else if (EAT(TOKEN_LPAREN)) {
            if (!EAT(TOKEN_RPAREN)) {
                node->data.vardecl.cons_init = arraylist_new(4, sizeof(struct ast_node*));
                do {
                    struct ast_node* nodex = parse_assignment_expression(ctx, tokens, token_count);
                    CHECK_EXPR_AND(nodex, free_ast_node(node));
                    arraylist_add(node->data.vardecl.cons_init, nodex);
                } while(EAT(TOKEN_COMMA));
                EXPECT_TOKEN(TOKEN_RPAREN, ")");
            }
            node->data.vardecl.cons_init = arraylist_new(1, sizeof(struct ast_node*));
        }
    }
    if (can_semi) {
        while (EAT(TOKEN_SEMICOLON));
    }
    END_NODE(node);
    return node;
}

struct ast_node* parse_func(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t virt, uint8_t async, uint8_t csig) {
    INIT_PARSE_FUNC()
    ALLOC_NODE(AST_NODE_FUNC);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_FUNC, "func");
    node->data.func.prot = prot;
    node->data.func.synch = synch;
    node->data.func.virt = virt;
    node->data.func.async = async;
    node->data.func.csig = csig;
    node->data.func.return_type = parse_type(ctx, tokens, token_count, 0, 1, 1);
    CHECK_EXPR_AND(node->data.func.return_type, free_ast_node(node));
    struct token* name_token = EAT(TOKEN_IDENTIFIER);
    node->data.func.name = name_token == NULL ? NULL : name_token->value;
    EXPECT_TOKEN(TOKEN_LPAREN, "(");
    if (!EAT(TOKEN_RPAREN)) {
        node->data.func.arguments = arraylist_new(4, sizeof(struct ast_node*));
        do {
            struct ast_node* vardecl = parse_vardecl(ctx, tokens, token_count, 0, 0, 0, 1, 1, 0);
            CHECK_EXPR_AND(vardecl, free_ast_node(node));
            arraylist_addptr(node->data.func.arguments, vardecl);
        } while (EAT(TOKEN_COMMA));
        EXPECT_TOKEN(TOKEN_RPAREN, ")");
    }
    node->data.func.body = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data.func.body, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_lambda_func(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t synch, uint8_t virt, uint8_t async, uint8_t csig) {
    INIT_PARSE_FUNC()
    ALLOC_NODE(AST_NODE_FUNC);
    START_NODE(node);
    node->data.func.prot = prot;
    node->data.func.synch = synch;
    node->data.func.virt = virt;
    node->data.func.async = async;
    node->data.func.csig = csig;
    EXPECT_TOKEN(TOKEN_LT, "<");
    if (!EAT(TOKEN_GT)) {
        node->data.func.arguments = arraylist_new(4, sizeof(struct ast_node*));
        do {
            struct ast_node* vardecl = parse_vardecl(ctx, tokens, token_count, 0, 0, 0, 1, 1, 0);
            CHECK_EXPR_AND(vardecl, free_ast_node(node));
            arraylist_addptr(node->data.func.arguments, vardecl);
        } while (EAT(TOKEN_COMMA));
        EXPECT_TOKEN(TOKEN_GT, ">");
    }
    node->data.func.return_type = parse_type(ctx, tokens, token_count, 0, 1, 1);
    CHECK_EXPR_AND(node->data.func.return_type, free_ast_node(node));
    EXPECT_TOKEN(TOKEN_ARROW, "=>");
    node->data.func.body = parse_expression_maybe_semicolon(ctx, tokens, token_count);
    CHECK_EXPR_AND(node->data.func.body, free_ast_node(node));
    END_NODE(node);
    return node;
}

struct ast_node* parse_class(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot, uint8_t typed, uint8_t synch, uint8_t virt, uint8_t iface) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_CLASS);
    START_NODE(node);
    node->data.class.prot = prot;
    node->data.class.typed = typed;
    node->data.class.synch = synch;
    node->data.class.virt = virt;
    node->data.class.iface = iface;
    EXPECT_TOKEN(TOKEN_CLASS, "class");
    
    node->data.class.name = ttok->value;
    if (EAT(TOKEN_COLON)) {
        node->data.class.parents = arraylist_new(2, sizeof(struct ast_node*));
        do {
            struct ast_node* type = parse_type(ctx, tokens, token_count, 0, 0, 1);
            CHECK_EXPR_AND(type, free_ast_node(node));
            arraylist_addptr(node->data.class.parents, type);
        } while (EAT(TOKEN_COMMA));
    }
    node->data.class.body = scalloc(sizeof(struct ast_node));
    node->data.class.body->type = AST_NODE_BODY;
    node->data.class.body->data.body.children = arraylist_new(8, sizeof(struct ast_node*));
    START_NODE(node->data.class.body);
    EXPECT_TOKEN(TOKEN_LCURLY, "{");
    while (1) {
        uint8_t prot = maybe_protection(tokens, token_count);
        uint8_t synch = EAT(TOKEN_SYNCH);
        uint8_t virt = EAT(TOKEN_VIRT);
        uint8_t async = EAT(TOKEN_ASYNC);
        uint8_t csig = EAT(TOKEN_CSIG);
        uint8_t can_var = !virt && !async;
        struct ast_node* child_node = NULL;
        if (MATCH(TOKEN_FUNC)) {
            child_node = parse_func(ctx, tokens, token_count, prot, synch, virt, async, csig);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.class.body->data.body.children, child_node);
        } else if (MATCH(TOKEN_LT)) {
            child_node = parse_lambda_func(ctx, tokens, token_count, prot, synch, virt, async, csig);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.class.body->data.body.children, child_node);
        } else if (can_var && MATCH_TYPE()) {
            child_node = parse_vardecl(ctx, tokens, token_count, prot, synch, csig, 0, 1, 1);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.class.body->data.body.children, child_node);
        } else if (!MATCH(TOKEN_RCURLY)) {
            PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "'func', '<', type, or '}'. Confirm correct modifiers");
            if (*token_count <= 0) {
                break;
            }
        } else {
            break;
        }
    }
    EXPECT_TOKEN(TOKEN_RCURLY, "}");
    END_NODE(node->data.class.body);
    while (EAT(TOKEN_SEMICOLON));
    END_NODE(node);
    return node;
}

struct ast_node* parse_import(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_IMPORT);
    START_NODE(node);
    EXPECT_TOKEN(TOKEN_IMPORT, "import");
    if (MATCH(TOKEN_STRING_LIT)) {
        node->data.import.what = parse_primary_expression(ctx, tokens, token_count);
    } else {
        node->data.import.what = parse_postfix_unary_expression(ctx, tokens, token_count);
    }
    CHECK_EXPR_AND(node->data.import.what, free_ast_node(node));
    EAT(TOKEN_SEMICOLON);
    END_NODE(node);
    return node;
}

struct ast_node* parse_module(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count, uint8_t prot) {
    INIT_PARSE_FUNC();
    ALLOC_NODE(AST_NODE_MODULE);
    START_NODE(node);
    node->data.module.prot = prot;
    node->data.module.body = scalloc(sizeof(struct ast_node));
    node->data.module.body->type = AST_NODE_BODY;
    node->data.module.body->data.body.children = arraylist_new(4, sizeof(struct ast_node*));
    EXPECT_TOKEN(TOKEN_MODULE, "module");
    EXPECT_TOKEN(TOKEN_IDENTIFIER, "identifier");
    node->data.module.name = ttok->value;
    START_NODE(node->data.module.body);
    EXPECT_TOKEN(TOKEN_LCURLY, "{");
    while (1) {
        uint8_t prot = maybe_protection(tokens, token_count);
        uint8_t typed = EAT(TOKEN_TYPED);
        uint8_t synch = EAT(TOKEN_SYNCH);
        uint8_t virt = EAT(TOKEN_VIRT);
        uint8_t iface = EAT(TOKEN_IFACE);
        uint8_t async = EAT(TOKEN_ASYNC);
        uint8_t csig = EAT(TOKEN_CSIG);
        uint8_t can_module = !typed && !synch && !virt && !iface && !async && !csig;
        uint8_t can_class = !async && !csig;
        uint8_t can_func = !typed && !iface;
        uint8_t can_var = !typed && !virt && !iface && !async;
        struct ast_node* child_node = NULL;
        if (can_module && MATCH(TOKEN_MODULE)) {
            child_node = parse_module(ctx, tokens, token_count, prot);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (can_module && !prot && MATCH(TOKEN_IMPORT)) {
            child_node = parse_import(ctx, tokens, token_count);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (can_class && MATCH(TOKEN_CLASS)) {
            child_node = parse_class(ctx, tokens, token_count, prot, typed, synch, virt, iface);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (can_func && MATCH(TOKEN_FUNC)) {
            child_node = parse_func(ctx, tokens, token_count, prot, synch, virt, async, csig);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (can_func && MATCH(TOKEN_LT)) {
            child_node = parse_lambda_func(ctx, tokens, token_count, prot, synch, virt, async, csig);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (can_var && MATCH_TYPE()) {
            child_node = parse_vardecl(ctx, tokens, token_count, prot, synch, csig, 0, 1, 1);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            arraylist_addptr(node->data.module.body->data.body.children, child_node);
        } else if (!MATCH(TOKEN_RCURLY)) {
            PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "'module', 'class', 'func', '<', type, or '}'. Confirm correct modifiers");
            if (*token_count <= 0) {
                break;
            }
        } else {
            if (prot != PROTECTION_NONE || typed || synch || virt || iface || async || csig) {
                PARSE_ERROR_UNEXPECTED_TOKEN(**tokens, "'module', 'class', 'func', '<', type, or '}'");
            }
            break;
        }
    }
    EXPECT_TOKEN(TOKEN_RCURLY, "}");
    END_NODE(node->data.module.body);
    while (EAT(TOKEN_SEMICOLON));
    END_NODE(node);
    return node;
}

struct ast_node* parse_file(struct parse_ctx* ctx, struct token*** tokens, size_t* token_count) {
    INIT_PARSE_FUNC();
    if (*token_count == 0) {
        struct parse_error* perr = smalloc(sizeof(struct parse_error));
        size_t bs = 128;
        perr->message = smalloc(bs);
        snprintf(perr->message, bs, "Unexpected token: 'EOF' @ 0:0. Expecting 'module' or protection modifier.\n");
        perr->line = 0;
        perr->col = 0;
        perr->type = PARSE_ERROR_TYPE_UNEXPECTED_TOKEN;
        arraylist_addptr(ctx->parse_errors, perr);
        return NULL;
    }
    ALLOC_NODE(AST_NODE_FILE);
    START_NODE(node);
    node->data.file.body = scalloc(sizeof(struct ast_node));
    node->data.file.body->type = AST_NODE_BODY;
    START_NODE(node->data.file.body);
    node->data.file.body->data.body.children = arraylist_new(4, sizeof(struct ast_node*));
    while (1) {
        uint8_t prot = maybe_protection(tokens, token_count);
        if (MATCH(TOKEN_MODULE)) {
            struct ast_node* child_node = parse_module(ctx, tokens, token_count, prot);
            CHECK_EXPR_AND(child_node, free_ast_node(node));
            EAT(TOKEN_SEMICOLON);
            arraylist_addptr(node->data.file.body->data.body.children, child_node);
        } else if (prot) {
            PARSE_ERROR_UNEXPECTED_TOKEN(EOF_ERROR_TOKEN, "'module'");
        } else {
            break;
        }
    }
    if (*token_count != 0) {
        PARSE_ERROR_UNEXPECTED_TOKEN(**tokens, "EOF or 'module'");
    }
    END_NODE(node->data.file.body);
    END_NODE(node);
    return node;
}

struct parse_intermediates parse(struct arraylist* tokens_list) {
    struct token** tokens = NULL;
    struct token** otokens = tokens;
    size_t token_count = arraylist_arrayify(tokens_list, &tokens);
    struct parse_ctx* ctx = scalloc(sizeof(struct parse_ctx));
    ctx->parse_errors = arraylist_new(16, sizeof(struct parse_error*));
    struct parse_intermediates immed = (struct parse_intermediates) {ctx, parse_file(ctx, &tokens, &token_count)};
    if (otokens != NULL) free(otokens);
    return immed;
}