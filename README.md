# LZP

Following: https://buildyourownlisp.com/

# Documentation

## Overview

This LZP programming language is designed for simplicity and flexibility, enabling users to define functions, perform mathematical operations, manipulate strings, and handle errors. The language supports standard Lisp constructs, including S-expressions and Q-expressions, and provides built-in functions for various common tasks.

## Usage

Starting an interactive shell

```sh
./lzp
```

Running a file

```sh
./lzp ./examples/pi.lzp
```

## Prelude

Lzp had a build in prelude that can be disabled by passing the `-n` flag.
Have a look in the [prelude.lzp](./prelude.lzp) file to see what is included.

## Types

- **NUM**: Integer numbers (long)
- **FLT**: Floating-point numbers (double)
- **STR**: Strings
- **SYM**: Symbols
- **SEXPR**: S-expressions
- **QEXPR**: Q-expressions
- **FUN**: Functions (both built-in and user-defined)
- **ERR**: Error messages

## Built-in Functions

### List/String Manipulation

#### List

Converts arguments into a Q-expression.

```sh
lzp> list 42 {1 b 1.45} list
{42 {1 b 1.45} <list>}
```

#### Head

Returns the first element of a Q-expression or string.

```sh
lzp> head {+ 3 - 4}
{x}

lzp> head "exmpl"
"e"
```

#### Tail

Returns all elements except the first of a Q-expression or string.

```sh
lzp> tail {+ 3 - 4}
{3 - 4}

lzp> tail "exmpl"
"xmpl"
```

#### Eval

Evaluates a Q-expression as an S-expression.

```sh
lzp> eval {+ 1 1}
2
```

#### Join

Concatenates Q-expressions or strings.

```sh
lzp> join {+ 1} {2 3}
{+ 1 2 3}

lzp> join "test" "ing"
"testing"
```

#### Len

Returns the length of a Q-expression or string.

```sh
lzp> len {+ 1 2 3}
4

lzp> len "testing"
7
```

### Arithmetic Operations

#### `+` Addition

```sh
lzp> + 1 -.5
0.5
```

#### `-` Subtraction

```sh
lzp> - 10 .2
9.8

lzp> - 5
-5
```

#### `*` Multiplication

```sh
lzp> * 10 2
20

lzp> 10 .5
5
```

#### `/` Division

```sh
lzp> / 10 2
5

lzp> / 0 10
0
```

#### `%` Modulus

```sh
lzp> % 13 5
3
```

#### `**` Exponentiation

```sh
lzp> ** 2 8
256

lzp> ** 225 .5
15
```

#### `min` Minimum

```sh
lzp> min -4 .2 10
-4
```

#### `max` Maximum

```sh
lzp> min -4 .2 10
10
```

### Variable Handling

#### Def

Defines global variables.

```
lzp> def {x} 123
()

lzp> + x 100
223
```

#### `=` Assign

Assigns values to variables in the current scope.

```sh
lzp> def {i} -1
lzp> def {j} -1
lzp> (\ {_} {def {i} 99}) ()
lzp> (\ {_} {= {j} 99}) ()
lzp> i
99
lzp> j
-1
```

### Logical Operations

#### `>` Greater than

```sh
lzp> > -10 2.1
0
```

#### `<` Less than

```sh
lzp> < -10 2.1
1
```

#### `>=` Greater than or equal to

```sh
lzp> >= -10 2.1
0

lzp> >= 2 2
1
```

#### `<=` Less than or equal to

```sh
lzp> <= -10 2.1
1

lzp> <= 2 2
1
```

#### `==` Equal to

```sh
lzp> == 10 10
1

lzp> == 0 12
0
```

#### `!=` Not equal to

```sh
lzp> != 10 10
0

lzp> != 0 12
1
```

#### `if` Conditional expressions

```sh
lzp> if 1 { "true" } { "false" }
"true"

lzp> if 0 { "true" } { "false" }
"false"
```

#### `!` Logical NOT

```sh
lzp> ! 0
1

lzp> ! 5
0
```

#### `||` Logical OR

```sh
lzp> || 0 0
0

lzp> || 0 0 1 0
1
```

#### `&&` Logical AND

```sh
lzp> && 1 0
0

lzp> && 1 1 1 1
1
```

### Functions and Evaluation

#### `exit`

Exits the program with a given status code.

```sh
lzp> exit 1
echo $?
1
```

#### `state`

Prints the current environment state.

```sh
lzp> def {x} 213
lzp> state ()
+ = <+>
- = <->
* = <*>
/ = </>
% = <%>
...
x = 213
```

#### `\` Lambda

Defines lambda functions.
The `&` syntax allows for varible amount of vars as input.

```sh
lzp> (\ {a b} {+ a b}) 1 2
3

lzp> def {add} (\ {a b} {+ a b})
lzp> add 1 2
3

lzp> def {temp} (add 1)
lzp> temp 2
3

lzp> def {add} (\ {& l} {eval (join (list +) l)})
lzp> add 4
4
lzp> add 1 2 3 4 5
15
```

#### `load`

Loads and evaluates code from a file.

```sh
lzp> load "./examples/pi.lzp"
Calculating...
3.14149265359004
()
```

#### `read`

Parses and evaluates a string as code.

```sh
lzp> read "(def {r} 100); comment\n (show \"aaa\")"
aaa
()
lzp> r
100
```

#### `print`

Prints values, can be of any type.

```sh
lzp> print add
(\ {& l} {eval (join (list +) l)})

lpz> print +
<+>

lpz> print "string"
"string"

lzp> print "string\ncool"
"string\ncool"
```

#### `show`

Displays strings.

```sh
lzp> print "string"
string

lzp> show "string\ncool"
string
cool
```

#### `error`

Returns an error message.

```sh
lzp> error "not good"
Error: not good
```
