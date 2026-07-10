# Design Document — Square Calculator

## Problem Statement

Write a C program that reads an integer and prints its square.

## Design

### Architecture

The program uses a simple single-function design:
- `main()`: Parses arguments, computes square, prints result

### Data Flow

```
Command-line Argument
    ↓
Parse using atoi()
    ↓
Compute: result = num * num
    ↓
Print result
    ↓
Exit
```

### Algorithm

1. Check if correct number of arguments (should be 2: program name + number)
2. Convert argument string to integer using `atoi()`
3. Compute square: `result = num * num`
4. Print result with `printf()`
5. Return 0 on success, 1 on error

### Code Style

- Uses Linux Kernel Coding Style
- Tabs for indentation (width 8)
- K&R brace style
- 80-character line limit
- snake_case for variables

## Test Strategy

Test three cases:
- **TC-01-01:** Positive number (5) → 25
- **TC-01-02:** Zero (0) → 0
- **TC-01-03:** Negative number (-3) → 9 (correct: (-3)² = 9)

Each test will verify the output matches expected result.
