# LLektor -- Metaprogramming for the Misguided

If you're wondering what this is, see [metaprogramming for
madmen](https://fgiesen.wordpress.com/2012/04/08/metaprogramming-for-madmen/)
for background. This is not *that* lekktor, but a reimplementation of
the same terrible idea using LLVM.

## Requirements

* LLVM 11
* A Unix system (Tested on Linux)
* An LLVM-based compiler (Tested with clang)

Porting to another version of LLVM should be fairly easy. Porting to a
non-UNIX OS will be more difficult; exporting a trace depends on the
presence of `mmap`, and reading the trace file is done using
traditional UNIX interfaces (`open`, `lseek`, and `pread`).

## Building

```
mkdir build
cd build
cmake ..
make
```

Then, optionally copy `llektor/LLektor.so` and
`llektor_stub/libllektor_stub.a` somewhere convenient.

## Usage

Compile the victim as usual, but make your compiler output LLVM
bitcode, and do not link yet.  For clang, this can be done with either
the `-flto` or the `-emit-llvm` flags. No optimization is necessary at
this step (but it likely won't hurt)

E.g.,

```
clang -c -flto -o program.bc program.c
```

If you have multiple source files, link them together while staying in LLVM bitcode, using the `llvm-link` command. (On debian, this is installed as `llvm-link-11`):

```
llvm-link -o linked-base.bc program.bc another-object.bc etc.bc
```

This is necessary because otherwise it is possible that duplicate
functions in different modules (e.g., template expansions or inline
functions) might be hacked apart differently, and it's a crapshoot
which version of the code you get in the final link. (Normally, this
doesn't matter because all versions of the compiled code are supposed
to be the same).

Now, "tag" that object file with the information that llektor needs:

```
opt -load /path/to/LLektor.so --llektor-tag -o linked-tagged.bc linked-base.bc
```

Then, add the instrumentation code to that tagged file:

```
opt -load /path/to/LLektor.so --llektor-instr -o linked-instr.bc linked-tagged.bc
```

and link the resuling program:

```
clang -o program-instr linked-instr.bc /path/to/llektor_stub.a <other libraries and linking flags>
```

Run this instrumented program to collect code usage information. This
can be done multiple times with the same tracefile; the runs will be
merged, and all code used in *any* run will be kept:

```
LLEKTOR_TRACE=/path/to/tracefile ./program-instr
```

When you're satisfied that all edge cases that you care about have
been exercised, it's time to run the pruning pass:

```
opt -load /path/to/LLektor.so --llektor-prune --llektor-tracefile /path/to/tracefile -o linked-pruned.bc linked-tagged.bc
```

Finally, perform a final link on that pruned bitcode:

```
clang -Oz -o program linked-pruned.bc <other libraries and linking flags>
```

(You may use different optimzation flags if you wish, but if you're
not at least using `-Oz`, you probably didn't want to use llektor in
the first place)

## Statistics

On [this Linux demo](https://github.com/faemiyah/faemiyah-demoscene_2019-08_4k-intro_region_de_magallanes/tree/bc3cd25073b46b6f34df9515dd493d4ec6dea18d),
compiled straight on x86_64 without any packer, the basic `-Os` build
was 804k. After llektoring, this was brought down to 300k without any
noticable loss in functionality.

## Warning

This is the equivalent of amputating a leg to lose weight. Chances
are, it will break your code.  Consider whether the faustian deal
you're making is worth it.

## Support

If you're using this, I recommend reaching out for help. Therapy is
probably the best choice, but failing that, feel free to make use of
the issue tracker.
