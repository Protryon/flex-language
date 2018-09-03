#include "smem.h"
#include "prog_ir.h"
#include "ast.h"
#include "hash.h"
#include "arraylist.h"
#include "xstring.h"


struct prog_type* gen_prog_type(struct prog_state* state, struct ast_node* node, uint8_t is_master) {
    struct prog_type* t = scalloc(sizeof(struct prog_type));
    t->type = PROG_TYPE_UNKNOWN;
    t->is_master = is_master;
    t->master_type = NULL;
    t->variadic = node->data.type.variadic;
    t->ptr_array = node->data.type.array_pointer_bitlist;
    t->ptr_array_count = node->data.type.array_pointer_count;
    t->name = node->data.type.name;
    if (node->data.type.generics != NULL) {
        t->generics = arraylist_new(node->data.type.generics->entry_count, sizeof(struct prog_type*));
        t->type = PROG_TYPE_CLASS;
    }
    for (size_t i = 0; i < node->data.type.generics->entry_count; i++) {
        struct ast_node* entry = arraylist_getptr(node->data.type.generics, i);
        arraylist_addptr(t->generics, gen_prog_type(state, node, 0));
    }
    if (str_eqCase(t->name, "v")) {
        t->data.prim.prim_type = PRIM_V;
    } else if (str_eqCase(t->name, "void")) {
        t->data.prim.prim_type = PRIM_VOID;
    } else if (str_eqCase(t->name, "u8")) {
        t->data.prim.prim_type = PRIM_U8;
    } else if (str_eqCase(t->name, "b")) {
        t->data.prim.prim_type = PRIM_B;
    } else if (str_eqCase(t->name, "byte")) {
        t->data.prim.prim_type = PRIM_BYTE;
    } else if (str_eqCase(t->name, "c")) {
        t->data.prim.prim_type = PRIM_C;
    } else if (str_eqCase(t->name, "char")) {
        t->data.prim.prim_type = PRIM_CHAR;
    } else if (str_eqCase(t->name, "i8")) {
        t->data.prim.prim_type = PRIM_I8;
    } else if (str_eqCase(t->name, "u16")) {
        t->data.prim.prim_type = PRIM_U16;
    } else if (str_eqCase(t->name, "ush")) {
        t->data.prim.prim_type = PRIM_USH;
    } else if (str_eqCase(t->name, "ushort")) {
        t->data.prim.prim_type = PRIM_USHORT;
    } else if (str_eqCase(t->name, "i16")) {
        t->data.prim.prim_type = PRIM_I16;
    } else if (str_eqCase(t->name, "sh")) {
        t->data.prim.prim_type = PRIM_SH;
    } else if (str_eqCase(t->name, "short")) {
        t->data.prim.prim_type = PRIM_SHORT;
    } else if (str_eqCase(t->name, "u32")) {
        t->data.prim.prim_type = PRIM_U32;
    } else if (str_eqCase(t->name, "u")) {
        t->data.prim.prim_type = PRIM_U;
    } else if (str_eqCase(t->name, "uint")) {
        t->data.prim.prim_type = PRIM_UINT;
    } else if (str_eqCase(t->name, "i32")) {
        t->data.prim.prim_type = PRIM_I32;
    } else if (str_eqCase(t->name, "i")) {
        t->data.prim.prim_type = PRIM_I;
    } else if (str_eqCase(t->name, "int")) {
        t->data.prim.prim_type = PRIM_INT;
    } else if (str_eqCase(t->name, "u64")) {
        t->data.prim.prim_type = PRIM_U64;
    } else if (str_eqCase(t->name, "ul")) {
        t->data.prim.prim_type = PRIM_UL;
    } else if (str_eqCase(t->name, "ulong")) {
        t->data.prim.prim_type = PRIM_ULONG;
    } else if (str_eqCase(t->name, "i64")) {
        t->data.prim.prim_type = PRIM_I64;
    } else if (str_eqCase(t->name, "l")) {
        t->data.prim.prim_type = PRIM_L;
    } else if (str_eqCase(t->name, "long")) {
        t->data.prim.prim_type = PRIM_LONG;
    } else if (str_eqCase(t->name, "f")) {
        t->data.prim.prim_type = PRIM_F;
    } else if (str_eqCase(t->name, "float")) {
        t->data.prim.prim_type = PRIM_FLOAT;
    } else if (str_eqCase(t->name, "d")) {
        t->data.prim.prim_type = PRIM_D;
    } else if (str_eqCase(t->name, "double")) {
        t->data.prim.prim_type = PRIM_DOUBLE;
    }
    if (t->data.prim.prim_type != 0) {
        t->type = PROG_TYPE_PRIMITIVE;
    }
    return t;
}

void gen_prog_clas_func(struct prog_state* state, struct ast_node* func, struct prog_class* parent) {
    struct prog_func* fun = scalloc(sizeof(struct prog_func));
    fun->name = func->data.func.name;
    fun->anonymous = func->data.func.name == NULL;
    fun->clas = parent;
    fun->prot = parent->prot == PROTECTION_NONE || parent->prot >= func->data.func.prot ? func->data.func.prot : parent->prot;
    fun->virt = func->data.func.virt;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->arguments = new_hashmap(4);
    for (size_t i = 0; i < func->data.func.arguments->entry_count; i++) {
        struct ast_node* arg = arraylist_getptr(func->data.func.arguments, i);
        struct prog_var* var = scalloc(sizeof(struct prog_var));
        var->name = arg->data.vardecl.name;
        var->clas = parent;
        var->prot = PROTECTION_PRIV;
        var->type = gen_prog_type(state, arg->data.vardecl.type, 0);
        var->proc.init = arg->data.vardecl.init;
        var->proc.cons_init = arg->data.vardecl.cons_init;
        hashmap_put(fun->arguments, var->name, var);
    }
    fun->return_type = gen_prog_type(state, func->data.func.return_type, 0);
    fun->proc.body = func->data.func.body;
    hashmap_put(parent->funcs, fun->name, fun);
    //hashmap_put(state->extracted_funcs, cl->name, cl->type);
}

void gen_prog_clas_var(struct prog_state* state, struct ast_node* vard, struct prog_class* parent) {
    struct prog_var* var = scalloc(sizeof(struct prog_var));
    var->name = vard->data.vardecl.name;
    var->clas = parent;
    var->prot = parent->prot == PROTECTION_NONE || parent->prot >= vard->data.vardecl.prot ? vard->data.vardecl.prot : parent->prot;
    var->synch = vard->data.vardecl.synch;
    var->csig = vard->data.vardecl.csig;
    var->type = gen_prog_type(state, vard->data.vardecl.type, 0);
    var->proc.init = vard->data.vardecl.init;
    var->proc.cons_init = vard->data.vardecl.cons_init;
    hashmap_put(parent->vars, var->name, var);
}

void gen_prog_class(struct prog_state* state, struct ast_node* clas, struct prog_module* parent) {
    struct prog_class* cl = scalloc(sizeof(struct prog_class));
    cl->name = clas->data.class.name->data.type.name;
    cl->type = gen_prog_type(state, clas->data.class.name, 1);
    cl->type->type = PROG_TYPE_CLASS;
    cl->type->data.clas.clas = cl;
    cl->prot = parent->prot == PROTECTION_NONE || parent->prot >= clas->data.module.prot ? clas->data.module.prot : parent->prot;
    cl->typed = clas->data.class.typed;
    cl->virt = clas->data.class.virt;
    cl->synch = clas->data.class.synch;
    cl->iface = clas->data.class.iface;
    cl->module = parent;
    cl->vars = new_hashmap(4);
    cl->parents = arraylist_new(clas->data.class.parents->entry_count, sizeof(struct ast_node*));
    cl->funcs = new_hashmap(4);
    hashmap_put(parent->classes, cl->name, cl);
    hashmap_put(parent->types, cl->name, cl->type);
    for (size_t i = 0; i < clas->data.class.body->data.body.children->entry_count; i++) {
        struct ast_node* node = arraylist_getptr(clas->data.class.body->data.body.children, i);
        if (node->type == AST_NODE_FUNC) {
            gen_prog_clas_func(state, node, cl);
        } else if (node->type == AST_NODE_VAR_DECL) {
            gen_prog_clas_var(state, node, cl);
        }
    }
    for (size_t i = 0; i < clas->data.class.parents->entry_count; i++) {
        struct ast_node* node = arraylist_getptr(clas->data.class.parents, i);
        arraylist_addptr(cl->parents, gen_prog_type(state, node, 0));
    }
}

void gen_prog_mod_func(struct prog_state* state, struct ast_node* func, struct prog_module* parent) {
    struct prog_func* fun = scalloc(sizeof(struct prog_func));
    fun->name = func->data.func.name;
    fun->anonymous = func->data.func.name == NULL;
    fun->module = parent;
    fun->prot = parent->prot == PROTECTION_NONE || parent->prot >= func->data.func.prot ? func->data.func.prot : parent->prot;
    fun->virt = func->data.func.virt;
    fun->synch = func->data.func.synch;
    fun->csig = func->data.func.csig;
    fun->async = func->data.func.async;
    fun->arguments = new_hashmap(4);
    for (size_t i = 0; i < func->data.func.arguments->entry_count; i++) {
        struct ast_node* arg = arraylist_getptr(func->data.func.arguments, i);
        struct prog_var* var = scalloc(sizeof(struct prog_var));
        var->name = arg->data.vardecl.name;
        var->func = fun;
        var->prot = PROTECTION_PRIV;
        var->type = gen_prog_type(state, arg->data.vardecl.type, 0);
        var->proc.init = arg->data.vardecl.init;
        var->proc.cons_init = arg->data.vardecl.cons_init;
        hashmap_put(fun->arguments, var->name, var);
    }
    fun->return_type = gen_prog_type(state, func->data.func.return_type, 0);
    fun->proc.body = func->data.func.body;
    hashmap_put(parent->funcs, fun->name, fun);
    //hashmap_put(state->extracted_funcs, cl->name, cl->type);
}

void gen_prog_mod_var(struct prog_state* state, struct ast_node* vard, struct prog_module* parent) {
    struct prog_var* var = scalloc(sizeof(struct prog_var));
    var->name = vard->data.vardecl.name;
    var->module = parent;
    var->prot = parent->prot == PROTECTION_NONE || parent->prot >= vard->data.vardecl.prot ? vard->data.vardecl.prot : parent->prot;
    var->synch = vard->data.vardecl.synch;
    var->csig = vard->data.vardecl.csig;
    var->type = gen_prog_type(state, vard->data.vardecl.type, 0);
    var->proc.init = vard->data.vardecl.init;
    var->proc.cons_init = vard->data.vardecl.cons_init;
    hashmap_put(parent->vars, var->name, var);
}

void gen_prog_module(struct prog_state* state, char* file, struct ast_node* module, struct prog_module* parent) {
    struct prog_module* mod = scalloc(sizeof(struct prog_module));
    mod->name = module->data.module.name;
    mod->prot = module->data.module.prot;
    mod->filename = file;
    mod->submodules = new_hashmap(4);
    mod->vars = new_hashmap(4);
    mod->classes = new_hashmap(4);
    mod->funcs = new_hashmap(4);
    mod->parent = parent;
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
            gen_prog_class(state, node, mod);
        } else if (node->type == AST_NODE_FUNC) {
            gen_prog_mod_func(state, node, mod);
        } else if (node->type == AST_NODE_VAR_DECL) {
            gen_prog_mod_var(state, node, mod);
        }
    }
}

struct prog_state* gen_prog(struct arraylist* files) {
    struct prog_state* state = scalloc(sizeof(struct prog_state));
    state->modules = new_hashmap(16);
    for (size_t j = 0; j < files->entry_count; j++) {
        struct ast_node* file = arraylist_getptr(files->entry_count, j);
        for (size_t i = 0; i < file->data.file.body->data.body.children->entry_count; i++) {
            struct ast_node* module = arraylist_getptr(file->data.file.body->data.body.children, i);
            gen_prog_module(state, file->data.file.rel_path, module, NULL);
        }
    }
    /*
    plan:
    include resolution
    high-level type propagation (for us and external includes)
    function-internal type propagation, variable tagging
    generic duplication/mutation
    color all function/(class/global variable) combos: externally mutable, externally accessible, mutable, accessable
    color reference entry/exit to functions
    generate export headers if requesested

    context checking/error reporting

    */
    return state;
}