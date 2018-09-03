# Flex Language

## Motivation

I wanted to create a language that has the power and performance of C, while simultaneously, and without trade off, the expressability and power of a functional language, a multi-paradigm language. This idea morphed into an amalgamation of object oriented, procedural, and functional programming all meshed together to create a highly expressive platform for rapid development and rapid speed.

## Inspirations

Flex was inspired by C, Rust, Java, Kotlin, Haskell, JavaScript, and Go.

## Syntax

The syntax is largely taken from C for expressions, with the addition of value-equality operators. Furthermore, all statements in C are expressions in Flex, inspired by Haskell / Kotlin. Variable declarations are also expressions, and are scoped inside parent expressions.

No semicolons are required for statements, unlike C. The grammar is unambiguous, and as such, semicolons are unnecessary. Many of us personally prefer to keep them anyways, and are optionally allowed. (and optionally enforced)

All functions must be prefixed with `func`, or use a lambda expression, expressed as `<u32 arg1, u64 arg2...> u64 => someExpr`.

For more information see [GRAMMAR.md](../blob/master/spec/GRAMMAR.md).

## Memory Management

Flex has automatic memory management in the form of compile-time reference counting and stack refactoring. That is to say, it is manually managed at runtime. Manual memory management is available through the standard library.

Pointers are discouraged from use, but included to make C interfacing easier.

## Current State

The lexer and parser are done, hand written. Work is currently being done on the first intermediate representation, specifically called `prog`, as it represents the overall program, linking modules together.

Future work will be directed towards the second intermediate representation, LLIR, or low-level IR. This will be a SSA reduced instruction set.

This is where memory allocation is handed, and various standard optimizations.

Initially, at this point, we will compile to LLVM IR, and natively. Once the standard library is filled out, and a self hosting compiler is ready, custom backends will be written.

## Contributing

Contributions are welcome.