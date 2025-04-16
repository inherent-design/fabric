# CLAUDE.md

Guidance for Claude Code when working with this repository.

## Essentials
```bash
# Build
cmake -G "Ninja" -B build && cmake --build build

# Test
./build/bin/UnitTests  # or IntegrationTests, E2ETests
./build/bin/UnitTests --gtest_filter=TestName --gtest_timeout=30
```

## Code Style & Organization
- Files: `.hh` headers, `.cc` implementations
- Naming: `PascalCase` classes, `camelCase` methods, `ALL_CAPS` constants
- Structure: Core (`include/fabric/core/`, `src/core/`), Utils (`include/fabric/utils/`), mirrored Tests

## Critical Patterns

### Error Handling & Logging
- Use `throwError()` not direct throws; derive from `FabricException`
- Always use `Logger::log*()` methods (never iostream):
  - `logDebug` → diagnostics, `logInfo` → normal operations
  - `logWarning` → recoverable issues, `logError` → functionality blockers
  - `logCritical` → operation blockers

### Concurrency
- Follow intent-based locking with explicit timeouts
- Specify intent: `Read`, `NodeModify`, or `GraphStructure`
- Lock hierarchy: graph locks → node locks
- Release locks ASAP; copy data to minimize lock duration

### Breaking Circular Dependencies
- Forward declare in base class; implement in separate helper
- Example: `Resource.hh` → `ResourceHelpers.hh` ← `ResourceHub.hh`

## Testing Best Practices
- Structure: Clear setup, isolated execution, explicit verification, predictable cleanup
- Cover: Happy path, failure paths, edge cases, thread safety, memory management
- Avoid: Stub tests, missing assertions, global state, hardcoded data, magic sleeps
- Thread Safety: Deterministic fixtures, timeouts, high contention scenarios

## Philosophical Approach

### Architecture & Understanding
- Prioritize clean architecture over quick fixes
- Read more, work less - deep understanding leads to better solutions
- Ask clarifying questions; questions can be better than answers

### Balancing Ideals with Pragmatism
- Circular dependencies: reorganize when practical, tolerate when necessary
- De-dupe when possible without extreme performance loss
- Simple, clear code often beats clever, complex optimization
- Correctness trumps chunking; find the root to understand how leaves grow

### Development Mindset
- Surface tensions explicitly; document the "why" behind decisions
- Consider both components and their interactions within the larger system
- Balance immediate implementation with long-term architecture
