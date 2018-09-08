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

struct prog_type* gen_prog_type(struct ast_node* node, struct prog_file* file, uint8_t is_master, uint8_t is_generic) {
    if (node == NULL) return NULL;
    struct prog_type* t = scalloc(sizeof(struct prog_type));
    t->type = PROG_TYPE_UNKNOWN;
    t->is_master = is_master;
    t->file = file;
    t->master_type = NULL;
    t->ast = node;
    if (node->type == AST_NODE_TYPE) {
        if (node->data.type.protofunc) {
            t->type = PROG_TYPE_FUNC;
            t->data.func.return_type = gen_prog_type(node->data.type.protofunc_return_type, file, 0, 0);
            t->data.func.arg_types = node->data.type.protofunc_arguments == NULL ? NULL : arraylist_new(node->data.type.protofunc_arguments->entry_count, sizeof(struct prog_type*));
            if (t->data.func.arg_types != NULL) {
                for (size_t i = 0; i < node->data.type.protofunc_arguments->entry_count; i++) {
                    arraylist_addptr(t->data.func.arg_types, gen_prog_type(arraylist_getptr(node->data.type.protofunc_arguments, i), file, 0, 0));
                }
            }
        } else {
            t->variadic = node->data.type.variadic;
            t->ptr_array = node->data.type.array_pointer_bitlist;
            t->ptr_array_count = node->data.type.array_pointer_count;
            t->name = node->data.type.name;
            if (node->data.type.generics != NULL) {
                t->generics = new_hashmap(4);
                t->type = PROG_TYPE_CLASS;
                if (!is_master || !is_generic)
                    for (size_t i = 0; i < node->data.type.generics->entry_count; i++) {
                        struct ast_node *entry = arraylist_getptr(node->data.type.generics, i);
                        hashmap_put(t->generics, entry->data.type.name, gen_prog_type(entry, file, is_master, 1));
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

    } else if (str_eqCase(t->name, "v")) {
        prim_type = PRIM_V;
    } else if (str_eqCase(t->name, "void")) {
        prim_type = PRIM_VOID;
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
    fun->arguments = new_hashmap(4);
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
        var->type = gen_prog_type(arg->data.vardecl.type, file, 0, 0);
        var->proc.init = arg->data.vardecl.init;
        if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
        var->proc.cons_init = arg->data.vardecl.cons_init;
        if (var->proc.cons_init != NULL) {
            for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
            }
        }
        hashmap_put(fun->arguments, var->name, var);
    }
    fun->return_type = gen_prog_type(func->data.func.return_type, file, 0, 0);
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
        node->prog->data.type = gen_prog_type(node, ctx->file, 0, 1);
        struct hashmap* om = NULL;
        if (ctx->func != NULL) {
            om = ctx->func->node_map;
        } else if (ctx->clas != NULL) {
            om = ctx->clas->node_map;
        } else if (ctx->module != NULL) {
            om = ctx->module->node_map;
        }
        if (om != NULL) hashmap_putptr(om, node, node->prog);
        hashmap_putptr(ctx->state->node_map, node, node->prog);
    } else if (node->type == AST_NODE_FUNC) {
        node->prog = scalloc(sizeof(struct prog_node));
        node->prog->ast_node = node;
        node->prog->prog_type = PROG_NODE_EXTRACTED_FUNC_REF;
        node->prog->data.func = gen_prog_type(node, ctx->file, 0, 1);
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
    fun->virt = func->data.func.virt;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->arguments = new_hashmap(4);
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
            var->type = gen_prog_type(arg->data.vardecl.type, fun->file, 0, 0);
            var->proc.init = arg->data.vardecl.init;
            if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
            var->proc.cons_init = arg->data.vardecl.cons_init;
            if (var->proc.cons_init != NULL) {
                for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                    if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
                }
            }
            hashmap_put(fun->arguments, var->name, var);
        }
    fun->return_type = gen_prog_type(func->data.func.return_type, fun->file, 0, 0);
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
        var->is_const = 1;
        var->type = gen_prog_type(func, fun->file, 0, 0);
        struct preprocess_ctx lctx = (struct preprocess_ctx) {state, NULL, parent, NULL, file};
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
    var->type = gen_prog_type(vard->data.vardecl.type, file, 0, 0);
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
    cl->type = gen_prog_type(clas->data.class.name, file, 1, 0);
    cl->file = file;
    cl->type->type = PROG_TYPE_CLASS;
    cl->type->data.clas.clas = cl;
    cl->prot = parent->prot == PROTECTION_NONE || parent->prot >= clas->data.module.prot ? clas->data.module.prot : parent->prot;
    cl->typed = clas->data.class.typed;
    cl->virt = clas->data.class.virt;
    cl->synch = clas->data.class.synch;
    cl->iface = clas->data.class.iface;
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
            arraylist_addptr(cl->parents, gen_prog_type(node, file, 0, 0));
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
    fun->arguments = new_hashmap(4);
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
            var->type = gen_prog_type(arg->data.vardecl.type, file, 0, 0);
            var->proc.init = arg->data.vardecl.init;
            if (var->proc.init != NULL) traverse_node(var->proc.init, preprocess_expr, &lctx, 1);
            var->proc.cons_init = arg->data.vardecl.cons_init;
            if (var->proc.cons_init != NULL) {
                for (size_t j = 0; j < var->proc.cons_init->entry_count; j++) {
                    if (var->proc.init != NULL) traverse_node(arraylist_getptr(var->proc.cons_init, j), preprocess_expr, &lctx, 1);
                }
            }
            hashmap_put(fun->arguments, var->name, var);
        }
    fun->return_type = gen_prog_type(func->data.func.return_type, file, 0, 0);
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
        var->is_const = 1;
        var->type = gen_prog_type(func, fun->file, 0, 0);
        struct preprocess_ctx lctx = (struct preprocess_ctx) {state, parent, NULL, NULL, file};
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
    var->type = gen_prog_type(vard->data.vardecl.type, file, 0, 0);
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
    if (parent == NULL) {
        mod = hashmap_get(state->modules, module->data.module.name);
    } else {
        mod = hashmap_get(parent->submodules, module->data.module.name);
    }
    struct {
        struct prog_file* file;
    } file_cont;
    file_cont.file = file;
    if (mod == NULL) {
        mod = scalloc(sizeof(struct prog_module));
        mod->name = module->data.module.name;
        mod->prot = module->data.module.prot;
        mod->file = file;
        mod->submodules = new_hashmap(4);
        mod->vars = new_hashmap(4);
        mod->node_map = new_hashmap(4);
        mod->classes = new_hashmap(4);
        mod->funcs = new_hashmap(4);
        mod->types = new_hashmap(16);
        mod->imported_modules = arraylist_new(4, sizeof(struct ast_node*));
        mod->parent = parent;
    } else {
        if (mod->prot != module->data.module.prot) {
            PROG_ERROR_AST((&file_cont), module, "conflicting module properties");
        }
    }
    if (parent == NULL) {
        hashmap_put(state->modules, mod->name, mod);
    } else {
        hashmap_put(parent->submodules, mod->name, mod);
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
            ITER_MAP(func->arguments) {
                provide_master_types(state, mod, func->file, NULL, func, value, 0);
            ITER_MAP_END()}
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
        ITER_MAP(func->arguments) {
            provide_master_types(state, mod, func->file, NULL, func, value, 0);
        ITER_MAP_END()}
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
#define TRAVERSE_SCOPED(item) if (item != NULL) { ALLOC_SCOPE(scope, item); scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); }
#define TRAVERSE_PRESCOPED_SPEC(item, scope) if (item != NULL) { scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); }
#define TRAVERSE_PRESCOPED(item) TRAVERSE_PRESCOPED_SPEC(item, scope)
#define TRAVERSE_ARRAYLIST(list) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, stack); } }
#define TRAVERSE_ARRAYLIST_SCOPED(list) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); if (item != NULL) { ALLOC_SCOPE(scope, item); scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); } } }
#define TRAVERSE_ARRAYLIST_PRESCOPED(list) if (list != NULL) { for (size_t i = 0; i < list->entry_count; i++) { struct ast_node* item = arraylist_getptr(list, i); if (item != NULL) { scope_analysis_expr(state, item, file, TRAVERSE_NEW_FUNC(item), mod, clas, scope); } } }

void scope_analysis_expr(struct prog_state* state, struct ast_node* root, struct prog_file* file, struct ast_node* nearest_func, struct prog_module* mod, struct prog_class* clas, struct prog_scope* stack) {
    if (root == NULL) return;
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
        case AST_NODE_BINARY:
        if (root->data.binary.op == BINARY_OP_LAND || root->data.binary.op == BINARY_OP_LOR) {
            TRAVERSE_SCOPED(root->data.binary.left);
            TRAVERSE_SCOPED(root->data.binary.right);
        } else {
            TRAVERSE(root->data.binary.left);
            if(root->data.binary.op != BINARY_OP_MEMBER)
                TRAVERSE(root->data.binary.right);
        }
        break;
        case AST_NODE_BODY:
        TRAVERSE_ARRAYLIST_SCOPED(root->data.body.children);
        break;
        case AST_NODE_CALC_MEMBER:
        TRAVERSE_SCOPED(root->data.calc_member.parent);
        TRAVERSE_SCOPED(root->data.calc_member.calc);
        break;
        case AST_NODE_CALL:
        TRAVERSE_SCOPED(root->data.call.func);
        TRAVERSE_ARRAYLIST_SCOPED(root->data.call.parameters);
        break;
        case AST_NODE_CASE:
        TRAVERSE(root->data._case.value);
        TRAVERSE(root->data._case.expr);
        break;
        case AST_NODE_CAST:
        TRAVERSE(root->data.cast.type);
        TRAVERSE(root->data.cast.expr);
        break;
        case AST_NODE_DEFAULT_CASE:
        TRAVERSE(root->data.default_case.expr);
        break;
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
        case AST_NODE_FUNC:;
            {
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
                        var->is_const = 1;
                        var->type = gen_prog_type(func, clas->file, 0, 0);
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
    ITER_MAP(func->arguments) {
        struct prog_var* var = value;
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
    ITER_MAP_END()}
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
        scope_analysis_class(state, mod, value, scope);
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
    error reporting for scope analysis
    test scope analysis
    type inference & type expr propagation
    implcit cast insertion?
    generic duplication/mutation
    color all function/(class/global variable) combos: externally mutable, externally accessible, mutable, accessable
    color reference entry/exit to functions
    generate export headers if requested

    context checking/error reporting

    */
    return state;
}