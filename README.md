# buildyourownlisp

Following: https://buildyourownlisp.com/

## Examples

### Fibonacci sequence

```lisp
def {fib} (\ {n} {if (<= n 1) {n} {+ (fib (- n 1)) (fib (- n 2))}})
```

```shell
lzp> fib 25
75025
```
