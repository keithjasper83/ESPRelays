---
name: esp32-monitor-compile
description: Monitor terminal output from build and compile commands, capture logs, identify errors and warnings, and summarise build results with likely causes and fixes. Use this skill whenever a build system such as make, cmake, ninja, gradle, cargo, go, or similar is executed.
---

# Build Monitor

## Purpose

Monitor build and compile output, identify failures, surface warnings, and provide a concise summary of the build status.

## When to use

Use this skill whenever:

- A build or compile command is executed.
- The user wants to monitor build progress.
- A compilation fails.
- Build warnings need summarising.
- The user wants the root cause of a failed build.

## Instructions

### 1. Execute the build

Run the requested build command while capturing both stdout and stderr into a log.

Example:

```bash
<build-command> 2>&1 | tee build.log
```

If timing information is available, record the elapsed time.

---

### 2. Analyse the output

Extract:

- compiler errors
- linker errors
- configuration errors
- warnings
- success indicators
- final build status

Look for common patterns such as:

- `error:`
- `fatal error:`
- `undefined reference`
- `warning:`
- `make: ***`
- `FAILED`
- `BUILD FAILED`
- `BUILD SUCCESS`
- `Build completed`

---

### 3. Determine the root cause

If multiple errors are present:

- locate the first real compiler or configuration error
- ignore cascading failures caused by that error
- identify the source file and line number where possible

---

### 4. Produce a summary

Use the following format.

## Build Result: SUCCESS | FAILED

**Command**

`<command>`

**Duration**

`<duration if known>`

### Errors (N)

- `<file>:<line> — <message>`

### Warnings (N)

- `<file>:<line> — <message>`

### Summary

Provide a concise explanation of the outcome.

If the build failed, explain the most likely root cause.

If an obvious fix exists, suggest it.

---

## Common situations

### make

If output contains

```
make: *** [target] Error 1
```

look earlier in the log for the compiler error that caused the failure.

---

### CMake

Distinguish between:

- configuration failures
- generation failures
- compile failures

---

### Parallel builds

If output from parallel jobs is interleaved and difficult to interpret, recommend rerunning with a single worker, for example:

```bash
make -j1
```

or

```bash
cmake --build . --parallel 1
```

---

### Warnings only

If the build succeeds but warnings are present:

- summarise them
- group similar warnings together
- highlight warnings likely to become future errors