# Quantum Fluctuation Update - Design Document

[← Back to Documentation Index](DOCUMENTATION.md)

## Table of Contents
- [Overview](#overview)
- [1. Command Pattern & Action System](#1-command-pattern--action-system)
  - [Class Diagram](#class-diagram)
  - [Key Features](#key-features)
  - [Implementation Approach](#implementation-approach)
- [2. Reactive Programming Model](#2-reactive-programming-model)
  - [Class Diagram](#class-diagram-1)
  - [Key Features](#key-features-1)
  - [Implementation Approach](#implementation-approach-1)
- [3. Resource Management Framework](#3-resource-management-framework)
  - [Class Diagram](#class-diagram-2)
  - [Key Features](#key-features-2)
  - [Implementation Approach](#implementation-approach-2)
- [4. Spatial Primitives & Scene Graph](#4-spatial-primitives--scene-graph)
  - [Class Diagram](#class-diagram-3)
  - [Key Features](#key-features-3)
  - [Implementation Approach](#implementation-approach-3)
- [5. Temporal Dimension System](#5-temporal-dimension-system)
  - [Class Diagram](#class-diagram-4)
  - [Key Features](#key-features-4)
  - [Implementation Approach](#implementation-approach-4)
- [Integration Strategy](#integration-strategy)
- [Next Steps](#next-steps)

## Overview

The Quantum Fluctuation update introduces five foundational systems that will unlock Fabric's potential for perspective-fluid experiences. This document outlines the architecture and implementation approach for each system.

## 1. Command Pattern & Action System

This system encapsulates actions as first-class objects, providing undo/redo capabilities and representing transformations as entities.

### Class Diagram

```
┌─────────────────────────┐
│ Command                 │
├─────────────────────────┤
│ +execute(): void        │
│ +undo(): void           │
│ +isReversible(): bool   │
│ +getDescription(): string│
└─────────────────────────┘
          ▲
          │
┌─────────┴─────────┐
│                   │
┌─────────────────┐ ┌────────────────────┐
│ AtomicCommand   │ │ CompositeCommand   │
├─────────────────┤ ├────────────────────┤
│ -state: any     │ │ -commands: Command[]│
└─────────────────┘ └────────────────────┘

┌─────────────────────────┐
│ CommandManager          │
├─────────────────────────┤
│ +execute(cmd: Command)  │
│ +undo(): bool           │
│ +redo(): bool           │
│ +clearHistory()         │
│ +saveHistory(): string  │
│ +loadHistory(str: string)│
└─────────────────────────┘
```

### Key Features

- **Command Serialization**: Commands can be saved and replayed
- **Macro Recording**: Record sequences of user actions
- **Transaction Support**: Group commands that must execute atomically
- **Context Awareness**: Commands have access to execution context
- **JavaScript Bridge**: Commands can be triggered from web UI

### Implementation Approach

The Command system will be implemented as a template-based pattern to maintain type safety while allowing flexibility. Each command will capture its own context and state needed for execution and undo operations.

```cpp
template <typename StateType>
class AtomicCommand : public Command {
private:
  StateType beforeState;
  StateType afterState;
  std::function<void(StateType&)> executeFunc;
  
public:
  void execute() override {
    executeFunc(afterState);
  }
  
  void undo() override {
    if (isReversible()) {
      std::swap(beforeState, afterState);
      execute();
      std::swap(beforeState, afterState);
    }
  }
};
```

## 2. Reactive Programming Model

This system enables data-driven architecture where changes propagate automatically through a dependency graph.

### Class Diagram

```
┌─────────────────────────────┐
│ Observable<T>               │
├─────────────────────────────┤
│ -value: T                   │
│ -observers: Observer<T>[]   │
│ +get(): T                   │
│ +set(value: T): void        │
│ +observe(observer): string  │
│ +unobserve(id: string): void│
└─────────────────────────────┘

┌─────────────────────────────┐
│ ComputedValue<T>            │
├─────────────────────────────┤
│ -dependencies: Observable[] │
│ -computeFunc: () => T       │
│ +get(): T                   │
└─────────────────────────────┘

┌─────────────────────────────┐
│ ReactiveContext             │
├─────────────────────────────┤
│ +trackDependencies(): void  │
│ +bindToDOM(el, observable)  │
└─────────────────────────────┘
```

### Key Features

- **Fine-grained Reactivity**: Updates only what needs to be updated
- **Automatic Dependency Tracking**: No manual subscription management
- **Computed Values**: Values derived from other observables
- **Batched Updates**: Updates are batched for efficiency
- **WebView Integration**: Bidirectional data binding with JavaScript

### Implementation Approach

The reactive system will use templates for type safety and a smart tracking system to automatically establish dependencies. This will be particularly powerful for C++ state that we want to reflect in the WebView UI.

```cpp
template <typename T>
class Observable {
private:
  T value;
  std::unordered_map<std::string, std::function<void(const T&)>> observers;
  
public:
  T get() const {
    ReactiveContext::current().trackDependency(this);
    return value;
  }
  
  void set(const T& newValue) {
    if (value != newValue) {
      T oldValue = value;
      value = newValue;
      notifyObservers(oldValue, newValue);
    }
  }
};
```

## 3. Resource Management Framework

This system handles loading, caching, and lifecycle management of assets across different scales.

### Class Diagram

```
┌────────────────────────────┐
│ ResourceHub                │
├────────────────────────────┤
│ +load<T>(id: string): T    │
│ +unload(id: string): bool  │
│ +preload(ids: string[])    │
│ +setMemoryBudget(bytes)    │
└────────────────────────────┘

┌────────────────────────────┐
│ Resource                   │
├────────────────────────────┤
│ -id: string                │
│ -memoryUsage: size_t       │
│ -state: ResourceState      │
│ +load(): bool              │
│ +unload(): void            │
│ +getMemoryUsage(): size_t  │
└────────────────────────────┘
       ▲
       │
┌──────┴───────┬───────────┬──────────┐
│              │           │          │
┌──────────┐ ┌─────┐ ┌──────────┐ ┌───────┐
│ Texture  │ │ Mesh│ │  Sound   │ │ Shader│
└──────────┘ └─────┘ └──────────┘ └───────┘

┌────────────────────────────┐
│ ResourceCache              │
├────────────────────────────┤
│ +get(id: string): Resource │
│ +add(resource: Resource)   │
│ +evict(bytes: size_t)      │
└────────────────────────────┘
```

### Key Features

- **Intent-Based Locking**: Uses the Quantum Fluctuation concurrency model
- **Asynchronous Loading**: Resources load without blocking the main thread
- **Memory-Aware Caching**: Automatically manages memory pressure
- **Priority-Based Loading**: Important resources load first
- **Scale-Appropriate Resources**: Different detail levels based on perspective
- **Reference Counting**: Resources unload when no longer needed

### Implementation Approach

The resource system uses CoordinatedGraph with intent-based locking, complemented by a flexible factory pattern with template specialization for each resource type. Background threads handle loading while maintaining a memory budget.

```cpp
template <typename T>
class ResourceHandle {
private:
  std::weak_ptr<T> resource;
  std::string resourceId;
  
public:
  T* get() {
    auto ptr = resource.lock();
    if (!ptr) {
      // Try to reload from cache
      ptr = ResourceHub::instance().load<T>(resourceId);
    }
    return ptr.get();
  }
};
```

## 4. Spatial Primitives & Scene Graph

This system provides the mathematical foundation for working with space across different scales.

### Class Diagram

```
┌───────────────────────────┐
│ Transform                 │
├───────────────────────────┤
│ -position: Vector3        │
│ -rotation: Quaternion     │
│ -scale: Vector3           │
│ +toMatrix(): Matrix4x4    │
│ +transformPoint(p): Vector3│
└───────────────────────────┘

┌───────────────────────────┐
│ SceneNode                 │
├───────────────────────────┤
│ -transform: Transform     │
│ -children: SceneNode[]    │
│ -parent: SceneNode*       │
│ +getGlobalTransform()     │
│ +attach(child: SceneNode) │
│ +detach(child: SceneNode) │
└───────────────────────────┘
       ▲
       │
┌──────┴────────┬───────────────┐
│               │               │
┌───────────┐ ┌─────────────┐ ┌─────────┐
│ Entity    │ │ SpatialGrid │ │ Camera  │
└───────────┘ └─────────────┘ └─────────┘
```

### Key Features

- **Scale-Independent Math**: Operations work correctly across vast scale differences
- **Hierarchical Transforms**: Nested coordinate systems
- **Spatial Queries**: Efficient neighbor finding and ray casting
- **Coordinate System Shifting**: Maintains precision at any scale
- **Perspective-Aware Rendering**: Detail level adjusts to perspective

### Implementation Approach

The spatial system leverages modern C++ templates for zero-overhead abstractions. It uses specialized types to prevent mixing different coordinate spaces accidentally.

```cpp
template <typename CoordinateSpace>
class Vector3 {
private:
  float x, y, z;
  
public:
  // Type-safe operations that prevent mixing coordinate spaces
  template <typename OtherSpace>
  Vector3<CoordinateSpace> operator+(const Vector3<OtherSpace>& other) = delete;
  
  // Same-space operations work fine
  Vector3<CoordinateSpace> operator+(const Vector3<CoordinateSpace>& other) {
    return Vector3<CoordinateSpace>{x + other.x, y + other.y, z + other.z};
  }
};
```

## 5. Temporal Dimension System

This system introduces the concept of time as a first-class dimension with manipulation capabilities.

### Class Diagram

```
┌────────────────────────────┐
│ Timeline                   │
├────────────────────────────┤
│ -currentTime: double       │
│ -timeRegions: TimeRegion[] │
│ +update(deltaTime: double) │
│ +createSnapshot(): TimeState│
│ +restoreSnapshot(state)    │
└────────────────────────────┘

┌────────────────────────────┐
│ TimeRegion                 │
├────────────────────────────┤
│ -timeScale: double         │
│ -entities: Entity[]        │
│ +update(worldDelta: double)│
└────────────────────────────┘

┌────────────────────────────┐
│ TimeState                  │
├────────────────────────────┤
│ -entityStates: Map<ID,any> │
│ -timestamp: double         │
│ +diff(other: TimeState)    │
└────────────────────────────┘
```

### Key Features

- **Multiple Time Flows**: Different regions progress at different rates
- **Time Manipulation**: Pause, slow, or accelerate time
- **Temporal Snapshots**: Capture and restore states at points in time
- **Predictive States**: Calculate future states based on current trajectories
- **Time-Travel Debugging**: Move backward and forward in time for debugging

### Implementation Approach

The timeline system uses a hierarchical approach where each timeline region manages its own subset of entities. Time-based properties use template specialization for automatic interpolation.

```cpp
class Timeline {
private:
  std::vector<TimeRegion> regions;
  std::deque<TimeState> history;
  
public:
  void update(double deltaTime) {
    // Create automatic snapshot
    if (snapshotInterval > 0) {
      snapshotCounter += deltaTime;
      if (snapshotCounter >= snapshotInterval) {
        history.push_back(createSnapshot());
        snapshotCounter = 0;
      }
    }
    
    // Update regions with their scaled time
    for (auto& region : regions) {
      region.update(deltaTime * region.getTimeScale());
    }
  }
};
```

## Integration Strategy

These five systems will work together to create a robust foundation for Fabric's perspective-fluid architecture:

1. **WebView for UI**: The reactive system will seamlessly sync C++ state with the WebView UI layer
2. **SDL3 for Rendering**: The spatial system will provide math primitives that SDL3 uses for rendering
3. **Commands for User Actions**: The command system bridges UI interactions with core state changes
4. **Resources for Assets**: The resource system manages loading/unloading of assets needed by UI and rendering
5. **Timeline for Animation**: The temporal system powers animations and simulations across scales

By implementing these systems before full SDL3/WebView integration, we ensure that Fabric maintains control over its core architecture while leveraging these libraries for what they do best.

## Next Steps

1. Implement each system's core classes and unit tests
2. Create integration tests between systems
3. Build example use cases demonstrating each system
4. Document API and usage patterns
5. Prepare integration points for SDL3 and WebView