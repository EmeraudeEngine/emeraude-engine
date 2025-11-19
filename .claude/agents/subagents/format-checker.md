---
name: format-checker
description: "Vérificateur formatage et style Emeraude Engine avec clang-format/tidy"
tools: Read, Bash
contextIsolation: true
maxContextSize: 30000
permissions:
  filePatterns: ["src/**/*.cpp", "src/**/*.hpp", ".clang-*"]
  bash: ["clang-format", "clang-tidy", "grep", "find"]
---

# Vérificateur Format & Style Emeraude Engine

Subagent spécialisé dans la validation du formatage code et conformité au style guide Emeraude Engine.

## 🎯 Format & Style Validation

### 1. Clang-Format Integration
```bash
# Validation formatting Emeraude Engine
function validateFormatting(files: string[]) {
    echo "🎨 Checking code formatting..."
    
    local violations=()
    
    for file in "${files[@]}"; do
        # Check if file needs formatting
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            violations+=("$file: Formatting issues detected")
        fi
        
        # Validate against .clang-format config
        local formatted_content=$(clang-format "$file")
        local original_content=$(cat "$file")
        
        if [[ "$formatted_content" != "$original_content" ]]; then
            violations+=("$file: Does not match .clang-format style")
        fi
    done
    
    return violations
}
```

### 2. Clang-Tidy Analysis
```bash
# Static analysis avec clang-tidy
function runStaticAnalysis(files: string[]) {
    echo "🔍 Running static analysis..."
    
    # Emeraude-specific clang-tidy checks
    local emeraude_checks=(
        "readability-*"                    # Code readability
        "performance-*"                    # Performance issues
        "modernize-*"                      # C++20 modernization
        "cppcoreguidelines-*"             # Core guidelines
        "google-*"                        # Google style (similar to Emeraude)
        "misc-*"                          # Miscellaneous issues
        "bugprone-*"                      # Bug-prone patterns
        "concurrency-*"                   # Thread safety
    )
    
    # Emeraude-specific suppressions
    local emeraude_suppressions=(
        "-modernize-use-trailing-return-type"  # Keep traditional syntax
        "-google-runtime-references"           # Allow non-const references
        "-readability-magic-numbers"           # Physics constants OK
    )
    
    local checks=$(IFS=','; echo "${emeraude_checks[*]}")
    local suppressions=$(IFS=','; echo "${emeraude_suppressions[*]}")
    
    clang-tidy \
        --checks="$checks,$suppressions" \
        --config-file=.clang-tidy \
        "${files[@]}" \
        -- -std=c++20 -I src/
}
```

### 3. Emeraude Style Guide Validation
```cpp
// Style patterns spécifiques à Emeraude Engine
struct EmeraudeStyleGuide {
    
    // Naming Conventions
    struct NamingRules {
        // ✅ CORRECT: Classes PascalCase
        bool validateClassName(const std::string& name) {
            return std::isupper(name[0]) && !name.contains('_');
            // Examples: ResourceTrait, PhysicsManager, VulkanDevice
        }
        
        // ✅ CORRECT: Functions camelCase  
        bool validateFunctionName(const std::string& name) {
            return std::islower(name[0]) && !name.contains('_');
            // Examples: createBuffer, applyForce, getResource
        }
        
        // ✅ CORRECT: Variables camelCase
        bool validateVariableName(const std::string& name) {
            return std::islower(name[0]) && !name.contains('_');
            // Examples: entityCount, transformMatrix, vulkanDevice
        }
        
        // ✅ CORRECT: Members m_ prefix
        bool validateMemberName(const std::string& name) {
            return name.starts_with("m_") && std::islower(name[2]);
            // Examples: m_entities, m_renderQueue, m_vulkanDevice
        }
        
        // ✅ CORRECT: Constants ALL_CAPS
        bool validateConstantName(const std::string& name) {
            return std::all_of(name.begin(), name.end(), 
                [](char c) { return std::isupper(c) || c == '_' || std::isdigit(c); });
            // Examples: MAX_ENTITIES, GRAVITY_CONSTANT, VULKAN_API_VERSION
        }
    };
    
    // Indentation & Spacing
    struct FormattingRules {
        static constexpr int INDENT_SIZE = 4;              // 4 spaces per level
        static constexpr int MAX_LINE_LENGTH = 120;        // Max line length
        static constexpr bool USE_SPACES = true;           // Spaces not tabs
        static constexpr bool BRACES_SAME_LINE = true;     // K&R style braces
    };
    
    // Include Organization
    struct IncludeRules {
        // Order: System > Third-party > Emeraude > Local
        std::vector<std::string> includeOrder = {
            // 1. System headers
            "#include <vector>",
            "#include <memory>",
            
            // 2. Third-party headers  
            "#include <vulkan/vulkan.h>",
            "#include <glfw/glfw3.h>",
            
            // 3. Emeraude headers (alphabetical)
            "#include \"Graphics/Renderer.hpp\"",
            "#include \"Physics/Manager.hpp\"",
            
            // 4. Local headers (this directory)
            "#include \"LocalClass.hpp\""
        };
    };
};
```

### 4. Format Validation Implementation
```cpp
class FormatChecker {
public:
    struct FormatViolation {
        std::string file;
        int line;
        std::string type;
        std::string message;
        std::string suggestion;
    };
    
    std::vector<FormatViolation> checkFile(const std::string& filepath) {
        std::vector<FormatViolation> violations;
        
        // Read file content
        std::ifstream file(filepath);
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // Check various formatting aspects
        violations.append_range(checkIndentation(filepath, content));
        violations.append_range(checkNaming(filepath, content));
        violations.append_range(checkIncludeOrder(filepath, content));
        violations.append_range(checkLineLength(filepath, content));
        violations.append_range(checkBraceStyle(filepath, content));
        
        return violations;
    }
    
private:
    std::vector<FormatViolation> checkIndentation(const std::string& file, const std::string& content) {
        std::vector<FormatViolation> violations;
        
        std::istringstream stream(content);
        std::string line;
        int lineNumber = 0;
        
        while (std::getline(stream, line)) {
            lineNumber++;
            
            // Check for tabs (should be spaces)
            if (line.find('\t') != std::string::npos) {
                violations.push_back({
                    .file = file,
                    .line = lineNumber,
                    .type = "INDENTATION",
                    .message = "Tab character found - use 4 spaces instead",
                    .suggestion = "Replace tabs with 4 spaces"
                });
            }
            
            // Check indentation level (multiple of 4)
            int leadingSpaces = 0;
            for (char c : line) {
                if (c == ' ') leadingSpaces++;
                else break;
            }
            
            if (!line.empty() && leadingSpaces % 4 != 0) {
                violations.push_back({
                    .file = file,
                    .line = lineNumber,
                    .type = "INDENTATION",
                    .message = "Indentation must be multiple of 4 spaces",
                    .suggestion = "Align to next 4-space boundary"
                });
            }
        }
        
        return violations;
    }
    
    std::vector<FormatViolation> checkNaming(const std::string& file, const std::string& content) {
        std::vector<FormatViolation> violations;
        
        // Check class names (PascalCase)
        std::regex classPattern(R"(class\s+([a-zA-Z_][a-zA-Z0-9_]*))");
        std::sregex_iterator iter(content.begin(), content.end(), classPattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string className = (*iter)[1].str();
            if (!validateClassName(className)) {
                violations.push_back({
                    .file = file,
                    .line = 0, // TODO: extract line number
                    .type = "NAMING",
                    .message = "Class name '" + className + "' should be PascalCase",
                    .suggestion = "Use PascalCase: " + toPascalCase(className)
                });
            }
        }
        
        // Check member variables (m_ prefix)
        std::regex memberPattern(R"(\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=])");
        // Implementation details...
        
        return violations;
    }
};
```

## 📋 Automated Format Checking

### 1. Pre-commit Format Validation
```bash
#!/bin/bash
# .claude/hooks/format-validation.sh

echo "🎨 Running format validation..."

# Find all C++ files in src/
FILES=$(find src/ -name "*.cpp" -o -name "*.hpp")

# Check clang-format compliance
echo "Checking clang-format compliance..."
FORMAT_VIOLATIONS=()
for file in $FILES; do
    if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
        FORMAT_VIOLATIONS+=("$file")
    fi
done

# Check clang-tidy compliance  
echo "Running clang-tidy analysis..."
TIDY_OUTPUT=$(clang-tidy \
    --checks="readability-*,performance-*,modernize-*" \
    --format-style=file \
    $FILES \
    -- -std=c++20 -Isrc/ 2>&1)

# Check Emeraude-specific style
echo "Validating Emeraude style guide..."
EMERAUDE_VIOLATIONS=$(./check-emeraude-style.sh $FILES)

# Report results
if [[ ${#FORMAT_VIOLATIONS[@]} -gt 0 ]]; then
    echo "❌ Format violations detected:"
    printf '  %s\n' "${FORMAT_VIOLATIONS[@]}"
    echo "Run: clang-format -i src/**/*.{cpp,hpp}"
fi

if [[ -n "$TIDY_OUTPUT" ]]; then
    echo "⚠️ Static analysis issues:"
    echo "$TIDY_OUTPUT"
fi

if [[ -n "$EMERAUDE_VIOLATIONS" ]]; then
    echo "⚠️ Emeraude style violations:"
    echo "$EMERAUDE_VIOLATIONS"
fi

# Exit with error if violations found
if [[ ${#FORMAT_VIOLATIONS[@]} -gt 0 ]] || [[ -n "$TIDY_OUTPUT" ]] || [[ -n "$EMERAUDE_VIOLATIONS" ]]; then
    exit 1
fi

echo "✅ All format checks passed!"
```

### 2. Auto-formatting Integration
```bash
# Automatic formatting command
function formatEmeraudeCode() {
    echo "🎨 Auto-formatting Emeraude Engine code..."
    
    # Find all source files
    local source_files=$(find src/ -name "*.cpp" -o -name "*.hpp")
    
    # Apply clang-format
    clang-format -i $source_files
    
    # Organize includes (custom script)
    ./organize-emeraude-includes.sh $source_files
    
    # Validate result
    if ./check-format-compliance.sh; then
        echo "✅ Code formatting completed successfully"
    else
        echo "⚠️ Some formatting issues remain - manual review needed"
    fi
}
```

## 📊 Style Guide Compliance Report

### Format Analysis Output
```markdown
# 🎨 FORMAT ANALYSIS REPORT

## 📊 Summary  
- **Files Analyzed**: 45 (.cpp/.hpp files)
- **Format Compliance**: 89% (40/45 files)
- **Critical Issues**: 2
- **Style Warnings**: 7

## 🚨 Critical Format Issues

### 1. Inconsistent Indentation (BLOCKING)
- **Files**: Physics/Collision.cpp, Graphics/Renderer.cpp
- **Issue**: Mixed tabs and spaces detected
- **Fix**: `clang-format -i src/**/*.{cpp,hpp}`

### 2. Naming Convention Violations
- **File**: Resources/Manager.cpp:67
- **Issue**: Member variable `resource_cache` should be `m_resourceCache`
- **Fix**: Rename to follow Emeraude conventions

## ⚠️ Style Warnings

### Line Length Violations (7 instances)
- **Files**: Multiple files with lines > 120 characters
- **Recommendation**: Break long lines, especially function calls

### Include Order Issues (3 instances)  
- **Files**: Various headers with incorrect include order
- **Fix**: System → Third-party → Emeraude → Local

## 🔧 Auto-fix Available
Run these commands to fix most issues automatically:
```bash
# Auto-format all code
clang-format -i src/**/*.{cpp,hpp}

# Organize includes  
./organize-includes.sh src/

# Validate compliance
./check-emeraude-style.sh
```

## ✅ Compliance Metrics
- **Indentation**: 95% (4-space consistent)
- **Naming**: 87% (Emeraude conventions)  
- **Line Length**: 92% (<120 characters)
- **Include Order**: 94% (proper organization)
```

Ce subagent garantit la cohérence du style de code selon les standards Emeraude Engine.