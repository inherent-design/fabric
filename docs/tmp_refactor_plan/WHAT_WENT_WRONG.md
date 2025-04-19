## Understanding Fabric

### What Makes Fabric Unique?

Based on the project documentation, Fabric's core uniqueness lies in its **perspective fluidity**[cite: 16, 46]. Unlike traditional engines with fixed scales, Fabric aims to create a framework where the representation of information ("quanta") adapts based on the observer's viewpoint ("perspective")[cite: 16, 46]. This allows seamless transitions between vastly different scales, from subatomic to cosmic, where the rules and interactions within a given reality container ("scope") change dynamically[cite: 16, 46]. The goal is to enable applications where the fundamental nature of "things" is redefined based on how they are observed[cite: 16].

### How to Build and Develop Fabric

* **Building:**
    * Requires CMake (4.0+) and a C++20 compatible compiler[cite: 17, 31].
    * Platform-specific requirements are detailed in `docs/BUILD.md`, including necessary frameworks for macOS, SDKs/libraries for Windows, and packages for Linux[cite: 31].
    * Dependencies like SDL3, WebView, and Google Test are fetched automatically via CMake's FetchContent[cite: 15, 31].
    * The recommended build process uses Ninja for speed, optionally combined with CCache[cite: 31].
    * Standard CMake commands are used: `cmake -G "Ninja" -B build` and `cmake --build build`[cite: 18, 31].
* **Testing:**
    * Tests are divided into unit, integration, and end-to-end categories[cite: 17, 29].
    * Run tests using executables like `./build/bin/UnitTests`, `./build/bin/IntegrationTests`, etc.[cite: 18, 31].
    * Specific tests can be run using `--gtest_filter=TestName`[cite: 18].
    * **Crucially**: When running tests involving `ResourceHub`, worker threads **must be disabled** using `ResourceHub::instance().disableWorkerThreadsForTesting()` to prevent hangs[cite: 18, 50]. The `CLAUDE.md` file specifically calls this out[cite: 18].
* **Development Practices:**
    * Use `.hh` for headers and `.cc` for implementations[cite: 18].
    * Follow `PascalCase` for classes and `camelCase` for methods[cite: 18].
    * Employ `std::unique_ptr` and `std::shared_ptr` for memory management[cite: 18].
    * Use the provided `throwError()` function for exceptions and `Logger::log*()` methods for logging[cite: 18].
    * Adhere to concurrency best practices, especially intent-based locking with timeouts and the Copy-Then-Process pattern described in `CLAUDE.md` and `docs/guides/IMPLEMENTATION_PATTERNS.md`[cite: 18, 54].

### Marketing Fabric

The unique selling proposition is the **perspective fluidity** concept[cite: 16, 46]. Potential marketing angles include:

* **"Engine for Infinite Detail":** Highlight the ability to represent worlds/data at any scale.
* **"Contextual Reality Framework":** Emphasize how the engine adapts reality based on observation.
* **"Beyond Fixed Hierarchies":** Position it as a tool for breaking conventional information structures.
* **"Game Engine & Research Tool":** Target both game developers seeking novel mechanics and researchers needing multi-scale simulation/visualization tools[cite: 14].
* **Use Cases:** Showcase potential applications like multi-scale simulations (cosmic to quantum), procedural generation with infinite detail, complex data visualization, and novel interactive experiences[cite: 48].

## The Refactoring Decision

### Why Refactor? (The Problems)

The core reason you initiated the refactor appears to stem from **significant concurrency issues**, particularly within the resource management system (`ResourceHub`) and the associated tests[cite: 14, 18, 32, 50].

1.  **Concurrency Complexity & Deadlocks:**
    * The original `ResourceHub` implementation, while attempting sophisticated concurrency with intent-based locking (`CoordinatedGraph`), seems to have encountered deadlocks or race conditions[cite: 18, 32, 50]. This is explicitly mentioned in `CLAUDE.md` and the testing guides, which stress disabling worker threads and using timeouts to avoid hangs[cite: 18, 50].
    * The `CoordinatedGraph` itself, intended to prevent deadlocks, might have complexities in its lock hierarchy or intent propagation logic that were difficult to manage correctly[cite: 32, 43, 54]. Documents like `docs/guides/CONCURRENCY.md` detail the intended patterns (e.g., graph lock -> node lock), but implementation reality might have diverged[cite: 55].
    * The `ResourceHub::enforceMemoryBudget` function was noted as having potential race conditions and deadlocks, requiring workarounds like copying data before processing[cite: 24, 54].
2.  **Testing Challenges:**
    * Tests involving `ResourceHub` were prone to hanging, necessitating disabling worker threads for deterministic behavior[cite: 18, 50]. This indicates that the concurrent operations were not reliably testable.
    * The need for explicit timeouts in tests suggests the underlying locking mechanism wasn't robust against indefinite waits[cite: 18, 50].
    * `QuantumFluctuationIntegrationTest.cc` is marked as disabled due to numerous issues, highlighting the difficulty in integrating and testing these concurrent systems together[cite: 57].
3.  **Potential Architectural Issues (Leading to Refactor Plan):**
    * The refactoring plan (`docs/tmp_refactor_plan/REFACTORING_PLAN.md`, `UNIFIED_REFACTORING_PLAN.md`) mentions goals like breaking down monolithic classes, creating explicit APIs, removing implicit behaviors, and eliminating singletons[cite: 45, 47]. This suggests the original codebase might have suffered from tight coupling, unclear APIs, global state issues (perhaps `ResourceHub` as a singleton), and implicit side effects that made concurrency and testing difficult.
    * The plan emphasizes a "clean-break" approach, indicating the problems were deemed fundamental enough to warrant a rewrite rather than incremental fixes[cite: 45, 47].

In essence, while the *ideas* (perspective fluidity, quantum fluctuation concurrency model) were innovative, the *implementation* struggled with the complexities of safe and testable concurrency, particularly in the interaction between the graph-based resource system and its worker threads. Using AI pair programming tools without deep expertise in C++ memory management and concurrency likely contributed to subtle errors and inconsistencies[cite: 14].

## Next Steps

### a) Programming/Implementation

1.  **Follow the Refactor Plan:** Stick to the phased approach outlined in `docs/tmp_refactor_plan/UNIFIED_REFACTORING_PLAN.md` and `IMPLEMENTATION_ROADMAP.md`[cite: 47, 51]. Prioritize building the core foundations (error handling, types, logging, basic concurrency utils) before tackling more complex systems.
2.  **Concurrency Primitives First:** Ensure the new `ThreadSafe<T>`, task system, and `ThreadSafeQueue` implementations (Phase 4 of the plan) are robust and thoroughly tested before building systems upon them[cite: 45]. Implement and rigorously test the `TimeoutLock` utility.
3.  **Resource System Rebuild:** Reimplement the resource management system (`ResourceHub`, `ResourceLoader`, `ResourceDependencyManager`, etc.) with the new, simplified concurrency primitives[cite: 45, 49]. Focus on clear ownership and explicit state transitions. The `CoordinatedGraph` might need simplification or replacement based on the new concurrency patterns.
4.  **Test-Driven Development (TDD):** Adhere strictly to TDD as planned[cite: 45, 47]. Write tests *before* implementing features, especially for concurrent components. Use the testing best practices identified (`docs/tmp_refactor_plan/TESTING_BEST_PRACTICES.md`, `docs/guides/RESOURCE_HUB_TESTING.md`)[cite: 50, 53].
5.  **Incremental Integration:** Integrate systems step-by-step, testing interactions thoroughly at each stage (e.g., Resource System + Event System, then add Rendering).

### b) Architectural Design and Planning

1.  **Solidify Core Interfaces:** Ensure the interfaces defined in Phase 1 (`docs/tmp_refactor_plan/REFACTORING_PLAN.md`) are clean, explicit, and minimize dependencies[cite: 45].
2.  **Refine Concurrency Model:** Based on the issues, re-evaluate the "Quantum Fluctuation" model[cite: 43]. Perhaps a simpler, more standard approach (e.g., task-based parallelism with message queues, careful use of atomics, standard mutexes with clear lock ordering) might be more manageable initially than the complex intent/awareness propagation system. Focus on clear lock hierarchies and deadlock prevention as outlined in `docs/guides/CONCURRENCY.md`[cite: 55].
3.  **Dependency Injection:** Eliminate singletons (like the potentially problematic `ResourceHub::instance()`) and use dependency injection as planned to improve testability and reduce coupling[cite: 45, 47].
4.  **State Management:** Clearly define ownership and lifecycle management for all components, using RAII and smart pointers consistently[cite: 18, 45, 47].
5.  **Error Handling:** Consistently use the planned `Result<T>` pattern instead of exceptions for predictable control flow, especially in concurrent code[cite: 45, 47].

### c) Marketing and Business Strategies

1.  **Refine the Vision:** Clearly articulate the "perspective fluidity" concept and its benefits using examples from `docs/tmp_refactor_plan/USE_CASES.md`[cite: 48]. Create compelling demos.
2.  **Target Niche First:** Focus initial marketing on domains where multi-scale visualization/simulation is crucial (e.g., scientific research, complex system modeling, specific game genres) before broadening the appeal[cite: 48].
3.  **Seek Seed Funding/Grants:** Target funding sources interested in simulation, scientific computing, or novel game engine technology. Use the architectural plans [cite: 46] and roadmap [cite: 51] to showcase a clear vision and execution plan.
4.  **Build a Small Core Team:** Focus on hiring 1-2 experienced C++ engineers with strong backgrounds in concurrency, systems architecture, and ideally, graphics/game engines to solidify the core before scaling.
5.  **Open Source Strategy (Consider):** An open-source model could attract contributors and build a community, potentially offsetting the need for a large initial team. However, this requires careful planning regarding licensing and contribution management.
6.  **Develop Partnerships:** Collaborate with research institutions or game studios interested in exploring the multi-scale paradigm.

The core challenge was managing the complexity of the advanced concurrency model. The refactoring plan correctly identifies the need for simplification, explicit APIs, and rigorous testing. Focusing on these foundational aspects should provide a more stable base for realizing Fabric's unique vision.
