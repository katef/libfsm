# Using libfsm for High-Performance Pattern Matching

libfsm compiles regular expressions to deterministic finite state machines (FSMs) and generates executable code. FSM-based matching runs in **linear time O(n)** with **no backtracking**.

**libfsm is not a drop-in replacement for traditional regex engines.** It only supports patterns that can be compiled to FSMs.

## What libfsm Cannot Do

These PCRE features will not compile:

* Word boundaries (`\b`)
* Non-greedy quantifiers (`*?`, `+?`, `??`)
* Group capture and backreferences
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

> Adding code generation for new languages is template-driven and straightforward.

---

## Workflow Overview

libfsm provides two main tools: **`re`** takes patterns from command line, **`rx`** takes patterns from file.

### 1. Validate the Regex

Test behavior using any PCRE-compatible tool (e.g., [https://regex101.com/](https://regex101.com/)).

### 2. Verify libfsm Compatibility

```bash
re -r pcre -l ast 'x*?'
# Output: /x*?/:3: Unsupported operator

rx -r pcre -l ast -d declined.txt 'x*?'
# Unsupported character in declined.txt
```

If unsupported constructs exist, libfsm reports the failing location.

### 3. Generate Code

```bash
re -p -r pcre -l rust -k str '^item-[A-Z]{3}$' > item_detector.rs
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
| Flag | Purpose                     | Common Options                             | Notes                                      |
| ---- | --------------------------- | ------------------------------------------ | ------------------------------------------ |
| `-r` | Select regex dialect        | `pcre`, `literal`, `glob`, `native`, `sql` | `pcre` supports the widest set of features |
| `-l` | Choose output language      | `go`, `rust`, `vmc`, `llvm`, `wasm`, `dot` | Use `vmc` for `C` code, pipe `dot` into `idot` for visualization |
| `-k` | Generated function I/O API  | `str`, `getc`, `pair`                      | `str` takes string, `pair` takes byte array, `getc` uses callback for streaming |
| `-p` | Production mode             | *(no value)*                               | Generates optimized code                   |
| `-d` | Output unsupported patterns | filename                                   | Only applies to `rx` (batch mode)          |

For more detailed information on flags, see [include/fsm/options.h](../include/fsm/options.h) and the man pages (by running `build/man/re.1/re.1` after `bmake doc`).

---

## Writing Effective libfsm Patterns

For additional regex best practices, see [Fastly's regex guide](https://www.fastly.com/documentation/reference/vcl/regex/#best-practices-and-common-mistakes).

### 1. Replace Broad Wildcards

Avoid `.*` whenever possible. Use negated character classes:

| Avoid      | Better         |
| ---------- | -------------- |
| `<.*>`     | `<[^>]*>`      |
| `\((.*)\)` | `\([^)]*\)`    |
| `price=.*` | `price=[0-9]+` |

---

### 2. Anchor When You Require Full Matches

FSMs only do what’s specified. Explicitly anchor when matching entire strings:

```regex
^task-[a-z]+-[0-9]{2}\z
```

Use `\z` for end-of-string.

---

## Byte Search Optimization (Optional)

Patterns that start with an **uncommon character** can be accelerated using an initial byte scan before running the FSM.
This quickly jumps to likely match positions instead of scanning every byte.

### Common fast byte search APIs

| Language | Function                   |
| -------- | -------------------------- |
| Go       | `strings.IndexByte`        |
| Rust     | `memchr::memchr`           |
| C        | `memchr` from `<string.h>` |


### Good candidates

Patterns that always start with uncommon prefix characters, for example:

```
#tag-[a-z]+
@user-[0-9]+
\[section\]
{"key":
"name='[^']+'"
```

These prefixes (`#`, `@`, `[`, `{`, `'`, `"`) rarely appear in normal text, making a byte search highly effective.

---

## Troubleshooting

### Pattern Matches Empty String Unintentionally

Pattern:

```regex
\s*
```

Will compile to code that always returns true.

**Fix options:**

* Require at least one match: `\s+`
* Anchor context: `^\s+$`
* Or alternatively, use `-Fb` flag

### Compilation Takes Too Long

Likely caused by unrestricted wildcards (`.*`, `.+`). Fix with:

* Negated classes (`[^)]*`)
* Bounded repeats (`{0,50}`)
* Pattern splitting
