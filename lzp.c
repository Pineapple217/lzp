#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2028];

enum lval_type {
    LVAL_NUM,
    LVAL_ERR
};

enum lval_err_type {
    LERR_DEV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
};

typedef struct {
    enum lval_type type;
    union {
        long num;
        enum lval_err_type err;
    } data;
} lval;


lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.data.num = x;
    return v;
}

lval lval_err(enum lval_err_type x) {
    lval v;
    v.type = LVAL_ERR;
    v.data.err = x;
    return v;
}

void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%li", v.data.num); break;
        
        case LVAL_ERR:
            if (v.data.err == LERR_DEV_ZERO) {
                printf("Error: Division By Zero!");
            } else 
            if  (v.data.err == LERR_BAD_OP) {
                printf("Error: Invalid Operator!");
            } else
            if (v.data.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number!");
            }
        break;
    }
}

void Lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

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

lval eval_op(lval x, char* op, lval y) {

    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if (strcmp(op, "+") == 0) { return lval_num(x.data.num + y.data.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.data.num - y.data.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.data.num * y.data.num); }
    if (strcmp(op, "**") == 0) { return lval_num(powl(x.data.num, y.data.num)); }
    if (strcmp(op, "%") == 0) { 
        if (y.data.num == 0) {
            return lval_err(LERR_DEV_ZERO);
        }
        return lval_num(x.data.num % y.data.num); 
    }
    if (strcmp(op, "/") == 0) { 
        if (y.data.num == 0) {
            return lval_err(LERR_DEV_ZERO);
        }
        return lval_num(x.data.num / y.data.num);
    }
    if (strcmp(op, "min") == 0) {
        if (x.data.num > y.data.num) { return y; }
        return x;
    }
    if (strcmp(op, "max") == 0) {
        if (x.data.num > y.data.num) { return x; }
        return y;
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        if (errno == ERANGE) {
            return lval_err(LERR_BAD_NUM);
        }
        return lval_num(x);
    }
    char* op = t->children[1]->contents;

    lval x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t-> children[i]));
        i++;
    }

    if(strcmp(op, "-") == 0 && i == 3) { return lval_num(-x.data.num); }
    return x;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t *Lzp = mpc_new("lzp");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                             \
        number:     /-?[0-9]+/ ;                                                  \
        operator:   \"min\" | \"max\" | \"**\" | '+' | '-' | '*' | '/' | '%' ;    \
        expr:       <number> | '(' <operator> <expr>+ ')' ;                       \
        lzp:        /^/ <operator> <expr>+ /$/  ;                                 \
    ",  Number, Operator, Expr, Lzp);

    puts("Lispy Version 0.0.0.0.4");
    puts("Press Ctrl+c to Exit\n");

    while(1) {
        char* input = readline("lzp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lzp, &r)) {
            lval result = eval(r.output);
            Lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lzp);
    return 0;
}

