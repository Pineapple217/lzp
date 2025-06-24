#include "lzp_core.h"
#include "mpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <math.h>

char* ltype_name(enum lval_type t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_FLT: return "Float";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_STR: return "String";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

// LVAL

lval* lval_num(long long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->data.num = x;
    return v;
}

lval* lval_flt(double x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FLT;
    v->data.flt = x;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->data.err = malloc(512);

    vsnprintf(v->data.err, 511, fmt, va);
    v->data.err = realloc(v->data.err, strlen(v->data.err) + 1);
    va_end(va);
    return v;
}

lval* lval_sym(char* s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->data.sym = malloc(strlen(s) + 1);
    strcpy(v->data.sym, s);
    return v;
}

lval* lval_str(char* s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->data.str = malloc(strlen(s) + 1);
    strcpy(v->data.str, s);
    return v;
}

lval* lval_builtin(lbuiltin func) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->data.builtin = func;
    return v;
}

lval* lval_lambda(lval* formals, lval* body) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    v->data.builtin = NULL;
    v->env = lenv_new();

    v->formals = formals;
    v->body = body;
    return v;
}

lval* lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_FLT: break;
        case LVAL_FUN:
            if (!v->data.builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_ERR:
            free(v->data.err); break;
        case LVAL_SYM:
            free(v->data.sym); break;
        case LVAL_STR:
            free(v->data.str); break;
        
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }

            free(v->cell);
            break;
    }
    free(v);
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_FUN:
            if (v->data.builtin) {
                x->data.builtin = v->data.builtin;
            } else {
                x->data.builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM:
            x->data.num = v->data.num; break;
        case LVAL_FLT:
            x->data.flt = v->data.flt; break;

        case LVAL_ERR:
            x->data.err = malloc(strlen(v->data.err) + 1);
            strcpy(x->data.err, v->data.err);
            break;
        case LVAL_SYM:
            x->data.sym = malloc(strlen(v->data.sym) + 1);
            strcpy(x->data.sym, v->data.sym);
            break;
        case LVAL_STR:
            x->data.str = malloc(strlen(v->data.str) + 1);
            strcpy(x->data.str, v->data.str);
            break;

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval *) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_join(lval* x, lval* y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

int lval_eq(lval* x, lval* y) {
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case LVAL_NUM:
            return (x->data.num == y->data.num);
        case LVAL_FLT:
            return (fabs(x->data.flt - y->data.flt) < 0.000000000000001);
        case LVAL_ERR:
            return (strcmp(x->data.err, y->data.err) == 0);
        case LVAL_SYM:
            return (strcmp(x->data.sym, y->data.sym) == 0);
        case LVAL_STR:
            return (strcmp(x->data.str, y->data.str) == 0);

        case LVAL_FUN:
            if (x->data.builtin || y->data.builtin) {
                return x->data.builtin == y->data.builtin;
            } 
            return lval_eq(x->formals, y->formals)
                && lval_eq(x->body, y->body);

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) {
                return 0;
            }

            for (int i = 0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) {
                    return 0;
                }
            }
            return 1;
        break;
    }
    return 0;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long long x = strtoll(t->contents, NULL, 10);
    if (errno == ERANGE) {
        return lval_err("invalid number");
    }
    return lval_num(x);
}

lval* lval_read_flt(mpc_ast_t* t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    if (errno == ERANGE) {
        return lval_err("invalid float");
    }
    return lval_flt(x);
}

lval* lval_read_str(mpc_ast_t* t) {
    t->contents[strlen(t->contents) - 1] = '\0';

    char* unescaped = malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);

    unescaped = mpcf_unescape(unescaped);

    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return lval_read_num(t);
    }
    if (strstr(t->tag, "float")) {
        return lval_read_flt(t);
    }
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
    }
    if (strstr(t->tag, "string")) {
        return lval_read_str(t);
    }

    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "sexpr")) {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "qexpr")) {
        x = lval_qexpr();
    }

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_expr_print(lenv* e, lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(e, v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval* v) {
    char* esc = malloc(strlen(v->data.str) + 1);
    strcpy(esc, v->data.str);
    esc = mpcf_escape(esc);
    printf("\"%s\"", esc);
    free(esc);
}

void lval_print(lenv* e, lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%lli", v->data.num); break;
        case LVAL_FLT:
            printf("%.15g", v->data.flt); break;
        case LVAL_ERR:
            printf("Error: %s", v->data.err); break;
        case LVAL_SYM:
            printf("%s", v->data.sym); break;
        case LVAL_STR:
            lval_print_str(v); break;
        case LVAL_FUN:
            if (v->data.builtin) {
                lval* x = lenv_fetch_symbol(e, v);
                printf("<%s>", x->data.sym);
            } else {
                printf("(\\ ");
                lval_print(e, v->formals);
                putchar(' ');
                lval_print(e, v->body);
                putchar(')');
            }
            break;
        case LVAL_SEXPR:
            lval_expr_print(e, v, '(', ')'); break;
        case LVAL_QEXPR:
            lval_expr_print(e, v, '{', '}'); break;
    }
}

void lval_println(lenv* e, lval* v) {
    lval_print(e, v);
    putchar('\n');
}

// LENV

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->data.sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->par) {
        return lenv_get(e->par, k);
    } 

    return lval_err("unbound symbol '%s'", k->data.sym);
}

lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

void lenv_def(lenv* e, lval* k, lval* v) {
    while (e->par) {
        e = e->par;
    }
    lenv_put(e, k, v);
}

lval* lenv_fetch_symbol(lenv* e, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (e->vals[i]->data.builtin == v->data.builtin) {
            return lval_sym(e->syms[i]);
        }
    }

    if (e->par) {
        return lenv_fetch_symbol(e->par, v);
    } 

    return lval_err("functions symbol not found");
}

void lenv_put(lenv* e, lval* k, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->data.sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->data.sym) + 1);
    strcpy(e->syms[e->count - 1], k->data.sym);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_builtin(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }
    return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    if (v->count == 0) {
        return v;
    }

    if (v->count == 1) {
        return lval_take(v, 0);
    }

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN)
        );
        lval_del(f); lval_del(v);
        return err;
    }

    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->data.builtin) {
        return f->data.builtin(e, a);
    }

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err(
                "Function passed too many arguments. "
                "Got %i, expected %i", given, total
            );
        }

        lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->data.sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. "
                    "Symbol '&' not followed by single symbol.");
            }

            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);

        lenv_put(f->env, sym, val);

        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);

    if (f->formals->count > 0 &&
        strcmp(f->formals->cell[0]->data.sym, "&") == 0) {
    
        if (f->formals->count != 2) {
        return lval_err("Function format invalid. "
            "Symbol '&' not followed by single symbol.");
        }
        
        lval_del(lval_pop(f->formals, 0));
        
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();
        
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->par = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }

    return lval_copy(f);
}

// buildin

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}