#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <math.h>

#include "lzp_core.h"
#include "mpc.h"
#include "prelude.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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

lval* builtin_op(lenv* e, lval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM && a->cell[i]->type != LVAL_FLT) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }

    }

    lval *x = lval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        if (x->type == LVAL_NUM) {
            x->data.num = -x->data.num;
        }
        else if (x->type == LVAL_FLT) {
            x->data.flt = -x->data.flt;
        }
    }

    while (a->count > 0) {
        lval *y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                x->data.num += y->data.num;
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                x->data.flt += y->data.flt;
            } else if (x->type == LVAL_FLT) {
                x->data.flt += (double)y->data.num;
            } else {
                x->type = LVAL_FLT;
                x->data.flt = (double)x->data.num + y->data.flt;
            }
        }
        if (strcmp(op, "-") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                x->data.num -= y->data.num;
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                x->data.flt -= y->data.flt;
            } else if (x->type == LVAL_FLT) {
                x->data.flt -= (double)y->data.num;
            } else {
                x->type = LVAL_FLT;
                x->data.flt = (double)x->data.num - y->data.flt;
            }
        }
        if (strcmp(op, "*") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                x->data.num *= y->data.num;
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                x->data.flt *= y->data.flt;
            } else if (x->type == LVAL_FLT) {
                x->data.flt *= (double)y->data.num;
            } else {
                x->type = LVAL_FLT;
                x->data.flt = (double)x->data.num * y->data.flt;
            }
        }
        if (strcmp(op, "/") == 0) {
            if ((y->type == LVAL_NUM && y->data.num == 0)
                || (y->type == LVAL_FLT && y->data.flt == 0)) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero!");
                break;
            }
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                x->data.num /= y->data.num;
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                x->data.flt /= y->data.flt;
            } else if (x->type == LVAL_FLT) {
                x->data.flt /= (double)y->data.num;
            } else {
                x->type = LVAL_FLT;
                x->data.flt = (double)x->data.num / y->data.flt;
            }
        }
        if (strcmp(op, "%") == 0) {
            if (x->type == LVAL_FLT || y->type == LVAL_FLT) {
                return lval_err("Cannot operate on floats!");
            }
            if(y->data.num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero!");
                break;
            }
            x->data.num %= y->data.num;
        }
        if (strcmp(op, "**") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                x->data.num = powl(x->data.num, y->data.num);
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                x->data.flt = pow(x->data.flt, y->data.flt);
            } else if (x->type == LVAL_FLT) {
                x->data.flt = pow(x->data.flt, (double)y->data.num);
            } else {
                x->type = LVAL_FLT;
                x->data.flt = pow((double)x->data.num, y->data.flt);
            }
        }
        if (strcmp(op, "min") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                if (x->data.num > y->data.num) {
                    x->data.num = y->data.num;
                }
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                if (x->data.flt > y->data.flt) {
                    x->data.flt = y->data.flt;
                }
            } else if (x->type == LVAL_FLT) {
                if (x->data.flt > y->data.num) {
                    x->type = LVAL_NUM;
                    x->data.num = y->data.num;
                }
            } else {
                if (x->data.num > y->data.flt) {
                    x->type = LVAL_FLT;
                    x->data.flt = y->data.flt;
                }
            }
        }
        if (strcmp(op, "max") == 0) {
            if (x->type == LVAL_NUM && y->type == LVAL_NUM) {
                if (x->data.num < y->data.num) {
                    x->data.num = y->data.num;
                }
            } else if (x->type == LVAL_FLT && y->type == LVAL_FLT) {
                if (x->data.flt < y->data.flt) {
                    x->data.flt = y->data.flt;
                }
            } else if (x->type == LVAL_FLT) {
                if (x->data.flt < y->data.num) {
                    x->type = LVAL_NUM;
                    x->data.num = y->data.num;
                }
            } else {
                if (x->data.num < y->data.flt) {
                    x->type = LVAL_FLT;
                    x->data.flt = y->data.flt;
                }
            }
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    if (a->cell[0]->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("head", a, 0);

        lval* v = lval_take(a, 0);
        while (v->count > 1) {
            lval_del(lval_pop(v, 1));
        }
        return v;
    }
    if (a->cell[0]->type == LVAL_STR) {
        if (strlen(a->cell[0]->data.str) == 0) {
            return lval_err("Can not take 'head' of empty string");
        }
        char* s = malloc(2);
        s[0] = a->cell[0]->data.str[0];
        s[1] = '\0';
        lval* v = lval_str(s);
        free(s);
        return v;
    }
    return lval_err("Function 'head' passed incorrect type. "
        "Got %s, Expected Q-Expression or String.", ltype_name(a->cell[0]->type));
}

lval* builtin_tail(lenv* e, lval *a) {
    LASSERT_NUM("tail", a, 1);
    if (a->cell[0]->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("tail", a, 0);

        lval *v = lval_take(a, 0);
        lval_del(lval_pop(v, 0));
        return v;
    }
    if (a->cell[0]->type == LVAL_STR) {
        if (strlen(a->cell[0]->data.str) == 0) {
            return lval_err("Can not take 'tail' of empty string");
        }
        if (strlen(a->cell[0]->data.str) == 1) {
            return lval_str("");
        }
        char* s = malloc(strlen(a->cell[0]->data.str));
        strcpy(s, a->cell[0]->data.str + 1);
        lval* v = lval_str(s);
        free(s);
        return v;
    }
        return lval_err("Function 'tail' passed incorrect type. "
    "Got %s, Expected Q-Expression or String.", ltype_name(a->cell[0]->type));
}


lval* builtin_join(lenv* e,lval* a) {
    if (a->cell[0]->type == LVAL_QEXPR) {
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
    if (a->cell[0]->type == LVAL_STR) {
        for (int i = 0; i < a->count; i++) {
            LASSERT_TYPE("join", a, i, LVAL_STR);
        }

        lval *x = lval_pop(a, 0);

        while (a->count) {
            lval* y = lval_pop(a, 0);
            x->data.str = strcat(x->data.str, y->data.str);
            lval_del(y);
        }
        lval_del(a);
        return x;
    }
    return lval_err("Function 'join' passed incorrect type. "
        "Got %s, Expected Q-Expression or String.", ltype_name(a->cell[0]->type));
}

lval* builtin_len(lenv* e,lval* a) {
    LASSERT_NUM("len", a, 1);
    if (a->cell[0]->type == LVAL_QEXPR) {
        lval* x = lval_num(a->cell[0]->count);
        lval_del(a);
        return x;
    }
    if (a->cell[0]->type == LVAL_STR) {
        lval* x = lval_num(strlen(a->cell[0]->data.str));
        lval_del(a);
        return x;
    }
    return lval_err("Function 'len' passed incorrect type. "
        "Got %s, Expected Q-Expression or String.", ltype_name(a->cell[0]->type));
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

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot define non-symbol. "
            "Got %s, expected %s.", func,
            ltype_name(syms->cell[i]->type),
            ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count - 1),
        "Function '%s' passed too many arguments for symbols. "
        "Got %i, expected %i.", func, syms->count, a->count - 1);

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        }

        if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }

    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
    return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
    return builtin_var(e, a, "=");
}

lval* builtin_exit(lenv* e, lval* a) {
    LASSERT_TYPE("exit", a, 0, LVAL_NUM);
    LASSERT_NUM("exit", a, 1);
    int x = a->cell[0]->data.num;
    lval_del(a);
    exit(x);
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

lval* builtin_lambda(lenv* e, lval* a) {
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
            "Cannot define non-symbol. Got %s, expected %s.",
            ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM)
        );
    }

    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->data.str, Lzp, &r)) {
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            lval* x = lval_eval(e, lval_pop(expr, 0));

            if (x->type == LVAL_ERR) {
                lval_println(e, x);
            }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(a);

        return lval_sexpr();
    } else {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval* err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    int r = 0;
    if (strcmp(op, "==") == 0) {
        r = lval_eq(a->cell[0], a->cell[1]);
    }
    if (strcmp(op, "!=") == 0) {
        r = !lval_eq(a->cell[0], a->cell[1]);
    }
    lval_del(a);
    return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a) {
    return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a) {
    return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a) {
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->data.num) {
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(a);
    return x;
}

lval* builtin_ord(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    if ((a->cell[0]->type != LVAL_NUM && a->cell[0]->type != LVAL_FLT) ||
        (a->cell[1]->type != LVAL_NUM && a->cell[1]->type != LVAL_FLT)) {
        return lval_err("Cannot operate on non-number!");
    }

    int r = 0;
    if (strcmp(op, ">") == 0) {
        if (a->cell[0]->type == LVAL_NUM && a->cell[1]->type == LVAL_NUM) {
            r = (a->cell[0]->data.num > a->cell[1]->data.num);
        } else if (a->cell[0]->type == LVAL_FLT && a->cell[1]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt > a->cell[1]->data.flt);
        } else if (a->cell[0]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt > a->cell[1]->data.num);
        } else {
            r = (a->cell[0]->data.num > a->cell[1]->data.flt);
        }
    }
    if (strcmp(op, "<") == 0) {
        if (a->cell[0]->type == LVAL_NUM && a->cell[1]->type == LVAL_NUM) {
            r = (a->cell[0]->data.num < a->cell[1]->data.num);
        } else if (a->cell[0]->type == LVAL_FLT && a->cell[1]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt < a->cell[1]->data.flt);
        } else if (a->cell[0]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt < a->cell[1]->data.num);
        } else {
            r = (a->cell[0]->data.num < a->cell[1]->data.flt);
        }
    }
    if (strcmp(op, ">=") == 0) {
        if (a->cell[0]->type == LVAL_NUM && a->cell[1]->type == LVAL_NUM) {
            r = (a->cell[0]->data.num >= a->cell[1]->data.num);
        } else if (a->cell[0]->type == LVAL_FLT && a->cell[1]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt >= a->cell[1]->data.flt);
        } else if (a->cell[0]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt >= a->cell[1]->data.num);
        } else {
            r = (a->cell[0]->data.num >= a->cell[1]->data.flt);
        }
    }
    if (strcmp(op, "<=") == 0) {
        if (a->cell[0]->type == LVAL_NUM && a->cell[1]->type == LVAL_NUM) {
            r = (a->cell[0]->data.num <= a->cell[1]->data.num);
        } else if (a->cell[0]->type == LVAL_FLT && a->cell[1]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt <= a->cell[1]->data.flt);
        } else if (a->cell[0]->type == LVAL_FLT) {
            r = (a->cell[0]->data.flt <= a->cell[1]->data.num);
        } else {
            r = (a->cell[0]->data.num <= a->cell[2]->data.flt);
        }
    }
    lval_del(a);
    return lval_num(r);
}

lval* builtin_log(lenv* e, lval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }
    
    lval* x = lval_pop(a, 0);

    if (a->count == 0 && strcmp(op, "!") == 0) {
        if (x->data.num == 0) {
            x->data.num = 1;
        } else {
            x->data.num = 0;
        }
    }

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);
        if (strcmp(op, "||") == 0) {
            if (x->data.num || y->data.num) {
                x->data.num = 1;
            } else {
                x->data.num = 0;
            }
        }

        if (strcmp(op, "&&") == 0) {
            if (x->data.num && y->data.num) {
                x->data.num = 1;
            } else {
                x->data.num = 0;
            }
        }
        lval_del(y);
    }

    lval_del(a);
    if (x->data.num) {
        x->data.num = 1;
    } else {
        x->data.num = 0;
    }
    return x;
}

lval* builtin_not(lenv* e, lval* a) {
    LASSERT_NUM("!", a, 1);
    return builtin_log(e, a, "!");
}

lval* builtin_or(lenv* e, lval* a) {
    return builtin_log(e, a, "||");
}

lval* builtin_and(lenv* e, lval* a) {
    return builtin_log(e, a, "&&");
}

lval* builtin_gt(lenv* e, lval* a) {
    return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a) {
    return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a) {
    return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a) {
    return builtin_ord(e, a, "<=");
}

lval* builtin_print(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(e, a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

lval* builtin_show(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("show", a, i, LVAL_STR);
        printf(a->cell[i]->data.str);
        putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    lval* err = lval_err(a->cell[0]->data.str);

    lval_del(a);
    return err;
}

lval* builtin_plugin(lenv* e, lval* a) {
    LASSERT_NUM("plugin", a, 1);
    LASSERT_TYPE("plugin", a, 0, LVAL_STR);

    char* input_path = a->cell[0]->data.str;
    char full_path[2024];

    int has_path = (strchr(input_path, '/') != NULL) || (strchr(input_path, '\\') != NULL);

    char temp_path[PATH_MAX];

    if (!has_path) {
        char* plugin_dir = getenv("LZP_PLUGIN_PATH");
        if (!plugin_dir) {
            lval_del(a);
            return lval_err("No plugin path specified and plugin filename is not a full path: %s", input_path);
        }
        snprintf(temp_path, sizeof(temp_path), "%s/%s", plugin_dir, input_path);
    } else {
        strncpy(temp_path, input_path, sizeof(temp_path));
        temp_path[sizeof(temp_path) - 1] = '\0';
    }

    char* last_slash1 = strrchr(temp_path, '/');
    char* last_slash2 = strrchr(temp_path, '\\');
    char* last_slash = last_slash1 > last_slash2 ? last_slash1 : last_slash2;

    char* filename_part = last_slash ? last_slash + 1 : temp_path;
    char* dot = strrchr(filename_part, '.');

    if (!dot) {
        snprintf(full_path, sizeof(full_path), "%s.lpp", temp_path);
    } else {
        strncpy(full_path, temp_path, sizeof(full_path));
        full_path[sizeof(full_path) - 1] = '\0';
    }

#ifdef _WIN32
    HMODULE lib = LoadLibraryA(full_path);
    if (!lib) {
        lval_del(a);
        return lval_err("Could not load plugin DLL: %s", full_path);
    }
    FARPROC proc = GetProcAddress(lib, "lzp_plugin_init");
    if (!proc) {
        lval_del(a);
        return lval_err("Plugin does not export lzp_plugin_init: %s", full_path);
    }
    lzp_plugin_init_fn init = (lzp_plugin_init_fn)proc;
    init(e);
#else
    void* lib = dlopen(full_path, RTLD_NOW);
    if (!lib) {
        lval_del(a);
        return lval_err("Could not load plugin shared object: %s", dlerror());
    }
    dlerror();
    lzp_plugin_init_fn init = (lzp_plugin_init_fn)dlsym(lib, "lzp_plugin_init");
    char* err = dlerror();
    if (err != NULL || !init) {
        lval_del(a);
        return lval_err("Plugin does not export lzp_plugin_init: %s", err ? err : "unknown");
    }
    init(e);
#endif

    lval_del(a);
    return lval_sexpr();
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
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "=", builtin_put);

    lenv_add_builtin(e, "exit", builtin_exit);
    lenv_add_builtin(e, "state", builtin_state);

    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "!", builtin_not);
    lenv_add_builtin(e, "||", builtin_or);
    lenv_add_builtin(e, "&&", builtin_and);

    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "show", builtin_show);
    lenv_add_builtin(e, "read", builtin_read);

    lenv_add_builtin(e, "plugin", builtin_plugin);
}


int main(int argc, char** argv) {
    init_parser();

    bool enable_prelude = true;
    bool shell = true;

    int opt; 
    while((opt = getopt(argc, argv, "n")) != -1) {  
        switch(opt) {  
            case 'n': enable_prelude = false; break;
        }  
    }  

    if (optind < argc) {
        shell = false;
    }


    lenv* e = lenv_new();
    lenv_add_builtins(e);

    if (enable_prelude) {
        read_xxd(e, prelude_lzp, prelude_lzp_len);
    }


    if (shell) {
        puts("Lzp Version 0.1.0");
        puts("Press Ctrl+c to Exit\n");
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
    }

    if (!shell) {
        for (int i = optind; i < argc; i++) {
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

            lval* x = builtin_load(e, args);

            if (x->type == LVAL_ERR) {
                lval_println(e, x);
            }
            lval_del(x);
        }
    }

    lenv_del(e);

    mpc_cleanup(9, Number, Float, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lzp);
    return 0;
}
