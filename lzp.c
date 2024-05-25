#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mpc.h"

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

#ifdef _WIN32
#include <string.h>

static char buffer[2028];

char* readline(char* promt) {
    fputs(promt, stdout);
    fgets(buffer, 20248, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


typedef lval*(*lbuiltin)(lenv*, lval*);

enum lval_type {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

char* ltype_name(enum lval_type t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

typedef struct lval {
    enum lval_type type;
    union {
        long num;
        char* err;
        char* sym;
        lbuiltin fun;
    } data;

    int count;
    struct lval** cell;
} lval;

lval* lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->data.num = x;
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

lval* lval_fun(lbuiltin func) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->data.fun = func;
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
        case LVAL_FUN: break;
            case LVAL_ERR:
            free(v->data.err); break;
        case LVAL_SYM:
            free(v->data.sym); break;
        
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
            x->data.fun = v->data.fun; break;
        case LVAL_NUM:
            x->data.num = v->data.num; break;

        case LVAL_ERR:
            x->data.err = malloc(strlen(v->data.err) + 1);
            strcpy(x->data.err, v->data.err);
            break;
        case LVAL_SYM:
            x->data.sym = malloc(strlen(v->data.sym) + 1);
            strcpy(x->data.sym, v->data.sym);
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
    lval *x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
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

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    if (errno == ERANGE) {
        return lval_err("invalid number");
    }
    return lval_num(x);
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return lval_read_num(t);
    }
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
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
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_print(lenv* e, lval *v);

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

lval* lenv_fetch_symbol(lenv* e, lval* v);

void lval_print(lenv* e, lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->data.num); break;
        case LVAL_ERR:
            printf("Error: %s", v->data.err); break;
        case LVAL_SYM:
            printf("%s", v->data.sym); break;
        case LVAL_FUN:
            lval* x = lenv_fetch_symbol(e, v);
            printf("<%s>", x->data.sym); break;
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

struct lenv{
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

lenv* lenv_del(lenv* e) {
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

    return lval_err("unbound symbol '%s'", k->data.sym);
}

lval* lenv_fetch_symbol(lenv* e, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (e->vals[i]->data.fun == v->data.fun) {
            return lval_sym(e->syms[i]);
        }
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

lval* builtin_op(lenv* e, lval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    lval *x = lval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->data.num = -x->data.num;
    }

    while (a->count > 0) {
        lval *y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) {
            x->data.num += y->data.num;
        }
        if (strcmp(op, "-") == 0) {
            x->data.num -= y->data.num;
        }
        if (strcmp(op, "*") == 0) {
            x->data.num *= y->data.num;
        }
        if (strcmp(op, "/") == 0) {
            if(y->data.num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero!");
                break;
            }
            x->data.num /= y->data.num;
        }
        if (strcmp(op, "%") == 0) {
            if(y->data.num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero!");
                break;
            }
            x->data.num %= y->data.num;
        }
        if (strcmp(op, "**") == 0) {
            x->data.num = powl(x->data.num, y->data.num);
        }
        if (strcmp(op, "min") == 0) {
            if (x->data.num > y->data.num) {
                x->data.num = y->data.num;
            }
        }
        if (strcmp(op, "max") == 0) {
            if (x->data.num < y->data.num) {
                x->data.num = y->data.num;
            }
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval *lval_eval(lenv* e, lval *v);

lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);
    while (v->count > 1) {
        lval_del(lval_pop(v, 1));
    }
    return v;
}

lval* builtin_tail(lenv* e, lval *a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval *v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e,lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval *x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval* builtin_len(lenv* e,lval* a) {
    LASSERT_NUM("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

    lval* x = lval_num(a->cell[0]->count);
    lval_del(a);
    return x;
}

lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
}

lval* builtin_mod(lenv* e, lval* a) {
    return builtin_op(e, a, "%");
}

lval* builtin_pow(lenv* e, lval* a) {
    return builtin_op(e, a, "**");
}

lval* builtin_min(lenv* e, lval* a) {
    return builtin_op(e, a, "min");
}

lval* builtin_max(lenv* e, lval* a) {
    return builtin_op(e, a, "max");
}

lval* builtin_def(lenv* e, lval* a) {
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];

    for (int i = 0; i < syms->count; i++) {
        LASSERT_TYPE("def", syms, i, LVAL_SYM);
    }

    LASSERT_NUM("def", a, syms->count + 1);


    for (int i = 0; i < syms->count;i++) {
        lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    lval_del(a);
    return lval_sexpr();
}

lval* builtin_exit(lenv* e, lval* a) {
    lval_del(a);
    exit(0);
}

lval* builtin_state(lenv* e, lval* a) {
    for (int i = 0; i < e->count; i++) {
        printf("%s = ", e->syms[i]);
        lval_print(e, e->vals[i]);
        printf("\n");
    }
    lval_del(a);
    return lval_sexpr();
}


void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "len", builtin_len);
    
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "**", builtin_pow);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);

    lenv_add_builtin(e, "def", builtin_def);

    lenv_add_builtin(e, "exit", builtin_exit);
    lenv_add_builtin(e, "state", builtin_state);
}

lval *lval_eval_sexpr(lenv* e, lval* v);

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
        lval_del(f);
        lval_del(v);
        return lval_err("first element is not a function");
    }

    lval *result = f->data.fun(e, v);
    lval_del(f);
    return result;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lzp = mpc_new("lzp");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                       \
        number: /-?[0-9]+/ ;                                \
        symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>%!&]+/ ;         \
        sexpr:  '(' <expr>* ')' ;                           \
        qexpr:  '{' <expr>* '}' ;                           \
        expr:   <number> | <symbol> | <sexpr> | <qexpr> ;   \
        lzp:    /^/ <expr>* /$/  ;                          \
    ",  Number, Symbol, Sexpr, Qexpr, Expr, Lzp);

    puts("Lzp Version 0.0.0.0.7");
    puts("Press Ctrl+c to Exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    while(1) {
        char* input = readline("lzp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lzp, &r)) {
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(e, x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    lenv_del(e);

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lzp);
    return 0;
}
