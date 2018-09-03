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

A function is a series of expressions. They can be nested inside modules or classes.

They can be declared using the `func` keyword, or via lambda expressions.

Allowed modifiers:
* `pub`/`prot`/`priv`: Same as class.
* `csig`: If exported, name mangling will not occur.
* `async`: Must return a reference, and may execute in parallel. Returning is atomic.
* `synch`: All children of the containing classes are written to atomically. Redundant if the class is marked `synch`.
* `virt`: If the parent class is inherited, this function will be implicitly called by its overridding functions.

## Expressions

### For

`for (int i = 0; i < 10; i++) {}`

### For In

`for (int i : myArray) {}`

`for (void* entry, int index : myObject) {}`

### While

`while (x < 5) {}`

### Do

`do {} while (x < 5)`

### If, Else If

`if (x < 5) {} else if (x < 10) {} else {};`

### Switch

`switch (x) {case 5: break; case 10: default: }`

### Goto

`goto label;`

`label:`

### Mathematics

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