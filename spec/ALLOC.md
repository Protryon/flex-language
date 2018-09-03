# Memory Allocation Scheme

## Pointers

Pointers are always managed manually. No attempt should be made to automate their destruction due to to their unrestrained power.

They should be avoided unless absolutely necessary.

## Non-Array Values

Both classes and primitives are stored on the stack. Their allocation may be hoisted to callers if returned directly or indirectly.

If they are stored in a global variable, i.e. a static or module variable, then they are stored in static memory.

## Arrays

### Local Arrays

Local arrays, or arrays which do not leave a function or enclosed functions, are stored on the stack if of stack length, or closely bounded length.

Dynamic arrays, sufficiently large arrays, or resized arrays, are allocated on a thread-specific sparse stack for our particular function.

Sparse stack allocation should actually happen in the caller.

Any arrays inside this array (directly or indirectly) are also local, and using their own sparse stack.

Static length internal arrays may be inlined inside parent arrays.

### Non-Local Arrays

A non-local array is an array that is returned directly or indirectly. 

They are hoisted up the call stack until they are not returned, and become local arrays.

They should be reused for different function calls if possible, otherwise, allocate using a sparse stack.

Internal arrays are inherently non-local as well, and hoisted accordingly.

### Non-Linear Memory Deallocation Handling

In the event of some sparse stack allocated array degenerating in a non-linear fashion, the compiler should insert a high performance pointer hashset of non-locally freed entries for that sparse stack. Any linear free operation should check this hashset for the head of the sparse stack iteratively, and pop all entries from the stack. Furthermore, if the hashset's length approaches some percentage or absolute wasted memory, then the allocator should request memory from the hashset, rather than from the sparse stack.

In some circumstances, it may be ideal to use self modifying code to replace stack allocation with hashset allocation as needed, or to have multiple copies of functions.

### Static Arrays

Static arrays are allocated compile time in static memory. They are immutable, cannot be allocated runtime, and cannot be freed.

### Global Arrays

Global arrays are arrays referenced by global memory. They are freed when their primary global reference is modified.

They may also transition to local or non-local arrays, and are sourced from them accordingly.

A local to global transition is a shallow copy, however any arrays that must become global should be optimized accordingly.

Any global to local transition is a pointer copy, and the global is not freed.

## Class Referencability

Class variables, if declared as a class variable, are pointers if they are not defined in the initializer only, and non-self-referential directly or indirectly.

The `this` keyword is always a pointer to a class, and never null.

Function arguments and local variables will be allocated on the call stack, but used as pointers.

Non-local variables are allocated on the call stack, hoisted.

### Dynamic Memory

In order to support C code running the same memory space, and to provide end users with manual memory management if required, a `malloc`/`realloc`/`calloc`/`free` interface is provided. No compiler aided optimizations will be performed on these allocations, but the allocators will be implemented to not interfere with the sparse stack allocator.

Furthermore, thread local versions of these operations will be available as `tlmalloc`, `tlrealloc`, `tlcalloc`, and `tlfree`.

Furthermore, `mmap` may need to be intercepted and prevented from allocating pages reserved for sparse stacks. Optionally, it may be possible, on some platforms, to explicitly mark a memory region as reserved, or to map a zero value sinkhole.

`brk` must be intercepted and always fail.