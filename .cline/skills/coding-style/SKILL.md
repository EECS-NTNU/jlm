---
name: coding-style
description: JLM compiler C++ coding style and formatting. Use when writing, editing, or reviewing C++ code to ensure compliance with project standards using clang-format and clang-tidy.
---

## Quick Cheat Sheet
- Format all source files: `make format`
- Dry‑run formatting check: `make format-dry-run`
- Run static analysis: `make tidy`

-------

This skill provides guidance on the JLM compiler's C++ coding style and formatting standards. The project uses **clang-format** for automatic code formatting and **clang-tidy** for static analysis.

## Quick Reference

### Formatting Commands

```bash
# Format all source files in-place
make format

# Dry-run to check if formatting is needed (without modifying)
make format-dry-run

# Run clang-tidy checks (read-only, shows issues)
make tidy
```

### Tool Versions

The project uses **LLVM 18** tools:
- `clang-format-18` for code formatting
- `clang-tidy-18` for static analysis

---

## clang-format Configuration

The `.clang-format` file defines the coding style. Key rules:

### Basic Style

| Setting | Value | Description |
|---------|-------|-------------|
| Standard | C++Latest | Use latest C++ features |
| ColumnLimit | 100 | Max line length |
| IndentWidth | 2 | Spaces per indent level |
| UseTab | Never | Always use spaces |

|---------|-------|-------------|
| Standard | C++Latest | Use latest C++ features |
| PointerAlignment | Middle | `int * a` and `int & a` |
| ReferenceAlignment | Pointer | Align references like pointers |
| QualifierAlignment | Leave | Don't change `const int *` vs `int const *` |
| ColumnLimit | 100 | Max line length |

### Indentation

| Setting | Value | Description |
|---------|-------|-------------|
| IndentWidth | 2 | Use 2 spaces per indent level |
| TabWidth | 2 | Display tabs as 2 spaces |
| UseTab | Never | Always use spaces |
| ConstructorInitializerIndentWidth | 4 | Align constructor initializers |
| ContinuationIndentWidth | 4 | Indent continuation lines |

### Spacing Rules

| Setting | Value | Example |
|---------|-------|---------|
| SpaceAroundPointerQualifiers | Both | `int const * a` |
| SpaceAfterTemplateKeyword | false | `template<typename T>` |
| SpaceBeforeParens | ControlStatements | `if (...)` but `func()` |
| SpaceBeforeRangeBasedForLoopColon | true | `for (auto it : list)` |
| SpacesInAngles | Never | `vector<int>` not `vector< int >` |

### Brace Wrapping

The project uses custom brace wrapping:

```cpp
// After control statements - ALWAYS wrap
if (condition)
{
    // body
}

// After function definition - ALWAYS wrap
int
MyFunction()
{
    return 0;
}

// Before else/catch - ALWAYS wrap
else
{
    // handle
}
```

Full `BraceWrapping` settings:
- `AfterCaseLabel: true`
- `AfterClass: true`
- `AfterControlStatement: Always`
- `AfterEnum: true`
- `AfterFunction: true`
- `BeforeCatch: true`
- `BeforeElse: true`
- `BeforeLambdaBody: true`

### Function Definitions

```cpp
// Return type on separate line, function name on its own line
int
MyFunction(
    int arg1,
    const std::string & arg2)
{
    return 0;
}

// Empty functions can have {} body (not split)
void
SimpleFunction()
{}
```

---

## clang-tidy Checks

The `.clang-tidy` file enforces additional style rules. Key checks:

| Category | Check | Description |
|----------|-------|-------------|
| Bugprone | `move-forwarding-reference` | Avoid perfect forwarding issues |
| CppCoreGuidelines | `init-variables` | Initialize variables on declaration |
| CppCoreGuidelines | `pro-type-member-init` | Initialize all class members |
| Misc | `unused-parameters` | Remove unused parameters |
| Misc | `unused-using-decls` | Remove unused using declarations |
| Modernize | `deprecated-headers` | Use modern headers |
| Modernize | `redundant-void-arg` | Use `()` not `(void)` for no args |
| Modernize | `use-override` | Use `override` keyword |
| Readability | `identifier-naming` | Class names in CamelCase |

---

## Code Style Examples

### File Header

```cpp
/*
 * Copyright 2025 Your Name <email@example.com>
 * See COPYING for terms of redistribution.
 */
```

### Include Order

```cpp
#include <jlm/llvm/ir/RvsdgModule.hpp>    // JLM project headers first
#include <jlm/rvsdg/graph.hpp>

#include <vector>                           // Standard library headers
#include <string>
```

### Namespaces

```cpp
// No indentation inside namespaces (NamespaceIndentation: None)
namespace jlm::llvm
{

void MyFunction()
{
  // code here
}

}  // namespace jlm::llvm
```

### Class Definitions

```cpp
class MyClass : public BaseClass
{
public:
  ~MyClass() noexcept override;

  int
  GetValue() const
  {
    return value_;
  }

protected:
  void
  SetValue(int v)
  {
    value_ = v;
  }

private:
  int value_;
};
```

### Function Definitions

```cpp
// Return type on separate line
std::unique_ptr<MyClass>
CreateMyClass()
{
  return std::make_unique<MyClass>();
}

// Parameters may break to new lines if needed
void
ProcessData(
    const std::vector<int> & data,
    int threshold,
    bool verbose)
{
  // implementation
}
```

### Lambda Functions

```cpp
auto lambda = [](int x, int y) -> int
{
  return x + y;
};
```

---

## Workflow

### Writing New Code

1. Write your C++ code following the style guidelines below
2. Run `make format` to auto-format
3. Run `make tidy` to check for static analysis issues
4. Fix any clang-tidy warnings

### Editing Existing Code

1. If modifying a file, run `make format` on it first (or just use your editor's clang-format integration)
2. Ensure new code matches the surrounding style
3. Use `clang-format-18 --style=file:.clang-format -i path/to/file.cpp` for single files

### Common Patterns

#### Smart Pointers

```cpp
// Use make_unique/make_shared
auto ptr = std::make_unique<MyClass>();

// Return unique_ptr by value
std::unique_ptr<MyClass> Create();

// Return shared_ptr by value  
std::shared_ptr<MyClass> GetShared();
```

#### References vs Pointers

```cpp
// Prefer references for non-null parameters
void Process(const MyClass & obj);

// Use pointers when nullable
void SetOptional(MyClass * ptr);
```

#### Const Correctness

```cpp
// Const references for read-only access
void Print(const std::string & text) const;

// Const return values when appropriate
const int & GetValue() const;
```

---

## Integration with Editor

### VS Code Setup

Add to `.vscode/settings.json`:

```json
{
  "clang-format.executable": "clang-format-18",
  "clang-format.style": "file",
  "cpp.clangFormatStyle": "file"
}
```

### Format on Save

```json
{
  "[cpp]": {
    "editor.formatOnSave": true
  }
}
```

---

## SEE ALSO

- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **HLS SKILL**: [HLS mode details](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)

## Related Files

| File | Purpose |
|------|---------|
| `.clang-format` | clang-format configuration |
| `.clang-tidy` | clang-tidy check definitions |
| `Makefile.rules` | format/tidy make targets |