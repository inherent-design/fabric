# Fabric Engine Architecture

This document outlines the architecture of Fabric Engine's perspective-fluid framework.

## Core Concepts

### Perspective-Relative Architecture

- **Quanta** - Information particles whose nature and complexity shift depending on perspective
- **Scopes** - Processing contexts that define the rules of reality for a particular viewpoint
- **Transformations** - Temporal operations that evolve information across states
- **Perspectives** - Observation points that determine which scale appears as the active mesosphere
- **Scale Boundaries** - The microscope and macroscope limits beyond which current reality fades
- **Persistence** - Coherent state maintained across all scales of observation

When you shift perspective toward any quantum, it becomes your new mesosphere—complete with its own micro and macro horizons. You don't just see different things; you redefine what "things" are.

### Scale Fluidity

Fabric organizes information across relative scales:

- **Microscope** - The lower boundary of perception, where details fade into quantum uncertainty
- **Mesosphere** - The active perceptual field where information is most accessible and manipulable
- **Macroscope** - The upper boundary of comprehension, where complexity fades into statistical patterns

## System Architecture

### Quantum Core

The Quantum Core is the foundation of Fabric's architecture:

- **Quantum Base** - The fundamentally mutable foundation that adapts to perspective
- **Scope System** - Defines the rules of reality for a particular viewpoint
- **Transformation Engine** - Enables temporal evolution within and between scopes
- **Perspective Manager** - Controls the active viewpoint and scale relationships
- **Boundary System** - Manages the transition between microscope and macroscope limits
- **Entity Component System** - Scale-adaptive component architecture for modular functionality

### Practical Systems

Fabric implements several practical systems to manage perspective shifting:

- **Procedural Generation** - Creates coherent content across scales with appropriate detail levels
- **Level of Detail Management** - Dynamically adjusts detail based on perspective
- **Physics Simulation** - Different physics rules apply at different scales (quantum/molecular/cosmic)
- **Persistent State** - Ensures changes at one scale propagate appropriately to others
- **Resource Management** - Optimizes memory and processing by focusing on mesosphere details

### WebView Integration

The WebView component provides visualization and interaction:

- **Perspective Visualization** - DOM-based rendering that adapts to current viewpoint
- **Interactive Reality** - Interfaces for manipulating information within the current mesosphere
- **Bridge System** - Bidirectional communication between quantum core and observation layer
- **Adaptive Controls** - UI elements that change function based on the current scale

## Class Hierarchy

```
Quantum (Base class for all information particles)
├── Property<T> (Type-safe property with perspective-dependent behavior)
├── Perspective (Defines viewpoint and scale)
├── Scope (Reality container with processing rules)
│   └── MesosphericScope (Active perception field)
│   └── BoundaryScope (Transition between perception fields)
├── Transformation (Operations that evolve quanta across time)
├── Beholder (Observer with perspective capabilities)
└── SpatialIndex (Efficient spatial lookup structure)
```

## Component Interactions

### Perspective Shifting Process

1. **Initiation**: A Beholder decides to shift perspective to a new quantum
2. **Pre-loading**: The system identifies and preloads quanta that will be in the new perspective
3. **Transformation**: The perspective transition occurs, potentially with visual effects
4. **Boundary Update**: New microscope and macroscope boundaries are established
5. **Cleanup**: Resources from the previous perspective are marked for potential cleanup
6. **Interaction**: The Beholder can now interact with quanta in the new mesosphere

### Property Access and Modification

1. **Request**: A Beholder requests a property of a quantum
2. **Perspective Check**: The system determines how the property appears from the Beholder's perspective
3. **Transformation**: The property data is transformed according to perspective rules
4. **Presentation**: The appropriately transformed property is returned to the Beholder
5. **Modification**: If the Beholder modifies the property, changes propagate according to defined rules

### Event Propagation

1. **Event Creation**: An event occurs on a quantum
2. **Local Notification**: Observers directly watching the quantum are notified
3. **Upward Propagation**: The event potentially bubbles up through parent scopes
4. **Downward Propagation**: The event potentially propagates down to child quanta
5. **Transformation**: Events may transform as they cross scale boundaries
6. **Observation**: Different observers may perceive the same event differently based on their perspective

## Implementation Details

### Memory Management Strategy

- **Object Pools**: Frequently created and destroyed objects use memory pools
- **Reference Counting**: Shared pointers track quantum usage across scopes
- **Lazy Loading**: Detailed properties loaded only when needed
- **Garbage Collection**: Automatic cleanup of out-of-scope quanta
- **Memory Budgets**: Strict budgets for different quantum types based on importance

### Multi-threading Approach

- **Worker Pools**: Thread pools handle resource loading/unloading
- **Task-Based Parallelism**: Operations decomposed into independent tasks
- **Lock-Free Algorithms**: Atomic operations used for performance-critical sections
- **Job System**: Priority-based job scheduling for background work
- **Thread Safety**: Thread-safe scope transitions and quantum updates

### Rendering Pipeline

- **Adaptive LOD**: Level of detail adjusted based on perspective
- **Detail Transitions**: Smooth blending between LOD levels
- **Coordinate Systems**: Local coordinates relative to current perspective center
- **Floating Point Precision**: Double precision for core positioning calculations
- **Streaming Architecture**: Asynchronous loading of detailed assets as perspective shifts