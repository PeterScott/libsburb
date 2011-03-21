Some causal tree manipulation code
===============

This is a library of heavily optimized code for causal tree manipulation,
written as part of my devious plan to get out of grad school. Currently capable
of inserting patches into simple vector-based weaves, with excellent speed. Some
of the primitives, like waiting sets and wefts, are really nicely implemented
here, and would be handy in a higher-level language implementation when wrapped
with some FFI magic. The higher-level code, while it's based on really elegant
and proper formulations of everything (which is nontrivial), is still really
ugly due to the low-level representations and the natural unpleasantness of C.

I believe that, in order to go from this to something that anybody would want to
use, the right option would be to do a rewrite in a Lisp dialect, with some
heavy macrology to wrap the ugly internal representations and heavyweight
operations with decent abstractions. C just doesn't cut it.

This code is provided as-is, and I'll probably not be back to build more on it,
though I expect it will be very useful as a source of parts for salvage.

The name is a reference to [Homestuck](http://www.mspaintadventures.com/?s=6),
because I needed to call it *something*.
