# "Make it Async" Examples

This is a set of examples for [a blog post](https://blog.vito.nyc/posts/make-it-async/)
I wrote about building asynchronous resources in ASIO. It also serves as maybe
an interesting case study of ASIO's support for C++20 coroutines, or perhaps for
integrating C++ I/O with Python application logic. It's got a little of
everything.

The goal was to present a quasi-practical integration, so for example there's
correct handling of SIGINT passthrough and restoration, but _only_ SIGINT.
There's correct error detection for various operations, but the result of any
error is an unlogged drop of the current connection.

The figures are all located in their respective directories under the `src/`
folder. **Figure1** and **AppendixA** are plain C++ programs, the rest are all
implemented as Python extension modules. After compiling a Python extension
module, it can be run by placing it in the same folder as its respective Python
script and running the Python script.

All of the figures can be interacted with via the provided **Client.py** script,
which is just a read-write-print loop, as all of the figures are merely
variations on an echo server.

## License

This source code (and associated build/test/utility files) is released into the
public domain via CC0-1.0, see `License` for details.

I (Vito Gamberini) hold no patents and have no knowledge of any patented
techniques used by these examples. However, some organizations refuse to
incorporate or distribute public domain code due to patent concerns, for this
reason these examples are additionally licensed for use under Zlib, see
`UsageLicense` for details.

As the source code itself is public domain, there is no need to retain licensing
information in full or partial source code distributions unless the patent
concerns are of significance to you.

The purpose in using these well known legal texts is that they are widely
trusted and understood by the open source community. A bespoke legal text might
limit usage by particularly skittish developers or organizations. However, I
want to make my intentions clear in plain English as well:

**Do what you want with this stuff. There are zero restrictions on the use of
this code in any context.**
