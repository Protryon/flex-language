# Notice

This specification is very loose, and is more so a mirror of the compiler, than the compiler is a mirror of it.

The grammar was developed to be unambiguous, easy to parse, and extensible. It is subject to overhaul at any time.

# Flex Grammar

## Program

A program is made up of any number of modules in any number of files.

## Module

A module can include any number of classes, functions, modules, typedefs, or variable declaration expressions.

Allowed modifiers:
* `pub`: All internal symbols, unless marked otherwise, are accessible from other programs and modules.
* `prot`: All internal symbols, unless marked otherwise, are accessible from other modules, but never other programs. Default protection.
* `priv`: All internal symbols, regardless of marking, are inaccessible from outside this module.

Directives:
* `import`: Imports Flex or C library.

## Class

A class is a series of variable declaration expressions and member functions.

Allowed modifiers:
* `pub`: No effect. Default protection.
* `prot`: Same as module, except scope is within module.
* `priv`: Same as module, except scope is within module.
* `typed`: Class stores internal type information at runtime.
* `synch`: All children are strictly read/written to atomically.
* `virt`: This class may be inherited by other classes.
* `iface`: This class may not define function bodies or variables. Used only to generalize types.

## Function

A function is a series of expressions. They can be nested inside modules, classes, or other functions.

They can be declared using the `func` keyword, or via lambda expressions.

Allowed modifiers:
* `pub`/`prot`/`priv`: Same as class.
* `csig`: If exported, name mangling will not occur.
* `async`: Must return a reference, and may execute in parallel. Returning is atomic.
* `synch`: All children of the containing classes are written to atomically. Redundant if the class is marked `synch`.
* `virt`: If the parent class is inherited, this function will be implicitly called by its overridding functions.

Syntax:

`func return_type name(u32 arg1, u32 arg2 = 632, u32... argn) expr`

Note that names are optional, and empty for constructors of classes and initializers of modules.

Expressions are required after a function declaration, however an empty body or semicolon may be used to indicate no effect.

Functions are first class objects.

### Lambdas

A lambda can be declared in a module, class, or internal to a function. They are identical in function to functions, except that a name cannot be specified.

Syntax:

`<u32 arg1, u32 arg2> return_type => expr`

The return type is optional, but recomended in cases of primitives being returned.

As flex is strongly and statically typed, all argument types are explicit.

## Expressions

## Functions and Lambdas

All functions and lambdas are expressions, equivilant to a `lang.function` type.

### Body

Bodies, aka blocks, are used to group multiple expressions together, the last of which is result of the body expression. If none are present, then 

Syntax:

`{ expr1 expr2 expr3 }`

### Variable Declarations

Variable declarations, allowed in functions, classes, and modules, declare a new variable in the current scope.

Variable declarations are expressions, with their returned value being their initialized value, or a default (i.e. 0, null, etc).

Keywords (only valid in class/module):
* `csig`: C signature, like functions.
* `synch`: The variable is atomic for all accesses. Note that the compiler will automatically designate `synch`.

Scope destroys a variable, including when created more deeply than a body expression. The following is valid:

`ret uint32 myVar = 7, someFunc(myVar), myVar`

Note that sequence expressions are not possible inside variable initializers.

### Unary Postfix



### Unary Prefix



### Call



### Calculated Member



### Cast



### Binary

### Type

### Integer Literal

### Decimal Literal

### String Literal

### Char Literal

### Identifer

### Ternary

### If

### For

### While

### For Each

### Switch

### Goto & Label

### Ret

### Continue

### Break

### Try

### Throw

### Catch

### Finally

### New

### Inferenced New

### Null


Defined operators:
* `+`
* `-`
* `*`
* `/`
* `%`
* `**`
* `|`
* `&`
* `^`
* `~`
* `!`
* `&&`
* `||`
* `=`
* `+=`
* `-=`
* `*=`
* `/=`
* `%=`
* `|=`
* `&=`
* `^=`
* `~=`
* `!=`
* `&&=`
* `||=`
* `**=`
* `+==`
* `-==`
* `*==`
* `/==`
* `%==`
* `**==`
* `|==`
* `&==`
* `^==`
* `~==`
* `!==`
* `&&==`
* `||==`

### Comparison

Defined operators:
* `==`
* `!=`
* `===`
* `!==`
* `<`
* `<=`
* `>`
* `>=`

### Management

Defined keywords:
* `new`

# Types

## Primitives

* `byte`/`char`
* `uint16`
* `int16`
* `uint32`
* `int32`
* `uint64`
* `int64`

## Pointers

* Just like C pointers
* Transparently works over inner constructs: classes, arrays, etc

## Arrays

* Multidimensonal data blocks, basically pointers
* Static Size

## Objects

* Instantiated classes
* Also can be accessed via pointer.
* Is actually a pointer under the hood.

# Standard Library

## I/O

### Streams

### Standard Input/Output

### File IO

### Network IO

### Shared Memory IO

## Data

* `iterable`
* `iterator`
* `indexable`
* `hashmap`
* `imhashmap`
* `lhashmap`
* `imlhashmap`
* `hashset`
* `imhashset`
* `list`
* `Imlist`
* `doublelist`
* `imdoublelist`
* `skiplist`
* `imskiplist`
* `arraylist`
* `imarrayaist`
* `queue`
* `imqueue`
* `stack`
* `imstack`
* `bitarray`
* `concat`
* `single`
* `pair`
* `tuple`
* `tuple3`
* `tuple4`
* `tuple5`
* `tuple6`
* `tuple7`
* `tuple8`
* `tuple9`
* `string`
* `imstring`

### Iterable Operations
* `map`
* `filter`
* `mapfilter`
* `multimap`
* `every`
* `some`
* `collectArray`
* `collectArrayList`
* `collectLinkedList`
* `size`
* `reverse`
* `iterator`
* `riterator`

### Indexable Operations
* All Iterable functions
* `copy`
* `chunk`
* `fill`
* `slice`
* `qsort`
* `splice`
* `pop`
* `push`
* `popBack`
* `pushBack`
* `size`
* `concat`
* `contains`
* `startsWith`
* `endsWith`
* `indexOf`
* `indexesOf`
* `lastIndexOf`
* `stringjoin`

## Parallelism

* `mutex`
* `rwlock`
* `condition`
* `threadiface`

## Math

* All trig funcs
* Exponentials, etc
* Algebraic classes
* Imaginary classes

## System
* Current directory
* File system permissions
* Environment variables
* Hardware information
* Program execution
* Manual Memory Management
