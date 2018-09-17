#include "smem.h"
#include "prog_ir.h"
#include "ast.h"
#include "hash.h"
#include "arraylist.h"
#include "xstring.h"
#include <stdio.h>

// 256 spaces
const char* whitespace = "                                                                                                                                                                                                                                                                ";

#define COMMA ,
#define PROG_ERROR(node, fmt, args) {arraylist_addptr(state->errors, node); fprintf(stderr, fmt "\n", args);}
#define PROG_ERROR_AST(module, node, expecting) PROG_ERROR(node, "Error: %s @ %lu:%lu.\n%s\n%s^", expecting COMMA node->start_line COMMA node->start_col COMMA arraylist_getptr(module->file->lines, node->start_line - 1) COMMA whitespace + ((node->start_col - 1) > 256 ? 0 : (256 - (node->start_col - 1))))

const char* operator_fns[] = {"op_member", "op_sequence", "op_eq_val", "op_neq_val", "op_eq", "op_neq", "op_mul", "op_div", "op_mod", "op_plus", "op_minus", "op_lsh", "op_rsh", "op_lt", "op_lte", "op_gt", "op_gte", "op_inst", "op_and", "op_xor", "op_or", "op_land", "op_lor", "op_assn", "op_mul_assn", "op_div_assn", "op_mod_assn", "op_plus_assn", "op_minus_assn", "op_lsh_assn", "op_rsh_assn", "op_and_assn", "op_xor_assn", "op_or_assn", "op_land_assn", "op_lor_assn", "op_mul_assn_pre", "op_div_assn_pre", "op_mod_assn_pre", "op_plus_assn_pre", "op_minus_assn_pre", "op_lsh_assn_pre", "op_rsh_assn_pre", "op_and_assn_pre", "op_xor_assn_pre", "op_or_assn_pre", "op_land_assn_pre", "op_lor_assn_pre"};

struct prog_type* gen_prog_type(struct prog_state* state, struct ast_node* node, struct prog_file* file, uint8_t is_master, uint8_t is_const, uint8_t is_generic) {
    if (node == NULL) return NULL;
    struct prog_type* t = scalloc(sizeof(struct prog_type));
    t->type = PROG_TYPE_UNKNOWN;
    t->is_master = is_master;
    t->is_const = is_const;
    t->file = file;
    t->master_type = NULL;
    t->ast = node;
    if (node->type == AST_NODE_TYPE) {
        if (node->data.type.protofunc) {
            t->type = PROG_TYPE_FUNC;
            t->data.func.return_type = gen_prog_type(state, node->data.type.protofunc_return_type, file, 0, 0, 0);
            t->data.func.arg_types = node->data.type.protofunc_arguments == NULL ? NULL : arraylist_new(node->data.type.protofunc_arguments->entry_count, sizeof(struct prog_type*));
            if (t->data.func.arg_types != NULL) {
                for (size_t i = 0; i < node->data.type.protofunc_arguments->entry_count; i++) {
                    arraylist_addptr(t->data.func.arg_types, gen_prog_type(state, arraylist_getptr(node->data.type.protofunc_arguments, i), file, 0, 0, 0));
                }
            }
        } else {
            t->variadic = node->data.type.variadic;
            t->array_dimensonality = node->data.type.array_dimensonality;
            t->is_ref = node->data.type.is_ref;
            t->name = node->data.type.name;
            if (node->data.type.generics != NULL) {
                t->generics = new_hashmap(4);
                t->type = PROG_TYPE_CLASS;
                if (!is_master || !is_generic)
                    for (size_t i = 0; i < node->data.type.generics->entry_count; i++) {
                        struct ast_node *entry = arraylist_getptr(node->data.type.generics, i);
                        hashmap_put(t->generics, entry->data.type.name, gen_prog_type(state, entry, file, is_master, 0, 1));
                    }
            }
        }
    } else if (node->type == AST_NODE_FUNC) {
        t->type = PROG_TYPE_FUNC;
        t->data.func.return_type = node->prog->data.func->return_type;
        if (node->prog->data.func->arguments != NULL) {
            t->data.func.arg_types = arraylist_new(node->prog->data.func->arguments->entry_count, sizeof(struct prog_type *));
            ITER_MAP(node->prog->data.func->arguments) {
                struct prog_var *var = value;
                arraylist_addptr(t->data.func.arg_types, var->type);
            ITER_MAP_END()}
        }
    }
    int32_t prim_type = -1;
    if (t->name == NULL || t->type != PROG_TYPE_UNKNOWN) {

    } else if (str_eqCase(t->name, "u8")) {
        prim_type = PRIM_U8;
    } else if (str_eqCase(t->name, "b")) {
        prim_type = PRIM_B;
    } else if (str_eqCase(t->name, "byte")) {
        prim_type = PRIM_BYTE;
    } else if (str_eqCase(t->name, "c")) {
        prim_type = PRIM_C;
    } else if (str_eqCase(t->name, "char")) {
        prim_type = PRIM_CHAR;
    } else if (str_eqCase(t->name, "uint8")) {
        prim_type = PRIM_UINT8;
    } else if (str_eqCase(t->name, "bool")) {
        prim_type = PRIM_BOOL;
    } else if (str_eqCase(t->name, "i8")) {
        prim_type = PRIM_I8;
    } else if (str_eqCase(t->name, "int8")) {
        prim_type = PRIM_INT8;
    } else if (str_eqCase(t->name, "u16")) {
        prim_type = PRIM_U16;
    } else if (str_eqCase(t->name, "ush")) {
        prim_type = PRIM_USH;
    } else if (str_eqCase(t->name, "ushort")) {
        prim_type = PRIM_USHORT;
    } else if (str_eqCase(t->name, "uint16")) {
        prim_type = PRIM_UINT16;
    } else if (str_eqCase(t->name, "i16")) {
        prim_type = PRIM_I16;
    } else if (str_eqCase(t->name, "sh")) {
        prim_type = PRIM_SH;
    } else if (str_eqCase(t->name, "short")) {
        prim_type = PRIM_SHORT;
    } else if (str_eqCase(t->name, "int16")) {
        prim_type = PRIM_INT16;
    } else if (str_eqCase(t->name, "u32")) {
        prim_type = PRIM_U32;
    } else if (str_eqCase(t->name, "u")) {
        prim_type = PRIM_U;
    } else if (str_eqCase(t->name, "uint")) {
        prim_type = PRIM_UINT;
    } else if (str_eqCase(t->name, "uint32")) {
        prim_type = PRIM_UINT32;
    } else if (str_eqCase(t->name, "i32")) {
        prim_type = PRIM_I32;
    } else if (str_eqCase(t->name, "i")) {
        prim_type = PRIM_I;
    } else if (str_eqCase(t->name, "int")) {
        prim_type = PRIM_INT;
    } else if (str_eqCase(t->name, "int32")) {
        prim_type = PRIM_INT32;
    } else if (str_eqCase(t->name, "u64")) {
        prim_type = PRIM_U64;
    } else if (str_eqCase(t->name, "ul")) {
        prim_type = PRIM_UL;
    } else if (str_eqCase(t->name, "ulong")) {
        prim_type = PRIM_ULONG;
    } else if (str_eqCase(t->name, "uint64")) {
        prim_type = PRIM_UINT64;
    } else if (str_eqCase(t->name, "i64")) {
        prim_type = PRIM_I64;
    } else if (str_eqCase(t->name, "l")) {
        prim_type = PRIM_L;
    } else if (str_eqCase(t->name, "long")) {
        prim_type = PRIM_LONG;
    } else if (str_eqCase(t->name, "int64")) {
        prim_type = PRIM_INT64;
    } else if (str_eqCase(t->name, "f")) {
        prim_type = PRIM_F;
    } else if (str_eqCase(t->name, "float")) {
        prim_type = PRIM_FLOAT;
    } else if (str_eqCase(t->name, "d")) {
        prim_type = PRIM_D;
    } else if (str_eqCase(t->name, "double")) {
        prim_type = PRIM_DOUBLE;
    }
    if (prim_type != -1) {
        t->type = PROG_TYPE_PRIMITIVE;
        t->data.prim.prim_type = prim_type;
    } else if (t->is_ref) {
        PROG_ERROR_AST(t, node, "only primitives can be passed by reference");
    }
    return t;
}

struct preprocess_ctx {
    struct prog_state* state;
    struct prog_func* func;
    struct prog_class* clas;
    struct prog_module* module;
    struct prog_file* file;
};

struct ast_node* preprocess_expr(struct ast_node* node, struct preprocess_ctx* ctx);

struct prog_func* gen_prog_func_func(struct prog_state* state, struct prog_file* file, struct ast_node* func, struct prog_func* parent) {
    struct prog_func* fun = scalloc(sizeof(struct prog_func));
    fun->name = func->data.func.name;
    fun->anonymous = func->data.func.name == NULL;
    fun->closing = parent;
    fun->file = file;
    fun->closures = arraylist_new(4, sizeof(struct prog_func*));
    fun->prot = PROTECTION_PRIV;
    fun->virt = func->data.func.virt;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->pure = func->data.func.pure || parent->pure;
    fun->stat = func->data.func.stat;
    fun->arguments = new_hashmap(4);
    fun->arguments_list = arraylist_new(16, sizeof(struct prog_var*));
    fun->node_map = new_hashmap(4);
    fun->proc.root = func;
    struct preprocess_ctx lctx = (struct preprocess_ctx) {state, fun, NULL, NULL, file};
    for (size_t i = 0; i < func->data.func.arguments->entry_count; i++) {
        struct ast_node* arg = arraylist_getptr(func->data.func.arguments, i);
        struct prog_var* var = scalloc(sizeof(struct prog_var));
        var->uid = state->next_var_id++;
        var->name = arg->data.vardecl.name;
        var->func = fun;
        var->file = fun->file;
        var->prot = PROTECTION_PRIV;
        var->type = gen_prog_type(state, arg->data.vardecl.type, file, 0, 0, 0);
        var->type->is_optional = var->proc.init != NULL || var->proc.cons_init != NULL;
        var->proc.init = arg->data.vardecl.init;
        if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
        var->proc.cons_init = arg->data.vardecl.cons_init;
        if (var->proc.cons_init != NULL) {
            for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
            }
        }
        hashmap_put(fun->arguments, var->name, var)
        arraylist_addptr(fun->arguments_list, var);
    }
    fun->return_type = gen_prog_type(state, func->data.func.return_type, file, 0, 0, 0);
    fun->proc.body = func->data.func.body;
    traverse_node(fun->proc.body, preprocess_expr, &lctx, 1);
    arraylist_addptr(parent->closures, fun);
    return fun;
}

struct prog_func* gen_prog_clas_func(struct prog_state* state, struct prog_file* file, struct ast_node* func, struct prog_class* parent);
struct prog_func* gen_prog_mod_func(struct prog_state* state, struct prog_file* file, struct ast_node* func, struct prog_module* parent);

struct ast_node* preprocess_expr(struct ast_node* node, struct preprocess_ctx* ctx) {
    if (node->type == AST_NODE_TYPE) {
        node->prog = scalloc(sizeof(struct prog_node));
        node->prog->ast_node = node;
        node->prog->prog_type = PROG_NODE_TYPE;
        node->prog->data.type = gen_prog_type(ctx->state, node, ctx->file, 0, node->data.type.cons, 1);
        struct hashmap *om = NULL;
        if (ctx->func != NULL) {
            om = ctx->func->node_map;
        } else if (ctx->clas != NULL) {
            om = ctx->clas->node_map;
        } else if (ctx->module != NULL) {
            om = ctx->module->node_map;
        }
        if (om != NULL) hashmap_putptr(om, node, node->prog);
        hashmap_putptr(ctx->state->node_map, node, node->prog);
    } else if (node->type == AST_NODE_VAR_DECL) {
        if (node->data.vardecl.cons) {
            node->data.vardecl.type->data.type.cons = 1;
        }
    } else if (node->type == AST_NODE_FUNC) {
        node->prog = scalloc(sizeof(struct prog_node));
        node->prog->ast_node = node;
        node->prog->prog_type = PROG_NODE_EXTRACTED_FUNC_REF;
        //gen_prog_type(ctx->state, node, ctx->file, 0, 1, 1);
        //TODO: if have done our variable coloring by this point, its a great place to reference our captured vars here
        hashmap_putptr(ctx->state->node_map, node, node->prog);
        if (ctx->func != NULL) {
            node->prog->data.func = gen_prog_func_func(ctx->state, ctx->file, node, ctx->func);
        } else if (ctx->clas != NULL) {
            node->prog->data.func = gen_prog_clas_func(ctx->state, ctx->file, node, ctx->clas);
        } else if (ctx->module != NULL) {
            node->prog->data.func = gen_prog_mod_func(ctx->state, ctx->file, node, ctx->module);
        }
    }
    return node;
}

struct prog_func* gen_prog_clas_func(struct prog_state* state, struct prog_file* file, struct ast_node* func, struct prog_class* parent) {
    struct prog_func* fun = scalloc(sizeof(struct prog_func));
    fun->name = func->data.func.name;
    fun->anonymous = func->data.func.name == NULL;
    fun->clas = parent;
    fun->file = file;
    fun->closures = arraylist_new(4, sizeof(struct prog_func*));
    fun->prot = parent->prot == PROTECTION_NONE || parent->prot >= func->data.func.prot ? func->data.func.prot : parent->prot;
    fun->virt = func->data.func.virt || parent->iface;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->stat = func->data.func.stat;
    fun->pure = func->data.func.pure || parent->pure;
    fun->arguments = new_hashmap(4);
    fun->arguments_list = arraylist_new(16, sizeof(struct prog_var*));
    fun->node_map = new_hashmap(4);
    struct prog_node* node = scalloc(sizeof(struct prog_node));
    node->ast_node = func;
    func->prog = node;
    node->prog_type = PROG_TYPE_FUNC;
    node->data.func = fun;
    fun->proc.root = func;
    struct preprocess_ctx lctx = (struct preprocess_ctx) {state, fun, NULL, NULL, file};
    if (func->data.func.arguments != NULL)
        for (size_t i = 0; i < func->data.func.arguments->entry_count; i++) {
            struct ast_node* arg = arraylist_getptr(func->data.func.arguments, i);
            struct prog_var* var = scalloc(sizeof(struct prog_var));
            var->uid = state->next_var_id++;
            var->name = arg->data.vardecl.name;
            var->func = fun;
            var->file = fun->file;
            var->prot = PROTECTION_PRIV;
            var->type = gen_prog_type(state, arg->data.vardecl.type, fun->file, 0, 0, 0);
            var->type->is_optional = var->proc.init != NULL || var->proc.cons_init != NULL;
            var->proc.init = arg->data.vardecl.init;
            if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
            var->proc.cons_init = arg->data.vardecl.cons_init;
            if (var->proc.cons_init != NULL) {
                for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                    if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
                }
            }
            hashmap_put(fun->arguments, var->name, var);
            arraylist_add(fun->arguments_list, var);
        }
    fun->return_type = gen_prog_type(state, func->data.func.return_type, fun->file, 0, 0, 0);
    fun->proc.body = func->data.func.body;
    traverse_node(fun->proc.body, preprocess_expr, &lctx, 1);
    if (fun->name == NULL) {
        hashmap_putptr(parent->funcs, fun, fun);
    } else {
        hashmap_put(parent->funcs, fun->name, fun);
    }
    if (fun->name != NULL) {
        struct prog_var* var = scalloc(sizeof(struct prog_var));
        var->uid = state->next_var_id++;
        var->name = fun->name;
        var->clas = parent;
        var->file = parent->file;
        var->prot = fun->prot;
        var->synch = fun->synch;
        var->csig = fun->csig;
        var->cons = 1;
        var->stat = fun->stat;
        var->type = gen_prog_type(state, func, fun->file, 0, 1, 0);
        hashmap_put(parent->vars, var->name, var);
    }
    return fun;
}

void gen_prog_clas_var(struct prog_state* state, struct prog_file* file, struct ast_node* vard, struct prog_class* parent) {
    struct prog_var* var = scalloc(sizeof(struct prog_var));
    var->uid = state->next_var_id++;
    var->name = vard->data.vardecl.name;
    var->clas = parent;
    var->file = file;
    var->prot = parent->prot == PROTECTION_NONE || parent->prot >= vard->data.vardecl.prot ? vard->data.vardecl.prot : parent->prot;
    var->synch = vard->data.vardecl.synch;
    var->csig = vard->data.vardecl.csig;
    var->stat = vard->data.vardecl.stat;
    var->cons = vard->data.vardecl.cons;
    var->type = gen_prog_type(state, vard->data.vardecl.type, file, 0, var->cons, 0);
    var->proc.init = vard->data.vardecl.init;
    struct preprocess_ctx lctx = (struct preprocess_ctx) {state, NULL, parent, NULL, file};
    if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
    var->proc.cons_init = vard->data.vardecl.cons_init;
    if (var->proc.cons_init != NULL) {
        for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
            if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
        }
    }
    hashmap_put(parent->vars, var->name, var);
}

void gen_prog_class(struct prog_state* state, struct prog_file* file, struct ast_node* clas, struct prog_module* parent) {
    struct prog_class* cl = scalloc(sizeof(struct prog_class));
    cl->name = clas->data.class.name->data.type.name;
    cl->type = gen_prog_type(state, clas->data.class.name, file, 1, 0, 0);
    cl->file = file;
    cl->type->type = PROG_TYPE_CLASS;
    cl->type->data.clas.clas = cl;
    cl->prot = parent->prot == PROTECTION_NONE || parent->prot >= clas->data.module.prot ? clas->data.module.prot : parent->prot;
    cl->virt = clas->data.class.virt;
    cl->synch = clas->data.class.synch;
    cl->iface = clas->data.class.iface;
    cl->pure = clas->data.class.pure;
    cl->module = parent;
    cl->vars = new_hashmap(4);
    cl->node_map = new_hashmap(4);
    cl->parents = clas->data.class.parents == NULL ? NULL : arraylist_new(clas->data.class.parents->entry_count, sizeof(struct ast_node*));
    cl->funcs = new_hashmap(4);
    hashmap_put(parent->classes, cl->name, cl);
    hashmap_put(parent->types, cl->name, cl->type);
    for (size_t i = 0; i < clas->data.class.body->data.body.children->entry_count; i++) {
        struct ast_node* node = arraylist_getptr(clas->data.class.body->data.body.children, i);
        if (node->type == AST_NODE_FUNC) {
            gen_prog_clas_func(state, file, node, cl);
        } else if (node->type == AST_NODE_VAR_DECL) {
            gen_prog_clas_var(state, file, node, cl);
        }
    }
    if (clas->data.class.parents != NULL)
        for (size_t i = 0; i < clas->data.class.parents->entry_count; i++) {
            struct ast_node* node = arraylist_getptr(clas->data.class.parents, i);
            arraylist_addptr(cl->parents, gen_prog_type(state, node, file, 0, 0, 0));
        }
}

struct prog_func* gen_prog_mod_func(struct prog_state* state, struct prog_file* file, struct ast_node* func, struct prog_module* parent) {
    struct prog_func* fun = scalloc(sizeof(struct prog_func));
    fun->name = func->data.func.name;
    fun->anonymous = func->data.func.name == NULL;
    fun->module = parent;
    fun->file = file;
    fun->closures = arraylist_new(4, sizeof(struct prog_func*));
    fun->prot = parent->prot == PROTECTION_NONE || parent->prot >= func->data.func.prot ? func->data.func.prot : parent->prot;
    fun->virt = func->data.func.virt;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->pure = func->data.func.pure;
    fun->stat = func->data.func.stat;
    fun->arguments = new_hashmap(4);
    fun->arguments_list = arraylist_new(16, sizeof(struct prog_var*));
    fun->node_map = new_hashmap(4);
    struct prog_node* node = scalloc(sizeof(struct prog_node));
    node->ast_node = func;
    func->prog = node;
    node->prog_type = PROG_TYPE_FUNC;
    node->data.func = fun;
    fun->proc.root = func;
    struct preprocess_ctx lctx = (struct preprocess_ctx) {state, fun, NULL, NULL, file};
    if (func->data.func.arguments != NULL)
        for (size_t i = 0; i < func->data.func.arguments->entry_count; i++) {
            struct ast_node* arg = arraylist_getptr(func->data.func.arguments, i);
            struct prog_var* var = scalloc(sizeof(struct prog_var));
            var->uid = state->next_var_id++;
            var->name = arg->data.vardecl.name;
            var->func = fun;
            var->file = fun->file;
            var->prot = PROTECTION_PRIV;
            var->type = gen_prog_type(state, arg->data.vardecl.type, file, 0, 0, 0);
            var->type->is_optional = var->proc.init != NULL || var->proc.cons_init != NULL;
            var->proc.init = arg->data.vardecl.init;
            if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
            var->proc.cons_init = arg->data.vardecl.cons_init;
            if (var->proc.cons_init != NULL) {
                for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                    if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
                }
            }
            hashmap_put(fun->arguments, var->name, var);
            arraylist_add(fun->arguments_list, var);
        }
    fun->return_type = gen_prog_type(state, func->data.func.return_type, file, 0, 0, 0);
    fun->proc.body = func->data.func.body;
    traverse_node(fun->proc.body, preprocess_expr, &lctx, 1);
    if (fun->name == NULL) {
        hashmap_putptr(parent->funcs, fun, fun);
    } else {
        hashmap_put(parent->funcs, fun->name, fun);
    }
    if (fun->name != NULL) {
        struct prog_var* var = scalloc(sizeof(struct prog_var));
        var->uid = state->next_var_id++;
        var->name = fun->name;
        var->file = fun->file;
        var->module = parent;
        var->prot = fun->prot;
        var->synch = fun->synch;
        var->csig = fun->csig;
        var->cons = 1;
        var->stat = fun->stat;
        var->type = gen_prog_type(state, func, fun->file, 0, 1, 0);
        hashmap_put(parent->vars, var->name, var);
    }
    return fun;
}

void gen_prog_mod_var(struct prog_state* state, struct prog_file* file, struct ast_node* vard, struct prog_module* parent) {
    struct prog_var* var = scalloc(sizeof(struct prog_var));
    var->uid = state->next_var_id++;
    var->name = vard->data.vardecl.name;
    var->module = parent;
    var->file = file;
    var->prot = parent->prot == PROTECTION_NONE || parent->prot >= vard->data.vardecl.prot ? vard->data.vardecl.prot : parent->prot;
    var->synch = vard->data.vardecl.synch;
    var->csig = vard->data.vardecl.csig;
    var->stat = vard->data.vardecl.stat;
    var->cons = vard->data.vardecl.cons;
    var->type = gen_prog_type(state, vard->data.vardecl.type, file, 0, var->cons, 0);
    var->proc.init = vard->data.vardecl.init;
    struct preprocess_ctx lctx = (struct preprocess_ctx) {state, NULL, NULL, parent, file};
    if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
    var->proc.cons_init = vard->data.vardecl.cons_init;
    if (var->proc.cons_init != NULL) {
        for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
            if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
        }
    }
    hashmap_put(parent->vars, var->name, var);
}

void gen_prog_module(struct prog_state* state, struct prog_file* file, struct ast_node* module, struct prog_module* parent) {
    struct prog_module* mod = NULL;
    struct {
        struct prog_file *file;
    } file_cont;
    file_cont.file = file;
    for (size_t i = 0; i < module->data.module.name_list->entry_count; i++) {
        int is_last = i == module->data.module.name_list->entry_count - 1;
        char* ident = arraylist_getptr(module->data.module.name_list, i);
        if (mod != NULL) {
            parent = mod;
            mod = NULL;
        }
        if (parent == NULL) {
            mod = hashmap_get(state->modules, ident);
        } else {
            mod = hashmap_get(parent->submodules, ident);
        }
        if (mod == NULL) {
            mod = scalloc(sizeof(struct prog_module));
            mod->name = ident;
            if (is_last) {
                mod->prot = module->data.module.prot;
            } else {
                mod->prot = PROTECTION_NONE;
            }
            mod->file = file;
            mod->submodules = new_hashmap(4);
            mod->vars = new_hashmap(4);
            mod->node_map = new_hashmap(4);
            mod->classes = new_hashmap(4);
            mod->funcs = new_hashmap(4);
            mod->types = new_hashmap(16);
            mod->imported_modules = arraylist_new(4, sizeof(struct ast_node *));
            mod->parent = parent;
        } else {
            if (is_last && mod->prot != module->data.module.prot) {
                PROG_ERROR_AST((&file_cont), module, "conflicting module properties");
            }
        }
        if (parent == NULL) {
            hashmap_put(state->modules, mod->name, mod);
        } else {
            hashmap_put(parent->submodules, mod->name, mod);
        }
    }
    if (mod == NULL) {
        return;
    }
    for (size_t i = 0; i < module->data.module.body->data.body.children->entry_count; i++) {
        struct ast_node* node = arraylist_getptr(module->data.module.body->data.body.children, i);
        if (node->type == AST_NODE_MODULE) {
            gen_prog_module(state, file, node, mod);
        } else if (node->type == AST_NODE_CLASS) {
            gen_prog_class(state, file, node, mod);
        } else if (node->type == AST_NODE_FUNC) {
            gen_prog_mod_func(state, file, node, mod);
        } else if (node->type == AST_NODE_VAR_DECL) {
            gen_prog_mod_var(state, file, node, mod);
        } else if (node->type == AST_NODE_IMPORT) {
            node->data.import.file = file;
            arraylist_addptr(mod->imported_modules, node);
        } else {
            PROG_ERROR_AST((&file_cont), node, "illegal AST in module");
        }
    }
}

void resolve_module_deps(struct prog_state* state, struct prog_module* mod) {
    // if this is slow, make imported_modules a hashset
    struct arraylist* new_imported_modules = NULL;
    if (mod->parent != NULL) {
        new_imported_modules = arraylist_new(mod->imported_modules->entry_count + mod->parent->imported_modules->entry_count + 1, sizeof(struct prog_module*));
        for (size_t i = 0; i < mod->parent->imported_modules->entry_count; i++) {
            arraylist_addptr(new_imported_modules, arraylist_getptr(mod->parent->imported_modules, i));
        }
        if (arraylist_indexptr(new_imported_modules, mod->parent) == -1) arraylist_addptr(new_imported_modules, mod->parent);
    } else {
        new_imported_modules = arraylist_new(mod->imported_modules->entry_count, sizeof(struct prog_module*));
    }
    for (size_t i = 0; i < mod->imported_modules->entry_count; i++) {
        struct ast_node* node = arraylist_getptr(mod->imported_modules, i);
        struct ast_node* prim = node;
        node = node->data.import.what;
        if (node->type == AST_NODE_STRING_LIT) {
            //TODO: C imports
        } else {
            struct prog_module* resolved_module = NULL;
            while ((node->type == AST_NODE_BINARY && node->data.binary.op == BINARY_OP_MEMBER) || node->type == AST_NODE_IDENTIFIER) {
                if (node->type == AST_NODE_BINARY && (node->data.binary.left == NULL || node->data.binary.left->type != AST_NODE_IDENTIFIER)) {
                    PROG_ERROR_AST((&(prim->data.import)), node, "expecting 'identifier'");
                    goto cont_imports;
                } else {
                    struct ast_node* ident = node;
                    if (node->type == AST_NODE_BINARY) {
                        ident = node->data.binary.left;
                    }
                    if (resolved_module == NULL) {
                        resolved_module = hashmap_get(state->modules, ident->data.identifier.identifier);
                        if (resolved_module == NULL) {
                            resolved_module = hashmap_get(mod->submodules, ident->data.identifier.identifier);
                            if (resolved_module == NULL) {
                                PROG_ERROR_AST((&(prim->data.import)), node, "module not found");
                                goto cont_imports;
                            } else if (resolved_module->prot == PROTECTION_PRIV) {
                                PROG_ERROR_AST((&(prim->data.import)), ident, "module is private");
                                resolved_module = NULL;
                                goto cont_imports;
                            }
                        } else if (resolved_module->prot == PROTECTION_PRIV) { // TODO: protected semantics
                            PROG_ERROR_AST((&(prim->data.import)), ident, "module is private");
                            resolved_module = NULL;
                            goto cont_imports;
                        }
                    } else if (resolved_module->prot == PROTECTION_PRIV) {
                        PROG_ERROR_AST((&(prim->data.import)), ident, "module is private");
                        resolved_module = NULL;
                        goto cont_imports;
                    } else {
                        resolved_module = hashmap_get(resolved_module->submodules, ident->data.identifier.identifier);
                        if (resolved_module == NULL) {
                            PROG_ERROR_AST((&(prim->data.import)), node, "submodule not found");
                            goto cont_imports;
                        } else if (resolved_module->prot == PROTECTION_PRIV) {
                            PROG_ERROR_AST((&(prim->data.import)), ident, "module is private");
                            resolved_module = NULL;
                            goto cont_imports;
                        }
                    }
                    if (node->type == AST_NODE_IDENTIFIER) {
                        node = NULL;
                        break;
                    } else {
                        node = node->data.binary.right;
                    }
                }
            }
            if (resolved_module == NULL) {
                PROG_ERROR_AST((&(prim->data.import)), prim, "module not found");
                goto cont_imports;
            } else if (node != NULL && node->type != AST_NODE_EMPTY) {
                PROG_ERROR_AST((&(prim->data.import)), node, "expecting end of statement");
                goto cont_imports;
            }
            if (arraylist_indexptr(new_imported_modules, resolved_module) == -1) arraylist_addptr(new_imported_modules, resolved_module);
        }
        cont_imports:;
    }
    arraylist_free(mod->imported_modules);
    mod->imported_modules = new_imported_modules;
    ITER_MAP(mod->submodules) {
        resolve_module_deps(state, value);
    ITER_MAP_END()}
}

void scope_module_types(struct prog_state* state, struct prog_module* mod) {
    // if this is slow, make imported_modules a hashset
    for (ssize_t i = mod->imported_modules->entry_count - 1; i >= 0; i--) {
        struct prog_module* import = arraylist_getptr(mod->imported_modules, i);
        ITER_MAP(import->types) {
            if (!hashmap_get(mod->types, str_key)) {
                hashmap_put(mod->types, str_key, value);
            }
        ITER_MAP_END()};
    }
    ITER_MAP(mod->submodules) {
        scope_module_types(state, value);
    ITER_MAP_END()}
}

void provide_master_types(struct prog_state* state, struct prog_module* mod, struct prog_file* file, struct prog_class* clas, struct prog_func* func, struct prog_type* type, uint8_t no_immed_generics) {
    if (type == NULL || type->is_master || type->master_type != NULL) return;
    if (type->type == PROG_TYPE_FUNC || type->type == PROG_TYPE_PRIMITIVE) {
        return; // funcs/prims have no master
    }
    struct prog_type* master = clas == NULL || no_immed_generics || clas->type->generics == NULL ? NULL : hashmap_get(clas->type->generics, type->name);
    if (master == NULL) {
        master = hashmap_get(mod->types, type->name);
    } else {
        type->is_generic = 1;
    }
    struct {
        struct prog_file* file;
    } file_cont;
    file_cont.file = file;
    if (master == NULL) {
        PROG_ERROR_AST((&file_cont), type->ast, "type not found");
        return;
    }
    type->type = PROG_TYPE_CLASS;
    if ((type->generics == NULL) != (master->generics == NULL)) {
        char msg[256];
        snprintf(msg, 256, "%s %lu found, expected %lu", type->generics == NULL ? "expected generics:" : "illegal generics:", type->generics->entry_count, master->generics->entry_count);
        PROG_ERROR_AST((&file_cont), type->ast, msg);
        return;
    }
    if (type->generics != NULL && type->generics->entry_count != master->generics->entry_count) {
        char msg[256];
        snprintf(msg, 256, "found %lu generics, expected %lu", type->generics->entry_count, master->generics->entry_count);
        PROG_ERROR_AST((&file_cont), type->ast, msg);
        return;
    }
    type->master_type = master;
    if (type->generics != NULL)
        ITER_MAP(type->generics) {
            provide_master_types(state, mod, func->file, clas, func, value, 0);
        ITER_MAP_END()}
}


void propagate_mod_types(struct prog_state* state, struct prog_module* mod) {
    ITER_MAP(mod->classes) {
        struct prog_class* clas = value;
        if (clas->parents != NULL)
            for (size_t j = 0; j < clas->parents->entry_count; j++) {
                // we don't want generics in our parents, but it's alright in their generics
                provide_master_types(state, mod, clas->file, clas, NULL, arraylist_getptr(clas->parents, j), 1);
            }
        ITER_MAP(clas->vars) {
            struct prog_var* var = value;
            provide_master_types(state, mod, clas->file, clas, NULL, var->type, 0);
        ITER_MAP_END()}
        ITER_MAP(clas->node_map) {
            struct ast_node* t_node = ptr_key;
            struct prog_node* p_node = value;
            if (p_node->prog_type == PROG_NODE_TYPE) {
                provide_master_types(state, mod, clas->file, clas, NULL, p_node->data.type, 0);
            }
        ITER_MAP_END()}
        ITER_MAP(clas->funcs) {
            struct prog_func* func = value;
            provide_master_types(state, mod, clas->file, clas, func, func->return_type, 0);
            for(size_t i = 0; i < func->arguments_list->entry_count; i++) {
                provide_master_types(state, mod, func->file, NULL, func, arraylist_getptr(func->arguments_list, i), 0);
            }
            ITER_MAP(func->node_map) {
                struct ast_node* t_node = ptr_key;
                struct prog_node* p_node = value;
                if (p_node->prog_type == PROG_NODE_TYPE) {
                    provide_master_types(state, mod, func->file, NULL, func, p_node->data.type, 0);
                }
            ITER_MAP_END()}
        ITER_MAP_END()}
    ITER_MAP_END()}
    ITER_MAP(mod->node_map) {
        struct ast_node* t_node = ptr_key;
        struct prog_node* p_node = value;
        if (p_node->prog_type == PROG_NODE_TYPE) {
            provide_master_types(state, mod, p_node->data.type->file, NULL, NULL, p_node->data.type, 0);
        }
    ITER_MAP_END()}
    ITER_MAP(mod->vars) {
        struct prog_var* var = value;
        provide_master_types(state, mod, var->file, NULL, NULL, var->type, 0);
    ITER_MAP_END()}
    ITER_MAP(mod->funcs) {
        struct prog_func* func = value;
        provide_master_types(state, mod, func->file, NULL, func, func->return_type, 0);
        for(size_t i = 0; i < func->arguments_list->entry_count; i++) {
            provide_master_types(state, mod, func->file, NULL, func, arraylist_getptr(func->arguments_list, i), 0);
        }
        ITER_MAP(func->node_map) {
            struct ast_node* t_node = ptr_key;
            struct prog_node* p_node = value;
            if (p_node->prog_type == PROG_NODE_TYPE) {
                provide_master_types(state, mod, func->file, NULL, func, p_node->data.type, 0);
            }
        ITER_MAP_END()}
    ITER_MAP_END()}
    ITER_MAP(mod->submodules) {
        propagate_mod_types(state, value);
    ITER_MAP_END()}
}

struct prog_scope {
    struct prog_scope* parent;
    struct arraylist* children;
    struct hashmap* vars;
    uint8_t copied_ref;
    struct ast_node* ast_node;
    uint8_t ascending_makes_closure;
    uint8_t exit_expr_scope;
    uint8_t is_param_level;
    uint8_t is_class_level;
};

#define ALLOC_REF_SCOPE(name, node, ref) struct prog_scope* name = scalloc(sizeof(struct prog_scope)); name->copied_ref = 1; name->vars = ref; name->parent = stack; name->children = arraylist_new(4, sizeof(struct prog_scope*)); name->ast_node = node; arraylist_addptr(scope->children, name);
#define ALLOC_SCOPE_PARENT(name, stack, node) struct prog_scope* name = scalloc(sizeof(struct prog_scope)); name->copied_ref = 0; name->vars = new_hashmap(8); name->parent = stack; name->children = arraylist_new(4, sizeof(struct prog_scope*)); name->ast_node = node; arraylist_addptr(scope->children, name);
#define ALLOC_SCOPE(name, node) ALLOC_SCOPE_PARENT(name, stack, node)


#define TRAVERSE_NEW_FUNC(item) root->type == AST_NODE_FUNC ? root : nearest_func
#define TRAVERSE(item) scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, stack);
#define TRAVERSE_SCOPED(item, name) if (item != NULL) { ALLOC_SCOPE(scope, item); name = scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); }
#define TRAVERSE_PRESCOPED_SPEC(item, scope) if (item != NULL) { scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); }
#define TRAVERSE_PRESCOPED(item) TRAVERSE_PRESCOPED_SPEC(item, scope)
#define TRAVERSE_ARRAYLIST(list, name) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); name = scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, stack); } }
#define TRAVERSE_ARRAYLIST_SCOPED(list, name) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); if (item != NULL) { ALLOC_SCOPE(scope, item); name = scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); } } }
#define TRAVERSE_ARRAYLIST_SCOPED_RETALL(list, name) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); if (item != NULL) { ALLOC_SCOPE(scope, item); arraylist_addptr(name, scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope)); } } }
#define TRAVERSE_ARRAYLIST_PRESCOPED(list, name) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); if (item != NULL) { name = scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); } } }

// does not support generic classes... very well... so don't make generic primitives?
struct prog_type* box_primitive(struct prog_state* state, struct prog_type* type, struct ast_node* for_node) {
    if (type->type != PROG_TYPE_PRIMITIVE && type->type != PROG_TYPE_FUNC) {
        return type;
    }
    struct prog_module* langModule = hashmap_get(state->modules, "lang");
    if (langModule == NULL) {
        PROG_ERROR_AST(type, for_node, "cannot box type: lang module not found");
        return NULL;
    }
    char* class_name = NULL;
    if (type->type == PROG_TYPE_PRIMITIVE) {
        switch (type->data.prim.prim_type) {
            case PRIM_U8:
                class_name = "UInt8";
                break;
            case PRIM_I8:
                class_name = "Int8";
                break;
            case PRIM_U16:
                class_name = "UInt16";
                break;
            case PRIM_I16:
                class_name = "Int16";
                break;
            case PRIM_U32:
                class_name = "UInt32";
                break;
            case PRIM_I32:
                class_name = "Int32";
                break;
            case PRIM_U64:
                class_name = "UInt64";
                break;
            case PRIM_I64:
                class_name = "Int64";
                break;
            case PRIM_F:
                class_name = "Float";
                break;
            case PRIM_D:
                class_name = "Double";
                break;
            default: PROG_ERROR_AST(type, for_node, "cannot unbox type: illegal primitive");
                return NULL;
        }
    } else {
        class_name = "Function";
    }
    struct prog_class* clas = hashmap_get(langModule->classes, class_name);
    if (clas == NULL) {
        PROG_ERROR_AST(type, for_node, "cannot unbox type: lang module doesn't implement unboxed type class");
        return NULL;
    }
    return clas->type;
}

int is_boxed_primitive(struct prog_state* state, struct prog_type* type, struct ast_node* for_node) {
    if (type->type == PROG_TYPE_PRIMITIVE) {
        return 1;
    } else if (type != PROG_TYPE_CLASS) {
        return 0;
    }
    struct prog_module* langModule = hashmap_get(state->modules, "lang");
    if (langModule == NULL) {
        PROG_ERROR_AST(type, for_node, "cannot unbox type: lang module not found");
        return NULL;
    }
    if (hashmap_get(langModule->classes, "UInt8") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Int8") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "UInt16") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Int16") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Int32") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "UInt64") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Int64") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Float") == type->data.clas.clas) {
        return 1;
    }
    if (hashmap_get(langModule->classes, "Double") == type->data.clas.clas) {
        return 1;
    }
    return 0;
}

int type_equal(struct prog_type* t1, struct prog_type* t2) {
    if (t1 == t2) return 1;
    if (t1->type != t2->type) return 0;
    if (!(t1->is_ref == t2->is_ref && t1->array_dimensonality == t2->array_dimensonality && t1->variadic != t2->variadic && t1->is_const == t2->is_const)) return 0;
    if (t1->type == PROG_TYPE_UNKNOWN) return 0;
    if (t1->type == PROG_TYPE_PRIMITIVE) return t1->data.prim.prim_type == t2->data.prim.prim_type;
    if (t1->type == PROG_TYPE_FUNC) {
        if (!type_equal(t1->data.func.return_type, t2->data.func.return_type)) return 0;
        if (t1->data.func.arg_types->entry_count != t2->data.func.arg_types->entry_count) return 0;
        for (size_t i = 0; i < t1->data.func.arg_types->entry_count; i++) {
            if (!type_equal(arraylist_getptr(t1->data.func.arg_types, i), arraylist_getptr(t2->data.func.arg_types, i))) return 0;
        }
        return 1;
    }
    if (t1->type == PROG_TYPE_CLASS) {
        return t1->data.clas.clas == t2->data.clas.clas;
    }
    return 0;
}

int class_subtype(struct prog_class* c1, struct prog_class* c2) {
    if (c1 == c2 && c1 != NULL) return 1;
    if (c1 == NULL || c2 == NULL || c2->parents == NULL) return 0;
    if (arraylist_indexptr(c2->parents, c1) != -1) {
        return 1;
    }
    for (size_t i = 0; i < c2->parents->entry_count; i++) {
        if (class_subtype(c1, arraylist_getptr(c2->parents, i))) return 1;
    }
    return 0;
}

int type_subtype(struct prog_type* t1, struct prog_type* t2) {
    if (type_equal(t1, t2)) return 1;
    //TODO: unboxing/boxing?
    if (t1->type != t2->type) return 0;
    if (!(t1->is_ref == t2->is_ref && t1->array_dimensonality == t2->array_dimensonality && t1->variadic != t2->variadic && t1->is_const == t2->is_const)) return 0;
    if (t1->type == PROG_TYPE_UNKNOWN) return 0;
    if (t1->type == PROG_TYPE_PRIMITIVE) return t1->data.prim.prim_type == t2->data.prim.prim_type;
    if (t1->type == PROG_TYPE_FUNC) {
        if (!type_subtype(t1->data.func.return_type, t2->data.func.return_type)) return 0;
        if (t1->data.func.arg_types->entry_count != t2->data.func.arg_types->entry_count) return 0;
        for (size_t i = 0; i < t1->data.func.arg_types->entry_count; i++) {
            if (!type_subtype(arraylist_getptr(t1->data.func.arg_types, i), arraylist_getptr(t2->data.func.arg_types, i))) return 0;
        }
        return 1;
    }
    if (t1->type == PROG_TYPE_CLASS) {
        return class_subtype(t1->data.clas.clas, t2->data.clas.clas);
    }
    return 0;
}

struct prog_type* type_inf_binary_expr(struct prog_state* state, struct ast_node* root, struct prog_type* t1, struct prog_type* t2, uint16_t bin_op) {
    if (bin_op <= BINARY_OP_START_ARITH) {
        return NULL;
    }
    //TODO: this is where we should do autounboxing
    //TODO: this function blatantly memory leaks with no hopes of freeing, but the OS can deal with that.
    //TODO: we should autoconstruct objects here for operator overloading and stuff
    if (t1->type != t2->type || t1->type == PROG_TYPE_UNKNOWN) {
        return NULL;
    }
    if (t1->type == PROG_TYPE_FUNC) {
        if (t1->is_ref || t2->is_ref) {
            PROG_ERROR_AST(t1, root->data.binary.left, "cannot have indirect reference to function");
            return NULL;
        }
        if (bin_op == BINARY_OP_EQ || bin_op == BINARY_OP_EQ_VAL) {
            struct prog_type* ret_type = scalloc(sizeof(struct prog_type));
            ret_type->type = PROG_TYPE_PRIMITIVE;
            ret_type->is_ref = 0;
            ret_type->data.prim.prim_type = PRIM_BOOL;
            ret_type->ast = root;
            ret_type->file = t1->file;
            return ret_type;
        } else if (bin_op == BINARY_OP_ASSN) {
            if (t1->is_const) {
                PROG_ERROR_AST(t1, root->data.binary.left, "expecting mutable value for assignment");
                return NULL;
            }
            if (!type_subtype(t1, t2)) {
                PROG_ERROR_AST(t1, root, "invalid type for assignment");
                return NULL;
            }
            return bin_op == BINARY_OP_ASSN ? t2 : t1; //TODO:  always t2, we need a ASSN_PRE op
        } else {
            PROG_ERROR_AST(t1, root, "illegal operation on function");
            return NULL;
        }
    } else if (t1->type == PROG_TYPE_PRIMITIVE) {
        if (bin_op == BINARY_OP_EQ || bin_op == BINARY_OP_EQ_VAL) {
            struct prog_type* ret_type = scalloc(sizeof(struct prog_type));
            ret_type->type = PROG_TYPE_PRIMITIVE;
            ret_type->is_ref = 0;
            ret_type->data.prim.prim_type = PRIM_BOOL;
            ret_type->ast = root;
            ret_type->file = t1->file;
            return ret_type;
        } else if (bin_op > BINARY_OP_ASSNT) {
            if (t1->is_const) {
                PROG_ERROR_AST(t1, root->data.binary.left, "expecting mutable value for assignment");
                return NULL;
            }
            if (!type_equal(t1, t2)) {
                PROG_ERROR_AST(t1, root, "unequal types for assignment");
                return NULL;
            }
            return bin_op > BINARY_OP_ASSN_PRE ? t1 : t2;
        } else {
            if (!type_equal(t1, t2)) {
                PROG_ERROR_AST(t1, root, "unequal types for expression");
                return NULL;
            }
            return t1;
        }
    } else if (t1->type == PROG_TYPE_CLASS) {
        if (t1->is_ref || t2->is_ref) {
            PROG_ERROR_AST(t1, root->data.binary.left, "cannot have indirect reference to class");
            return NULL;
        }
        if (bin_op == BINARY_OP_EQ || bin_op == BINARY_OP_EQ_VAL) {
            struct prog_type* ret_type = scalloc(sizeof(struct prog_type));
            ret_type->type = PROG_TYPE_PRIMITIVE;
            ret_type->is_ref = 0;
            ret_type->data.prim.prim_type = PRIM_BOOL;
            ret_type->ast = root;
            ret_type->file = t1->file;
            return ret_type;
        } else if (bin_op == BINARY_OP_ASSN) {
            if (t1->is_const) {
                PROG_ERROR_AST(t1, root->data.binary.left, "expecting mutable value for assignment");
                return NULL;
            }
            if (!type_subtype(t1, t2)) {
                PROG_ERROR_AST(t1, root, "invalid type for assignment");
                return NULL;
            }
            // we do not require definitions for these operators, nor will they be called.
            return t2;
        } else {
            if (bin_op > BINARY_OP_ASSN && t1->is_const) {
                PROG_ERROR_AST(t1, root->data.binary.left, "expecting mutable value for assignment");
                return NULL;
            }
            if (!type_subtype(t1, t2)) {
                PROG_ERROR_AST(t1, root, "invalid type for expression");
                return NULL;
            }
            if (!hashmap_get(t1->data.clas.clas->funcs, operator_fns[bin_op]) {
                PROG_ERROR_AST(t1, root, "operator not defined for class");
                return NULL;
            }
            return t1;
        }
    }
}

struct prog_type* duplicate_type(struct prog_type* type) {
    struct prog_type* retType = scalloc(sizeof(struct prog_type));
    memcpy(retType, type, sizeof(struct prog_type));
    if (type->generics != NULL) {
        retType->generics = hashmap_clone(type->generics);
    }
    return retType;
}

struct prog_type* apply_generics(struct hashmap* generics, struct prog_type* type) {
    struct prog_type* mapped_type = hashmap_get(generics, type->name);
    if (mapped_type == NULL && type->generics == NULL) {
        return type;
    }
    if (mapped_type == NULL) {
        mapped_type = type;
    }
    if (type->generics != NULL) {
        struct prog_type* retType = scalloc(sizeof(struct prog_type));
        memcpy(retType, type, sizeof(struct prog_type));
        retType->generics = hashmap_clone(type->generics);
        ITER_MAP(retType->generics) {
            bucket_entry->data = apply_generics(generics, value);
        ITER_MAP_END()}
        mapped_type = retType;
    }
    return mapped_type;
}

struct prog_type* apply_generic_stack(struct prog_scope* stack, struct prog_type* type) {
    if (!type->is_generic && type->generics == NULL) return type;
    while (stack != NULL) {
        if (stack->ast_node->type == AST_NODE_CLASS) {
            type = apply_generics(stack->ast_node->data.class.name->prog->data.type->generics, type);
            return type;
        } else if (stack->ast_node->type == AST_NODE_FUNC) {
            /*
            type = apply_generics(stack->ast_node->data.func.->data.type->generics, type);
            return type;
             //TODO: func generics
             */
        }
        stack = stack->parent;
    }
    return type;
}

// also does type inference
struct prog_type* scope_analysis_expr(struct prog_state* state, struct ast_node* root, struct prog_file* file, struct ast_node* nearest_func, struct prog_module* mod, struct prog_class* clas, struct prog_scope* stack) {
    if (root == NULL) return NULL;
    struct prog_scope* cstack = stack;
    if (root->scope_override) {
        stack = scalloc(sizeof(struct prog_scope));
        stack->copied_ref = 0;
        stack->vars = new_hashmap(8);
        stack->parent = cstack;
        stack->children = arraylist_new(4, sizeof(struct prog_scope*));
        stack->ast_node = root;
        stack->exit_expr_scope = 1;
        arraylist_addptr(cstack->children, stack);
    }
    struct {
        struct prog_file* file;
    } file_cont;
    file_cont.file = file;
    switch (root->type) {
        case AST_NODE_BINARY:;
        struct prog_type* btype1 = NULL;
        struct prog_type* btype2 = NULL;
        if (root->data.binary.op == BINARY_OP_LAND || root->data.binary.op == BINARY_OP_LOR) {
            TRAVERSE_SCOPED(root->data.binary.left, btype1);
            TRAVERSE_SCOPED(root->data.binary.right, btype2);
        } else if (root->data.binary.op == BINARY_OP_MEMBER) {
            struct prog_type* base_expr = TRAVERSE(root->data.binary.left);
            if (base_expr == NULL) return NULL;
            base_expr = box_primitive(state, base_expr, root->data.binary.left);
            if (base_expr == NULL) {
                return NULL;
            }
            if (base_expr->type == PROG_TYPE_UNKNOWN) {
                PROG_ERROR_AST((&file_cont), root->data.binary.left, "type inference failed: type is unknown");
                return NULL;
            }
            // now 100% a class
            struct prog_class* clas = base_expr->data.clas.clas;
            struct prog_var* sub_var = hashmap_get(clas->vars, root->data.binary.right->data.identifier.identifier);
            if (sub_var == NULL) {
                PROG_ERROR_AST((&file_cont), root->data.binary.right, "not a member of parent class");
                return NULL;
            }
            return sub_var->type;
        } else {
            btype1 = TRAVERSE(root->data.binary.left);
            btype2 = TRAVERSE(root->data.binary.right);
        }
        if (btype1 == NULL || btype2 == NULL) {
            return NULL;
        }
        if (root->data.binary.op == BINARY_OP_SEQUENCE) {
            return btype2;
        }
        return type_inf_binary_expr(state, root, btype1, btype2, root->data.binary.op);
        case AST_NODE_BODY:;
        struct prog_type* lt = NULL;
        TRAVERSE_ARRAYLIST_SCOPED(root->data.body.children, lt);
        return lt;
        case AST_NODE_CALC_MEMBER:;
        struct prog_type* ownerType = NULL;
        TRAVERSE_SCOPED(root->data.calc_member.parent, ownerType);
        if (ownerType == NULL) {
            return NULL;
        }
        struct prog_type* lookupType = NULL;
        TRAVERSE_SCOPED(root->data.calc_member.calc, lookupType);
        if (lookupType == NULL) {
            return NULL;
        }
        if (ownerType->array_dimensonality > 0) {
            if (!is_boxed_primitive(state, lookupType, root->data.calc_member.calc)) {
                PROG_ERROR_AST((&file_cont), root->data.calc_member.calc, "array lookups must be primitive");
                return NULL;
            }
            struct prog_type* retType = duplicate_type(ownerType);
            retType->array_dimensonality--;
            return retType;
        } else if (ownerType->type == PROG_TYPE_CLASS) {
            struct prog_func* func = hashmap_get(ownerType->data.clas.clas->funcs, "op_member");
            if (func == NULL) {
                PROG_ERROR_AST((&file_cont), root->data.calc_member.parent, "class does not define op_member function");
                return NULL;
            }
            if (func->arguments_list->entry_count != 1) {
                PROG_ERROR_AST((&file_cont), root->data.calc_member.parent, "definition of op_member does not have 1 argument");
                return NULL;
            }
            struct prog_var* arg = arraylist_getptr(func->arguments_list, 0);
            if (!type_subtype(arg->type, lookupType)) {
                PROG_ERROR_AST((&file_cont), root->data.calc_member.calc, "type does not match or inherit from argument type of op_member function");
                return NULL;
            }
            return func->return_type;
        } else {
            PROG_ERROR_AST((&file_cont), root->data.calc_member.parent, "illegal calculated member access on type");
            return NULL;
        }
        case AST_NODE_CALL:;
        struct prog_type* funcType = NULL;
        TRAVERSE_SCOPED(root->data.call.func, funcType);
        if (funcType == NULL || funcType->type != PROG_TYPE_FUNC) {
            PROG_ERROR_AST((&file_cont), root->data.call.func, "call on non-function is illegal");
            return NULL;
        }
        struct arraylist* paramTypes = arraylist_new(root->data.call.parameters->entry_count, sizeof(struct prog_type*));
        TRAVERSE_ARRAYLIST_SCOPED_RETALL(root->data.call.parameters, paramTypes);
        size_t mi = 0;
        for (size_t i = 0; i < paramTypes->entry_count; i++) {
            if (mi >= funcType->data.func.arg_types->entry_count) {
                PROG_ERROR_AST((&file_cont), root, "call is missing expected parameters"); // TODO: print expected params?
                return NULL;
            }
            struct prog_type* param_type = arraylist_getptr(paramTypes, i);
            struct prog_type* real_type = arraylist_getptr(funcType->data.func.arg_types, mi);
            if (param_type == NULL) {
                PROG_ERROR_AST((&file_cont), ((struct ast_node*)arraylist_getptr(root->data.call.parameters, i)), "illegal parameter type");
                return NULL;
            }
            if (!type_subtype(param_type, real_type)) {
                if (!real_type->is_optional) {
                    PROG_ERROR_AST((&file_cont), ((struct ast_node *) arraylist_getptr(root->data.call.parameters, i)), "illegal parameter type");
                    return NULL;
                } else {
                    mi++;
                    continue;
                }
            }
            if (!real_type->variadic) {
                mi++;
                continue;
            }
        }
        while (mi < funcType->data.func.arg_types->entry_count) {
            struct prog_type* real_type = arraylist_getptr(funcType->data.func.arg_types, mi);
            if (real_type->is_optional || real_type->variadic) {
                mi++;
                continue;
            }
            PROG_ERROR_AST((&file_cont), root, "too many arguments for function call");
            return NULL;
        }
        //TODO: we probably want to save these
        arraylist_free(paramTypes);
        break;
        case AST_NODE_CASE:;
        struct ast_node* parent = stack->parent->ast_node;
        struct prog_type* caseType = TRAVERSE(root->data._case.value);
        if (caseType == NULL || !type_subtype(caseType, parent->data._switch.switch_on->output_type)) {
            PROG_ERROR_AST((&file_cont), root, "invalid case expression value");
            return NULL;
        }
        return TRAVERSE(root->data._case.expr);
        case AST_NODE_CAST:;
        struct prog_type* castTarget = TRAVERSE(root->data.cast.type);
        struct prog_type* castExpr = TRAVERSE(root->data.cast.expr);
        if (castTarget == NULL || castExpr == NULL || (!type_subtype(castTarget, castExpr) && !type_subtype(castExpr, castTarget))) {
            PROG_ERROR_AST((&file_cont), root, "illegal cast");
            return NULL;
        }
        return castTarget;
        case AST_NODE_DEFAULT_CASE:
        return TRAVERSE(root->data.default_case.expr);
        case AST_NODE_FOR: {
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data._for.init);
            TRAVERSE_PRESCOPED(root->data._for.loop);
            TRAVERSE_PRESCOPED(root->data._for.final);
            TRAVERSE_PRESCOPED(root->data._for.expr);
        }
        break;
        case AST_NODE_FOR_EACH: {
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data.for_each.init);
            TRAVERSE_PRESCOPED(root->data.for_each.loop);
            TRAVERSE_PRESCOPED(root->data.for_each.expr);
        }
        break;
        case AST_NODE_FUNC: {
                ALLOC_SCOPE(scope, root);
                struct prog_func *func = root->prog->data.func;
                if (func->name != NULL) {
                    if (hashmap_get(stack->vars, func->name)) {
                        //TODO: type recognition?
                        PROG_ERROR_AST((&file_cont), root, "illegal redeclaration of function");
                    } else {
                        struct prog_var *var = scalloc(sizeof(struct prog_var));
                        var->uid = state->next_var_id++;
                        var->name = func->name;
                        var->func = nearest_func == NULL ? NULL : nearest_func->prog->data.func;
                        var->module = mod;
                        var->clas = clas;
                        var->file = clas->file;
                        var->prot = PROTECTION_PRIV;
                        var->synch = func->synch;
                        var->csig = func->csig;
                        var->cons = 1;
                        var->stat = func->stat;
                        var->type = gen_prog_type(state, func, clas->file, 0, 1, 0);
                        hashmap_put(stack->vars, func->name, var);
                    }
                }
                TRAVERSE(root->data.func.return_type);
                scope->is_param_level = 1;
                scope->ascending_makes_closure = 1;
                TRAVERSE_ARRAYLIST_PRESCOPED(root->data.func.arguments);
                ALLOC_SCOPE_PARENT(scopeBody, scope, root);
                TRAVERSE_PRESCOPED_SPEC(root->data.func.body, scopeBody);
            }
        break;
        case AST_NODE_IF: {
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data._if.condition);
            ALLOC_SCOPE_PARENT(scopeExpr, scope, root);
            TRAVERSE_PRESCOPED_SPEC(root->data._if.expr, scopeExpr);
            if (root->data._if.elseExpr != NULL) {
                ALLOC_SCOPE_PARENT(scopeElseExpr, scope, root);
                TRAVERSE(root->data._if.elseExpr);
            }
        }
        break;
        case AST_NODE_RET:
        TRAVERSE_SCOPED(root->data.ret.expr);
        break;
        case AST_NODE_SWITCH: {
                ALLOC_SCOPE(scope, root);
                TRAVERSE_PRESCOPED(root->data._switch.switch_on);
                TRAVERSE_ARRAYLIST_PRESCOPED(root->data._switch.cases);
            }
        break;
        case AST_NODE_TERNARY: {
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data.ternary.condition);
            ALLOC_SCOPE_PARENT(scopeTrue, scope, root);
            TRAVERSE_PRESCOPED_SPEC(root->data.ternary.if_true, scopeTrue);
            ALLOC_SCOPE_PARENT(scopeFalse, scope, root);
            TRAVERSE_PRESCOPED_SPEC(root->data.ternary.if_false, scopeFalse);
        }
        break;
        case AST_NODE_THROW:
        TRAVERSE_SCOPED(root->data.throw.what);
        break;
        case AST_NODE_TRY: {
            TRAVERSE_SCOPED(root->data.try.expr);
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data.try.catch_var_decl);
            TRAVERSE_PRESCOPED(root->data.try.catch_expr);
            TRAVERSE_SCOPED(root->data.try.finally_expr);
        }
        break;
        case AST_NODE_UNARY:
        TRAVERSE(root->data.unary.child);
        break;
        case AST_NODE_UNARY_POSTFIX:
        TRAVERSE(root->data.unary_postfix.child);
        break;
        case AST_NODE_VAR_DECL:
        TRAVERSE_SCOPED(root->data.vardecl.init);
        TRAVERSE_ARRAYLIST_SCOPED(root->data.vardecl.cons_init);
        if (str_eqCase(root->data.vardecl.name, "this") || hashmap_get(stack->vars, root->data.vardecl.name)) {
            PROG_ERROR_AST((&file_cont), root, "illegal redeclaration of variable");
        } else {
            struct prog_var* var = scalloc(sizeof(struct prog_var));
            var->func = nearest_func == NULL ? NULL : nearest_func->prog->data.func;
            var->file = file;
            var->module = mod;
            var->clas = clas;
            var->name = root->data.vardecl.name;
            var->proc.init = root->data.vardecl.init;
            var->proc.cons_init = root->data.vardecl.cons_init;
            var->prot = PROTECTION_PRIV;
            var->type = root->data.vardecl.type->prog->data.type;
            var->uid = state->next_var_id++;
            hashmap_put(stack->vars, root->data.vardecl.name, var);
        }
        break;
        case AST_NODE_WHILE: {
            ALLOC_SCOPE(scope, root);
            TRAVERSE_PRESCOPED(root->data._while.loop);
            TRAVERSE_PRESCOPED(root->data._while.expr);
        }
        break;
        case AST_NODE_IMP_NEW:
        TRAVERSE_ARRAYLIST(root->data.imp_new.parameters);
        break;
        case AST_NODE_IDENTIFIER:
        if (str_eqCase(root->data.identifier.identifier, "this")) {
            struct prog_node* node = scalloc(sizeof(struct prog_node));
            node->ast_node = root;
            root->prog = node;
            node->prog_type = PROG_NODE_THIS_REF;
            node->data._this = NULL; // TODO? now or later?
            break;
        }
        struct prog_var* ref_var = NULL;
        struct prog_scope* cur_stack = stack;
        uint8_t escaped_func = 0;
        uint8_t cur_class = 0;
        uint8_t cur_param = 0;
        uint8_t in_modules = 0;
        while (cur_stack != NULL) {
            ref_var = hashmap_get(cur_stack->vars, root->data.identifier.identifier);
            if (ref_var == NULL) {
                if (cur_stack->ascending_makes_closure) {
                    escaped_func = 1;
                } else if (cur_stack->is_class_level) {
                    in_modules = 1;
                }
            } else {
                if (cur_stack->is_class_level) {
                    cur_class = 1;
                } else if (cur_stack->is_param_level) {
                    cur_param = 1;
                }
                break;
            }
            cur_stack = cur_stack->parent;
        }
        if (ref_var == NULL) {
            PROG_ERROR_AST((&file_cont), root, "unexpected identifier");
        } else {
            struct prog_node* node = scalloc(sizeof(struct prog_node));
            node->ast_node = root;
            root->prog = node;
            if (cur_class) {
                node->prog_type = PROG_NODE_CLASS_REF;
            } else if (cur_param) {
                node->prog_type = PROG_NODE_PARAM_REF;
            } else if (in_modules) {
                node->prog_type = PROG_NODE_GLOBAL_REF;
            } else if (escaped_func) {
                node->prog_type = PROG_NODE_CAPTURED_REF;
            } else {
                node->prog_type = PROG_NODE_LOCAL_REF;
            }
        }
    }
}

void scope_analysis_func(struct prog_state* state, struct prog_module* mod, struct prog_class* clas, struct prog_func* func, struct prog_scope* stack) {
    for (size_t j = 0; j < func->arguments_list->entry_count; j++) {
        struct prog_var* var = arraylist_getptr(func->arguments_list, j);
        hashmap_put(stack->vars, var->name, var);
        if (var->proc.init != NULL) {
            ALLOC_SCOPE(scope, var->proc.init);
            scope_analysis_expr(state, var->proc.init, func->file, func->proc.root, mod, clas, scope);
        } else if (var->proc.cons_init) {
            for (size_t i = 0; i < var->proc.cons_init; i++) {
                struct ast_node* cons = arraylist_getptr(var->proc.cons_init, i);
                ALLOC_SCOPE(scope, cons);
                scope_analysis_expr(state, var->proc.init, func->file, func->proc.root, mod, clas, scope);
            }
        }
    }
    scope_analysis_expr(state, func->proc.body, func->file, func->proc.root, mod, clas, stack);
}

void scope_analysis_class(struct prog_state* state, struct prog_module* mod, struct prog_class* clas, struct prog_scope* stack) {
    ITER_MAP(clas->funcs) {
        ALLOC_SCOPE(scope, NULL);
        scope_analysis_func(state, mod, clas, value, scope);
    ITER_MAP_END()}
    ITER_MAP(clas->vars) {
        struct prog_var* var = value;
        if (var->proc.init != NULL) {
            ALLOC_SCOPE(scope, var->proc.init);
            scope_analysis_expr(state, var->proc.init, clas->file, NULL, mod, clas, scope);
        } else if (var->proc.cons_init) {
            for (size_t i = 0; i < var->proc.cons_init->entry_count; i++) {
                struct ast_node* cons = arraylist_getptr(var->proc.cons_init, i);
                ALLOC_SCOPE(scope, cons);
                scope_analysis_expr(state, var->proc.init, clas->file, NULL, mod, clas, scope);
            }
        }
    ITER_MAP_END()}
}

struct prog_scope* scope_analysis_mod(struct prog_state* state, struct prog_module* mod, struct prog_scope* stack) {
    if (stack == NULL) {
        stack = scalloc(sizeof(struct prog_scope));
        stack->vars = mod->vars;
        stack->copied_ref = 1;
    }
    ITER_MAP(mod->classes) {
        ALLOC_REF_SCOPE(scope, NULL, ((struct prog_class*)value)->vars);
        scope->is_class_level = 1;
        scope_analysis_class(state, mod, ((struct prog_class*)value), scope);
    ITER_MAP_END()}
    ITER_MAP(mod->funcs) {
        ALLOC_SCOPE(scope, NULL);
        scope_analysis_func(state, mod, NULL, value, scope);
    ITER_MAP_END()}
    ITER_MAP(mod->vars) {
        struct prog_var* var = value;
        if (var->proc.init != NULL) {
            ALLOC_SCOPE(scope, var->proc.init);
            scope_analysis_expr(state, var->proc.init, var->file, NULL, mod, NULL, scope);
        } else if (var->proc.cons_init) {
            for (size_t i = 0; i < var->proc.cons_init; i++) {
                struct ast_node* cons = arraylist_getptr(var->proc.cons_init, i);
                ALLOC_SCOPE(scope, cons);
                scope_analysis_expr(state, var->proc.init, var->file, NULL, mod, NULL, scope);
            }
        }
    ITER_MAP_END()}
    ITER_MAP(mod->submodules) {
        ALLOC_REF_SCOPE(scope, NULL, ((struct prog_module*)value)->vars);
        scope_analysis_mod(state, value, scope);
    ITER_MAP_END()}
    return stack;
}

struct prog_state* gen_prog(struct arraylist* files) {
    struct prog_state* state = scalloc(sizeof(struct prog_state));
    state->modules = new_hashmap(16);
    state->node_map = new_hashmap(128);
    state->errors = arraylist_new(8, sizeof(struct ast_node*));
    for (size_t j = 0; j < files->entry_count; j++) {
        struct ast_node* file = arraylist_getptr(files, j);
        struct prog_file* pfile = scalloc(sizeof(struct prog_file));
        pfile->filename = file->data.file.filename;
        pfile->rel_path = file->data.file.rel_path;
        pfile->lines = file->data.file.lines;
        for (size_t i = 0; i < file->data.file.body->data.body.children->entry_count; i++) {
            struct ast_node* module = arraylist_getptr(file->data.file.body->data.body.children, i);
            gen_prog_module(state, pfile, module, NULL);
        }
    }
    ITER_MAP(state->modules) {
        resolve_module_deps(state, value);
    ITER_MAP_END()}
    ITER_MAP(state->modules) {
        scope_module_types(state, value);
        propagate_mod_types(state, value);
    ITER_MAP_END()}
    ITER_MAP(state->modules) {
        scope_analysis_mod(state, value, NULL);
    ITER_MAP_END()}

    /*
    plan:

    type inference & type expr propagation
    implicit cast insertion?
    generic duplication/mutation
    color all function/(class/global variable) combos: externally mutable, externally accessible, mutable, accessable
    color reference entry/exit to functions
    generate export headers if requested

    context checking/error reporting

    */
    return state;
}