## Resources-Specific Rules

### MANDATORY Fail-Safe Philosophy
- **NEVER** return nullptr from Containers
- **ALWAYS** provide a valid resource (real or neutral)
- **NEVER** require error checking on client side
- Errors are logged but never break the application

### Neutral Resource Pattern
- **MANDATORY**: Implement `load()` (no parameters) for neutral/default resources
- Neutral resource must ALWAYS succeed (no I/O)
- Be immediately usable and visually identifiable
- No external dependencies
- ServiceProvider is available via `this->serviceProvider()` if needed

### Thread Safety (CRITICAL)

**Atomic status:**
```cpp
std::atomic<Status> m_status{Status::Unloaded};  // Lock-free queries
```

**Mutex for lists:**
```cpp
std::mutex m_dependenciesAccess;  // Protects m_parentsToNotify and m_dependenciesToWaitFor
```

**Two-phase pattern (avoids deadlocks):**
```cpp
void checkDependencies() noexcept {
    Action action = Action::None;
    {
        std::lock_guard lock{m_dependenciesAccess};
        // Phase 1: Determine action under lock
        if (allDependenciesLoaded()) action = Action::CallOnDependenciesLoaded;
    }
    // Phase 2: Execute OUTSIDE lock (virtual calls + notifications)
    if (action == Action::CallOnDependenciesLoaded) {
        this->onDependenciesLoaded();  // Virtual call OUTSIDE lock!
    }
}
```

### Cycle Detection (NEW v0.8.35)

**Automatic in addDependency():**
```cpp
if (this->wouldCreateCycle(dependency)) [[unlikely]] {
    m_status = Status::Failed;
    return false;
}
```

**Recursive DFS algorithm:**
```cpp
bool wouldCreateCycle(const shared_ptr<ResourceTrait>& dep) const noexcept {
    if (dep.get() == this) return true;  // Self-reference
    for (const auto& sub : dep->m_dependenciesToWaitFor) {
        if (sub.get() == this || this->wouldCreateCycle(sub)) return true;
    }
    return false;
}
```

### Dependency Management
- Use `addDependency()` to declare dependencies
- `onDependenciesLoaded()` for finalization (GPU upload, etc.)
- Automatic parent-child event propagation
- Reference counting with `std::shared_ptr`
