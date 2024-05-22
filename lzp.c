#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mpc.h"

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

enum lval_type {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR
};

typedef struct lval {
    enum lval_type type;
    union {
        long num;
        char* err;
        char* sym;
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

lval* lval_err(char* m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->data.err = malloc(strlen(m) + 1);
    strcpy(v->data.err, m);
    return v;
}

lval* lval_sym(char* s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->data.sym = malloc(strlen(s) + 1);
    strcpy(v->data.sym, s);
    return v;
}

lval* lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR:
            free(v->data.err); break;
        case LVAL_SYM:
            free(v->data.sym); break;
        
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }

            free(v->cell);
            break;
    }
    free(v);
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

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_print(lval *v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->data.num); break;
        case LVAL_ERR:
            printf("Error: %s", v->data.err); break;
        case LVAL_SYM:
            printf("%s", v->data.sym); break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')'); break;
    }
}

void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

lval* builtin_op(lval* a, char* op) {
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

lval *lval_eval_sexpr(lval *v);

lval* lval_eval(lval* v) {
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(v);
    }
    return v;
}

lval* lval_eval_sexpr(lval* v) {
    if (v->count == 0) {
        return v;
    }

    if (v->count == 1) {
        return lval_take(v, 0);
    }

    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression does not start with symbol!");
    }

    lval *result = builtin_op(v, f->data.sym);
    lval_del(f);
    return result;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lzp = mpc_new("lzp");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                         \
        number: /-?[0-9]+/ ;                                                  \
        symbol: \"min\" | \"max\" | \"**\" | '+' | '-' | '*' | '/' | '%' ;    \
        sexpr:  '(' <expr>* ')' ;                                             \
        expr:   <number> | <symbol> | <sexpr> ;                               \
        lzp:    /^/ <expr>* /$/  ;                                            \
    ",  Number, Symbol, Sexpr, Expr, Lzp);

    puts("Lispy Version 0.0.0.0.5");
    puts("Press Ctrl+c to Exit\n");

    while(1) {
        char* input = readline("lzp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lzp, &r)) {
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lzp);
    return 0;
}
