#ifndef LZP_CORE_H
#define LZP_CORE_H

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

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);
typedef void (*lzp_plugin_init_fn)(lenv* env);

extern mpc_parser_t* Number;
extern mpc_parser_t* Float;
extern mpc_parser_t* Symbol;
extern mpc_parser_t* String;
extern mpc_parser_t* Comment;
extern mpc_parser_t* Sexpr;
extern mpc_parser_t* Qexpr;
extern mpc_parser_t* Expr;
extern mpc_parser_t* Lzp;

enum lval_type {
    LVAL_NUM,
    LVAL_FLT,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_STR,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

struct lval {
    enum lval_type type;
    union {
        long num;
        double flt;
        char* err;
        char* sym;
        char* str;
        lbuiltin builtin;
    } data;

    lenv* env;
    lval* formals;
    lval* body;

    int count;
    struct lval** cell;
};

struct lenv {
    lenv* par;
    int count;
    char** syms;
    lval** vals;
};

char* ltype_name(enum lval_type t);

lval* lval_num(long x);
lval* lval_flt(double x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_str(char* s);
lval* lval_builtin(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
void lval_del(lval* v);
lval* lval_copy(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
int lval_eq(lval* x, lval* y);
lval* lval_read(mpc_ast_t* t);
void lval_print(lenv* e, lval* v);
void lval_println(lenv* e, lval* v);

lval* lval_eval_sexpr(lenv* e, lval* v);


lenv* lenv_new(void);


void lenv_del(lenv* e);
lenv* lenv_copy(lenv* e);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_def(lenv* e, lval* k, lval* v);
lval* lenv_get(lenv* e, lval* k);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);
lval* lenv_fetch_symbol(lenv* e, lval* v);

lval* lval_eval(lenv* e, lval* v);
lval* lval_call(lenv* e, lval* f, lval* a);

lval* builtin_eval(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);

#endif