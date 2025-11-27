# Advice on using libfsm for high-performance pattern matching

libfsm compiles regular expressions to deterministic finite state machines (FSMs) and generates executable code. FSM-based matching runs in **linear time O(n)** with **no backtracking**.

Regex engines like PCRE use backtracking to explore multiple possible match paths at **runtime**.
This means the same pattern can have different execution costs depending on the input.

libfsm instead resolves all match decisions at **compile time** by constructing a Deterministic Finite Automaton (DFA).
At runtime, matching is a single linear pass over the input with no alternative paths to explore.

As a result, libfsm avoids input-dependent slowdowns and is not susceptible to regular expression–based denial-of-service (ReDoS) attacks.

**libfsm is not a drop-in replacement for traditional regex engines.** It only supports patterns that can be compiled to FSMs.

### **Topics**

- [What libfsm Cannot Do](#what-libfsm-cannot-do)
- [Quick Start](#quick-start)
- [Supported Code Generation Targets](#supported-code-generation-targets)
- [Workflow Overview](#workflow-overview)
- [Writing Effective libfsm Patterns](#writing-effective-libfsm-patterns)
- [Byte Search Optimization](#byte-search-optimization-optional)
- [Troubleshooting](#troubleshooting)
- [Pattern Matches Empty String Unintentionally](#pattern-matches-empty-string-unintentionally)

## What libfsm Cannot Do

These PCRE features will not compile:

* Word boundaries (`\b`)
* Non-greedy quantifiers (`*?`, `+?`, `??`)
* Group capture (coming soon!) and backreferences
* Lookahead/lookbehind assertions (`(?=`, `(?!`, `(?<=`, `(?<!`)
* Conditional expressions (`(?(condition)then|else)`)
* Recursion and subroutines (`(?R)`, `(?1)`)

## Quick Start

Generate a matcher from a regex:

```sh
# Generate a Go matcher
re -p -r pcre -l go -k str 'user\d+' > user_detector.go
```

This produces a standalone matcher function.

## Supported Code Generation Targets

libfsm provides stable, “first-class” code generation for:
- High-level languages: C (via `-l vmc`), Go, Rust
- LLVM IR
- Native WebAssembly

Adding code generation for new languages is straightforward and is defined in [src/libfsm/print/](../src/libfsm/print/).

## Workflow Overview

libfsm provides two main tools for pattern matching:
  - **`re`** takes patterns from the command line
  - **`rx`** takes patterns from a file

A recommended workflow when using libfsm is:

1. Validate the regex

    Test behavior using any PCRE-compatible tool (e.g., [pcregrep(1)](https://man7.org/linux/man-pages/man1/pcregrep.1.html) on the CLI or [https://regex101.com/](https://regex101.com/) in the browser).

2. Verify libfsm compatibility

    If unsupported constructs exist, libfsm reports the failing location:
    ```sh
    re -r pcre -l ast 'x*?'
    # Output: /x*?/:3: Unsupported operator
    ```
    In this example, `:3` indicates that the character at byte offset three in the pattern is an unsupported feature.

    ```sh
    # patterns with unsupported operators are output to declined.txt
    rx -r pcre -l ast -d declined.txt 'x*?'
    ```


3. Generate code

    ```sh
    re -p -r pcre -l rust -k str '^item-[A-Z]{3}\z' > item_detector.rs
    ```

4. Use multiple patterns

   Execution complexity for the generated code is proportional to the length of the text being matched, not to the number of patterns.
   Assuming your generated code isn't too large to compile, this means you can have as many patterns as you want,
   for the same time it takes to execute a single pattern.

   Take advantage of this.

    ```sh
    # re - patterns from command line:
    re -p -r pcre -l go -k str '^x?a b+c$' '^x*def?$' '^x$'
    
    # rx - patterns from file:
    rx -p -r pcre -l vmc -k str -d skipped.txt patterns.txt > detectors.c
    ```

5. Call the generated code from your program somehow

    You're on your own for this. `-k` controls the API for the generated code to read in data to match. Try different options for the language you're using and see which suits you.

    The generated API can also vary depending on how you want libfsm to handle ambiguities between different patterns. See the `AMBIG_*` flags in [include/fsm/options.h](../include/fsm/options.h) for different approaches there.

Both tools:
* Combine all patterns into one function (like using `|` to join them)
* Generate code that can return `(bool, int)` for the match status and pattern ID
* Pattern ID is argument position for `re`, line number for `rx`
* When encountering unsupported patterns: `rx` can decline them to `-d` file and generates code with working patterns only; `re` fails completely

### Common Flags

| Flag | Purpose                      | Common Options                             | Notes                                                            |
|:----:|:---------------------------- |:------------------------------------------ |:---------------------------------------------------------------- |
| `-r` | Regex dialect                | `pcre`, `literal`, `glob`, `native`, `sql` | `pcre` supports the widest set of features                       |
| `-l` | Output language for printing | `go`, `rust`, `vmc`, `llvm`, `wasm`, `dot` | Use `vmc` for `C` code. Pipe `dot` into `idot` for visualization |
| `-k` | Generated function I/O API   | `str`, `getc`, `pair`                      | `str` takes string, `pair` takes byte array, `getc` uses callback for streaming |
| `-p` | Print mode                   | *(no value)*                               | Abbrv. of `-l fsm`. Print the constructed fsm, rather than executing it.        |
| `-d` | Declined patterns            | filename                                   | Only applies to `rx` (batch mode)                                |

This is not an exhaustive list. For full flag details, see [include/fsm/options.h](../include/fsm/options.h) and the [man pages](../man).
The man pages can be built by running `bmake -r doc`, then view with `man build/man/re.1/re.1`.

## Writing Effective libfsm Patterns

Generally, to keep generated code compact, stick to the least expressive subset of features.

libfsm has no way to know in advance what text you'll be passing to its generated code.
For example, are you matching a string that you know will never contain a newline?
libfsm doesn't know that.
It has to generate code that's capable of handling any input.
You can help it out by making your patterns precise.

Think about what you intend your pattern to match, and what it's actually capable of matching given arbitrary text.
This helps restrict the scope of your pattern from arbitrary text to exactly what you mean.
The following bits of advice illustrate various specific ways to bring down this scope.

1. Replace broad wildcards

    Avoid `.*` and `.+` when possible. Wildcards match “anything,” which is often imprecise. And although they look compact, libfsm must enumerate every possible byte and continuation. This quickly leads to large DFAs.

    For example, a double-quoted string should not use `".*"` because the content cannot contain an unescaped quote. Using `.*` forces libfsm to consider all characters -- including both the presence and absence of the closing `"` at every step. This greatly increases the number of states.

    Instead, restrict it to the actual valid characters `"[^"\r\n]*"`, which matches only what is allowed and will keep the DFA more compact.

    Use negated character classes to match only the allowed content:

    | Avoid      | Better         |
    | ---------- | -------------- |
    | `<.*>` | `<[^>]*>`      |
    | `\((.*)\)` | `\([^)]*\)`|
    | `price=.+` | `price=[0-9]+` |
    | `var\s.+=` | `var\s[^=]+=`  |

    The overlap between `.*` or `.+` and strings that follow is often the cause of an “explosion” in the size of the generated FSM. So when compilation is slow or generated output is large, look for `.*` and `.+` first and replace them with a narrower character class.

2. Take care with bounded repetition

    If you have the pattern `^x{3,5}$`, libfsm's resulting DFA will be structured like "match an x, then match an x, then match an x, then match an x or skip it, then match an x or skip it, then report an overall match if at the end of input". It has to repeat the pattern, noting each time whether it's required or optional (beyond the lower count in `{min,max}`), because DFA execution doesn't have a counter, just the current state within the overall DFA.

    When the subexpression (represented by `x`) unintentionally matches too many things, they all have to be spelled out every time.
    So pay especially close attention to tightening up subexpressions in bounded repetition clauses.

3. Anchor when matching full string

    When the intention is to match an entire string, use anchors.
    Use `^` at the beginning and `\z` for the true end of the string.

    ```regex
    # Correct: matches only this exact hostname
    # Matches "web12.example.com"
    # Does not match "foo-web12.example.com-bar"
    ^web\d+\.example\.com\z 
    
    # Incorrect: would match inside a larger string
    # Matches "web12.example.com"
    # Also matches "foo-web12.example.com-bar"
    web\d+\.example\.com
    ```

4. Prefer `\z` over `$` for End-of-String

    `\z` always matches the end of the string.
    `$` will also match a trailing newline at the end of the string,
    so if you use this in combination with capturing groups, you may not be capturing what you expect.
    Also, `\z` produces a smaller FSM, so it is better to use it in places where `\n` cannot appear.

    ```regex
    # Preferred: matches only if the string ends with "bar"
    # Matches "/foo/bar"
    # Does NOT match "/foo/bar\n"
    /bar\z
    
    # Incorrect: allows a trailing newline,
    # which is usually unintended and adds unnecessary complexity
    # Matches "/foo/bar"
    # Also matches "/foo/bar\n"
    /bar$
    ```

5. Escape special characters when used as literals

    Many characters have special meaning in regex (for example `.`, `+`, `*`, `?`, `[`, `(`).
    If you mean to match them literally, escape them:

    | Literal You Want           | Correct Regex               | Explanation                                |
    |----------------------------|-----------------------------|--------------------------------------------|
    | `example.com`              | `example\.com`              | `.` matches any character unless escaped   |
    | `a+b`                      | `a\+b`                      | `+` means “one or more”                    |
    | `price?`                   | `price\?`                   | `?` means “optional”                       |
    | `[value]`                  | `\[value\]`                 | `[` and `]` start/end a character class    |
    | `(test)`                   | `\(test\)`                  | `(` and `)` begin/end a group              |
    | Markdown link `[t](u)`     | `(\[[^]]*\]\([^)]*\))`      | Matches `[text](url)` without crossing `]` or `)` |

    The `.` wildcard in particular is often mistakenly left unescaped in practice.
    On testing, it will match a literal `.` as intended. But it will also match any other character.
    This means that not only is your pattern incorrect (write negative test cases!),
    but also this part of your FSM is 256 times larger than it should be.

6. Use non-capturing groups

    Capture groups are _currently_ not supported (coming soon!).

    If you don't need to capture things, don't use capture.
    If you need grouping for alternation or precedence, use PCRE's non-capturing syntax `(?:...)`:

    ```regex
    # Correct
    (?:private|no-store)
    
    # Not what's intended
    (private|no-store)
    ```

## Byte Search Optimization

Patterns that start with an uncommon character can be accelerated using an initial byte scan before running the FSM.
This quickly jumps to likely match positions instead of scanning every byte.

Good candidates are patterns that start with uncommon prefix characters, for example:

```regex
#tag-[a-z]+
@user-[0-9]+
\[section\]
{"key":
"name='[^']+'"
```

These prefixes (`#`, `@`, `[`, `{`, `'`, `"`) are rare in normal text, so a byte search can skip ahead before running the matcher.

We found using `strings.IndexByte` before calling the generated matcher in Go code significantly improved performance when matching strings with a large (>5k) leading prefix.

## Pattern Matches Empty String Unintentionally

Pattern:

```regex
\s*
```

Will compile to code that always returns true.

This is only an issue if that is not what you intend.

**Fix options:**

* Require at least one match: `\s+`
* Anchor context: `^\s+$` or alternatively, use `-Fb` flag
