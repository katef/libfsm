
## libfsm and friends: NFA, DFA, regular expressions and lexical analysis

    ; re -cb -pl dot '[Ll]ibf+(sm)*' '[Ll]ibre' | dot
![libfsm.svg](doc/tutorial/libfsm.svg)

Getting started:

 * See the [tutorial introduction](doc/tutorial/re.md) for a quick overview
   of the re(1) command line interface.
 * [Compilation phases](doc/tutorial/phases.md) for typical applications
   which compile regular expressions to code.

Lexing is the process of categorising a stream of items by their spellings.
The output from this process is a stream of tokens, each of a specific lexeme
category, which are most commonly input to a parser responsible for asserting
the order of these tokens is valid.

lx is an attempt to produce a simple, expressive, and unobtrusive lexer
generator which is good at lexing, does just lexing, is language independent,
and has no other features.

You get:

 * libfsm — library for manipulating FSM (NFA and DFA)
 * libre  — library for compiling regular expressions to NFA
 * fsm(1) — command line interface for FSM
 * re(1)  — command line interface for executing regular expressions
 * lx(1)  — lexer generator

Clone with submodules (contains required .mk files):

    ; git clone --recursive https://github.com/katef/libfsm.git

To build and install:

    ; pmake -r install

You can override a few things:

    ; CC=clang PREFIX=$HOME pmake -r install

Building depends on:

 * Any BSD make. This includes OpenBSD, FreeBSD and NetBSD make(1)
   and sjg's portable bmake (also packaged as pmake).

 * A C compiler. Any should do, but GCC and clang are best supported.

 * ar, ld, and a bunch of other stuff you probably already have.

Fuzzing depends on the theft property-based testing library:

 * https://github.com/silentbicycle/theft  
   Tests are currently based on the libtheft 0.4.2 API.

Ideas, comments or bugs: kate@elide.org

