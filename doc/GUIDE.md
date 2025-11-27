# Using libfsm for High-Performance Pattern Matching

libfsm compiles regular expressions to deterministic finite state machines (FSMs) and generates executable code. FSM-based matching runs in **linear time O(n)** with **no backtracking**.

> Regex engines like PCRE use backtracking to explore multiple possible match paths at **runtime**.
> This means the same pattern can have different execution costs depending on the input.
>
> libfsm instead resolves all match decisions at **compile time** by constructing a Deterministic Finite Automaton (DFA).
> At runtime, matching is a single linear pass over the input with no alternative paths to explore.
>
> As a result, libfsm avoids input-dependent slowdowns and is not susceptible to regular expression–based denial-of-service (ReDoS) attacks.

**libfsm is not a drop-in replacement for traditional regex engines.** It only supports patterns that can be compiled to FSMs.

### **Topics**

- [What libfsm Cannot Do](#what-libfsm-cannot-do)
- [Quick Start](#quick-start)
- [Supported Code Generation Targets](#supported-code-generation-targets)
- [Workflow Overview](#workflow-overview)
- [Writing Effective libfsm Patterns](#writing-effective-libfsm-patterns)
- [Byte Search Optimization (Optional)](#byte-search-optimization-optional)
- [Troubleshooting](#troubleshooting)

## What libfsm Cannot Do

These PCRE features will not compile:

* Word boundaries (`\b`)
* Non-greedy quantifiers (`*?`, `+?`, `??`)
* Group capture (coming soon!) and backreferences
* Lookahead/lookbehind assertions (`(?=`, `(?!`, `(?<=`, `(?<!`)
* Conditional expressions (`(?(condition)then|else)`)
* Recursion and subroutines (`(?R)`, `(?1)`)

---

## Quick Start

Generate a matcher from a regex:

```bash
# Generate a Go matcher
re -p -r pcre -l go -k str 'user\d+' > user_detector.go
```

This produces a standalone matcher function.

---

## Supported Code Generation Targets

libfsm provides stable, “first-class” code generation for:

| Category             | Output                         |
| -------------------- | ------------------------------ |
| High-level languages | **C (via `-l vmc`), Go, Rust** |
| Toolchains           | **LLVM IR**                    |
| Virtualization       | **Native WebAssembly**         |

> Adding code generation for new languages is straightforward and is defined in [src/libfsm/print/](../src/libfsm/print/).

---

## Workflow Overview

libfsm provides two main tools:
  - **`re`** takes patterns from command line
  - **`rx`** takes patterns from file

A recommended workflow when using libfsm is:

### 1. Validate the Regex

Test behavior using any PCRE-compatible tool (e.g., [pcregrep(1)](https://man7.org/linux/man-pages/man1/pcregrep.1.html) on the CLI or [https://regex101.com/](https://regex101.com/) in the browser).

### 2. Verify libfsm Compatibility

```bash
re -r pcre -l ast 'x*?'
# Output: /x*?/:3: Unsupported operator
# :3 indicates that the character at offset 3 in the pattern is rejected.

rx -r pcre -l ast -d declined.txt 'x*?'
# Unsupported character in declined.txt
```

If unsupported constructs exist, libfsm reports the failing location.

### 3. Generate Code

```bash
re -p -r pcre -l rust -k str '^item-[A-Z]{3}\z' > item_detector.rs
```

### 4. Multiple Patterns

```bash
# re - patterns from command line:
re -p -r pcre -l go -k str '^x?a b+c$' '^x*def?$' '^x$'

# rx - patterns from file:
rx -p -r pcre -l vmc -k str -d skipped.txt patterns.txt > detectors.c
```

Both tools:
* Combine all patterns into one function (like using `|` to join them)
* Return `(bool, int)` - match status and pattern ID
* Pattern ID is argument position for `re`, line number for `rx`
* When encountering unsupported patterns: `rx` skips them to `-d` file and generates code with working patterns; `re` fails completely

---

### Flag Reference
| Flag | Purpose                      | Common Options                             | Notes                                                            |
| ---- | ---------------------------- | ------------------------------------------ | ---------------------------------------------------------------- |
| `-r` | Regex dialect                | `pcre`, `literal`, `glob`, `native`, `sql` | `pcre` supports the widest set of features                       |
| `-l` | Output language for printing | `go`, `rust`, `vmc`, `llvm`, `wasm`, `dot` | Use `vmc` for `C` code. Pipe `dot` into `idot` for visualization |
| `-k` | Generated function I/O API   | `str`, `getc`, `pair`                      | `str` takes string, `pair` takes byte array, `getc` uses callback for streaming |
| `-p` | Print mode                   | *(no value)*                               | Abbrv. of `-l fsm`. Print the constructed fsm, rather than executing it.        |
| `-d` | Declined                     | filename                                   | Only applies to `rx` (batch mode)                                |

This is not exhausted list. For full flag details, see [include/fsm/options.h](../include/fsm/options.h) and the [man pages](../man).
The man pages can be built by running `bmake doc`, then view with `build/man/re.1/re.1`.

---

## Writing Effective libfsm Patterns

### 1. Replace Broad Wildcards

Avoid `.*` and `.+` when possible. Wildcards match “anything,” which is often imprecise and forces libfsm to build a large DFA.

For example, a double-quoted string should not use `".*?"` because the content cannot contain an unescaped quote.
Instead, restrict it to the actual valid characters `"[^"\r\n]*"`, which matches only what is allowd and will keep the DFA more compact.

Use negated character classes:

| Avoid      | Better         |
| ---------- | -------------- |
| `<.*>`     | `<[^>]*>`      |
| `\((.*)\)` | `\([^)]*\)`    |
| `price=.+` | `price=[0-9]+` |
| `var\s.+=` | `var\s[^=]+=`  |

> This is often the cause of an “explosion” in the size of the generated FSM.
>
> See [Compilation Takes Too Long](#compilation-takes-too-long) for more details.

---

### 2. Anchor When Matching Full String

When the intention is to match an entire string, use anchors.
Use `^` at the beginning and `\z` for the true end of the string.

```regex
# Correct: matches only this exact hostname
^web\d+\.example\.com\z 

# Incorrect: would match inside a larger string
web\d+\.example\.com        # also matches "foo-web12.example.com-bar"
```

---

### 3. Prefer `\z` Over `$` for End-of-String

`\z` always matches the end of the string.
`$` will also match a trailing newline at the end of the string,
so if you use this in combination with capturing groups, you may not be capturing what you expect.
Also, `\z` is more efficient, so it is better to use it in places where `\n` cannot appear.

```regex
# Preferred
/foo\z

# Risky: $ may allow an extra newline
/foo$
```

---

### 4. Escape Special Characters When Used As Literal

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

---

### 5. Use Non-Capturing Groups

Capture groups are _currently_ not supported (coming soon!).
If you need grouping for alternation or precedence, use non-capturing syntax `(?:...)`:

```regex
# Correct
(?:private|no-store)

# Unsupported
(private|no-store)
```

---

## Byte Search Optimization (Optional)

Patterns that start with an **uncommon character** can be accelerated using an initial byte scan before running the FSM.
This quickly jumps to likely match positions instead of scanning every byte.

Good candidates are patterns that start with uncommon prefix characters, for example:

```
#tag-[a-z]+
@user-[0-9]+
\[section\]
{"key":
"name='[^']+'"
```

These prefixes (`#`, `@`, `[`, `{`, `'`, `"`) are rare in normal text, so a byte search can skip ahead before running the matcher.

We found using `strings.IndexByte` before calling the generated matcher in Go code significantly improved performance when matching strings with a large (>5k) leading prefix.

---

## Troubleshooting

### Pattern Matches Empty String Unintentionally

Pattern:

```regex
\s*
```

Will compile to code that always returns true.

This is only an issue if that is not what you intend.

**Fix options:**

* Require at least one match: `\s+`
* Anchor context: `^\s+$` or alternatively, use `-Fb` flag

### Compilation Takes Too Long

This is often caused by unrestricted wildcards (`.*`, `.+`).
Although they look compact, libfsm must enumerate every possible byte and every possible continuation, causing the state machine to grow quickly.

For example, to match `var anything =`, a pattern such as `var\s.+=` looks simple, but `.+` forces libfsm to encode every possible byte
and every possible continuation -- including both the presence and absence of `=`. This drastically increases the number of states.

When compilation is slow, look for broad wildcards and replace them with more specific character classes (as shown [above](#writing-effective-libfsm-patterns)),
such as: `var\s[^=]+=`.
