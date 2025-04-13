# Fabric Engine Examples

This document contains code examples demonstrating core Fabric Engine concepts and implementations.

## Table of Contents
- [Fabric Engine Examples](#fabric-engine-examples)
    - [Table of Contents](#table-of-contents)
    - [Basic Quantum Structure](#basic-quantum-structure)
    - [Perspective and Observation](#perspective-and-observation)
    - [Nuclear Decay Simulation](#nuclear-decay-simulation)
    - [Scale-Adaptive Systems](#scale-adaptive-systems)

## Basic Quantum Structure

The following example shows how to create basic quanta (atoms) and composite structures (walls):

```cpp
// Define a basic Quantum (Atom)
class Atom : public Quantum {
public:
    Atom(const std::string& name) : Quantum(name) {}

    // Properties that adapt based on perspective
    Property<Color> color;
    Property<float> energy;

    // Each Atom can recursively contain its own universe
    void addInternalStructure(const Quantum& substructure) {
        internal_structure.push_back(substructure);
    }
};

// Define a Wall as a collection of Atoms
class Wall : public Quantum {
public:
    Wall(const std::string& name) : Quantum(name) {}

    // Add atoms to the wall
    void addAtom(const Atom& atom) {
        atoms.push_back(atom);
    }

    // From a macroscopic perspective, a wall has different properties
    // than the sum of its atoms
    Property<float> strength;
    Property<Color> apparentColor;

private:
    std::vector<Atom> atoms;
};
```

## Perspective and Observation

This example demonstrates the Beholder concept (an observer with a specific perspective) and perspective shifting:

```cpp
// A Beholder represents an observer with a specific perspective
class Beholder {
public:
    Beholder(const std::string& name) : name_(name) {}

    // Set the current scope (mesosphere) this Beholder perceives
    void setScope(Scope* scope) {
        current_scope_ = scope;
        updateBoundaries();
    }

    // Shift perspective toward a quantum, making it the center
    // of the new mesosphere
    void shiftPerspectiveTo(Quantum* quantum) {
        // Create a new scope centered on this quantum
        Scope* new_scope = quantum->createScopeAround();

        // Preserve relationships from previous scope
        new_scope->inheritRelationshipsFrom(current_scope_);

        // Update the current scope
        setScope(new_scope);
    }

    // Zoom out to see the macroscopic view
    void shiftToMacroscope() {
        if (current_scope_->hasParent()) {
            setScope(current_scope_->getParent());
        }
    }
};

// Example of using a Beholder to observe reality from different perspectives
void perspectiveExample() {
    // Create a world with various structures
    auto world = createWorld();
    auto livingRoom = world.findQuantum("Living Room");
    auto wall = livingRoom.findQuantum("North Wall");

    // Create a Beholder (observer)
    Beholder observer("Human");

    // Start with a room-scale perspective
    observer.setScope(livingRoom.getScope());

    // The wall is seen as a solid object at this scale
    auto wallView = observer.perceive(wall);
    std::cout << "Wall color: " << wallView.getProperty("apparentColor") << std::endl;

    // Shift perspective to see the wall's atomic structure
    observer.shiftPerspectiveTo(wall);

    // Now individual atoms become visible in the mesosphere
    auto atoms = observer.perceiveAll("Atom");
    std::cout << "Number of atoms visible: " << atoms.size() << std::endl;
}
```

## Nuclear Decay Simulation

This example shows how the same physical process (nuclear decay) appears differently when observed from different perspectives:

```cpp
// Demo of using radioactive atoms with perspective-dependent observation
void radioactiveDecayExample() {
    // Create a radioactive material (e.g., uranium)
    auto uranium = new RadioactiveAtom("Uranium-235", 22.0f * 365.0f * 24.0f * 3600.0f);

    // Create different observers at different scales
    Beholder geiger_counter("Geiger Counter");      // Mesoscale observer
    Beholder radiation_monitor("Radiation Monitor"); // Microscale statistical observer
    Beholder human("Human");                        // Macroscale observer

    // Set up perspectives for each beholder
    geiger_counter.setScope(Scope::createAround(uranium, Perspective::Scale::Meso));
    radiation_monitor.setScope(Scope::createAround(uranium, Perspective::Scale::Micro));
    human.setScope(Scope::createAround(world.findQuantum("Room"), Perspective::Scale::Macro));

    // Simulate decay over time
    for (float time = 0.0f; time <= 10.0f; time += 1.0f) {
        // Apply decay
        uranium->decay(365.0f * 24.0f * 3600.0f); // Simulate 1 year per step

        // Observe from different perspectives
        auto geiger_observation = geiger_counter.perceive(uranium);
        auto monitor_observation = radiation_monitor.perceive(uranium);
        auto human_observation = human.perceive(uranium);

        // Each observer sees a completely different reality:
        // - Geiger counter: Individual particles (mesoscale)
        // - Radiation monitor: Poisson distribution (microscale statistical)
        // - Human: Only aggregate effects like temperature (macroscale)
    }

    // When the human shifts perspective to mesoscale...
    human.shiftPerspectiveTo(uranium);

    // ...they suddenly see individual particles that were previously imperceptible
    auto new_observation = human.perceive(uranium);

    // The human gains understanding by experiencing multiple perspectives
    std::cout << "By experiencing both macroscale and mesoscale perspectives,";
    std::cout << " the human now understands how individual particle behavior";
    std::cout << " leads to the aggregate effects previously observed." << std::endl;
}
```

## Scale-Adaptive Systems

This example demonstrates implementation of a system that handles perspective shifting while managing memory, level of detail, and physics simulations:

```cpp
// Demonstrates memory optimization and LOD management
class ScaleAdaptiveSystem {
public:
    ScaleAdaptiveSystem() {
        // Create spatial partitioning structure for efficient quanta access
        spatial_index_ = std::make_unique<OctreeIndex>(Vector3(0, 0, 0), 1000000.0f);

        // Configure performance parameters
        config_.max_active_quanta = 10000;          // Memory constraint
        config_.mesosphere_radius = 100.0f;         // Perception radius
        config_.lod_transition_duration = 0.5f;     // For smooth transitions
        config_.physics_update_frequency = 60.0f;   // Updates per second
        config_.async_loading_threads = 4;          // Parallel loading
    }

    // Handle shifting perspective while managing system resources
    void shiftPerspective(Beholder& beholder, Quantum* target) {
        // Track previous perspective for transition effects
        auto previous_scope = beholder.getCurrentScope();
        auto previous_center = previous_scope->getCentralQuantum();

        // Pre-transition: prepare data that will be needed
        preloadQuantaAround(target, config_.mesosphere_radius);

        // Update beholder's perspective (may be animated over time)
        beholder.shiftPerspectiveTo(target);

        // Post-transition: clean up resources that are now out of scope
        performGarbageCollection(previous_scope);

        // Update spatial indexing structures
        updateSpatialPartitioning(beholder.getCurrentScope());

        // Adjust physics simulation parameters for the new scale
        adjustPhysicsParameters(target->getScale());

        // Schedule LOD transitions for visual smoothness
        scheduleLODTransitions(previous_center, target);
    }

    // Preload data for quanta that will become relevant at new perspective
    void preloadQuantaAround(Quantum* center, float radius) {
        // Query spatial index for nearby quanta
        auto nearby_quanta = spatial_index_->queryRadius(
            center->getPosition(), radius);

        // Check if we have too many quanta to load
        if (nearby_quanta.size() > config_.max_active_quanta) {
            // Prioritize quanta based on importance or distance
            prioritizeQuanta(nearby_quanta);
        }

        // Asynchronously load detailed data for these quanta
        thread_pool_.enqueue([this, nearby_quanta]() {
            for (auto quantum : nearby_quanta) {
                // Skip if already fully loaded
                if (quantum->getDetailLevel() >= DetailLevel::Full) {
                    continue;
                }

                // Load quantum details based on expected detail level
                quantum->loadDetails();

                // Update memory usage tracking
                memory_tracker_.recordAllocation(quantum->getMemoryUsage());
            }
        });
    }

    // Clean up resources that are no longer needed
    void performGarbageCollection(Scope* old_scope) {
        // Get quanta that were in old scope but not in new scope
        auto out_of_scope_quanta = old_scope->getQuanta();

        // Mark these for potential cleanup
        for (auto quantum : out_of_scope_quanta) {
            // Don't collect if it's still visible in another active scope
            if (isVisibleInAnyActiveScope(quantum)) {
                continue;
            }

            // If memory pressure is high, unload details immediately
            if (memory_tracker_.isPressureHigh()) {
                quantum->unloadDetails();
                memory_tracker_.recordDeallocation(quantum->getMemoryUsage());
            }
            // Otherwise, mark for potential future collection
            else {
                garbage_collector_.markForCollection(quantum);
            }
        }
    }

    // Handle scale-specific physics simulation
    void adjustPhysicsParameters(float scale) {
        // Different physics systems active at different scales
        if (scale < 0.001f) {
            // Quantum scale physics
            physics_engine_.setGravityEnabled(false);
            physics_engine_.setQuantumEffectsEnabled(true);
            physics_engine_.setSimulationTimestep(1e-15f);
        }
        else if (scale < 1.0f) {
            // Molecular scale physics
            physics_engine_.setGravityEnabled(false);
            physics_engine_.setElectromagneticForcesEnabled(true);
            physics_engine_.setSimulationTimestep(1e-12f);
        }
        else if (scale < 1000.0f) {
            // Human scale physics
            physics_engine_.setGravityEnabled(true);
            physics_engine_.setGravityStrength(9.8f);
            physics_engine_.setSimulationTimestep(1/60.0f);
        }
        else if (scale < 1e9f) {
            // Planetary scale physics
            physics_engine_.setGravityEnabled(true);
            physics_engine_.setOrbitalMechanicsEnabled(true);
            physics_engine_.setSimulationTimestep(60.0f);
        }
        else {
            // Cosmic scale physics
            physics_engine_.setCosmicExpansionEnabled(true);
            physics_engine_.setSimulationTimestep(86400.0f * 30.0f); // 30 days
        }
    }

    // Schedule smooth transitions between detail levels
    void scheduleLODTransitions(Quantum* from, Quantum* to) {
        // Calculate transition parameters
        float distance = (from->getPosition() - to->getPosition()).length();
        float scale_change = std::abs(from->getScale() - to->getScale());

        // Calculate transition duration (longer for bigger changes)
        float duration = std::min(config_.lod_transition_duration * scale_change, 2.0f);

        // Create transition effect for each affected quantum
        auto affected_quanta = getAffectedQuanta(from, to);

        for (auto quantum : affected_quanta) {
            LODTransition transition;
            transition.quantum = quantum;
            transition.start_time = getCurrentTime();
            transition.duration = duration;
            transition.start_detail = quantum->getDetailLevel();
            transition.target_detail = calculateTargetDetailLevel(quantum, to);

            // Add to active transitions
            active_lod_transitions_.push_back(transition);
        }
    }

private:
    struct Config {
        size_t max_active_quanta;
        float mesosphere_radius;
        float lod_transition_duration;
        float physics_update_frequency;
        int async_loading_threads;
    };

    struct LODTransition {
        Quantum* quantum;
        float start_time;
        float duration;
        DetailLevel start_detail;
        DetailLevel target_detail;
    };

    Config config_;
    std::unique_ptr<OctreeIndex> spatial_index_;
    std::vector<LODTransition> active_lod_transitions_;
    PhysicsEngine physics_engine_;
    ThreadPool thread_pool_;
    MemoryTracker memory_tracker_;
    GarbageCollector garbage_collector_;

    // Implementation details omitted for brevity
};
```
