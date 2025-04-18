# Fabric Engine Component Flow

This document illustrates how the refactored Fabric components interact to create a robust game engine architecture. These diagrams provide a clear visual representation of the system design.

## System Architecture

The overall system architecture divides functionality into distinct layers:

```mermaid
flowchart TD
    subgraph "Game Application"
        Game["Game Instance"]
        Scene["Scene Manager"]
        Input["Input System"]
        Renderer["Rendering System"]
        Physics["Physics System"]
        Audio["Audio System"]
    end
    
    subgraph "Fabric Core"
        Component["Component System"]
        Event["Event System"]
        Lifecycle["Lifecycle Manager"]
        Plugin["Plugin System"]
    end
    
    subgraph "Resource Management"
        ResourceHub["ResourceHub (Facade)"]
        ResourceLoader["ResourceLoader"]
        ResourceDependencyManager["ResourceDependencyManager"]
        ResourceMemoryManager["ResourceMemoryManager"]
        ResourceThreadPool["ResourceThreadPool"]
    end
    
    subgraph "Utils"
        Concurrency["Concurrency Utilities"]
        Graph["Graph Algorithms"]
        Memory["Memory Management"]
        ThreadSafe["Thread-Safe Containers"]
    end
    
    Game --> Scene
    Game --> Input
    Game --> Renderer
    Game --> Physics
    Game --> Audio
    
    Scene --> Component
    Renderer --> Component
    Physics --> Component
    Audio --> Component
    
    Component --> Lifecycle
    Component --> Event
    
    ResourceHub --> ResourceLoader
    ResourceHub --> ResourceDependencyManager
    ResourceHub --> ResourceMemoryManager
    ResourceHub --> ResourceThreadPool
    
    ResourceDependencyManager --> Graph
    ResourceMemoryManager --> Memory
    ResourceThreadPool --> Concurrency
    ResourceThreadPool --> ThreadSafe
```

## Component Lifecycle

Components follow a consistent, predictable lifecycle:

```mermaid
stateDiagram-v2
    [*] --> Created: Constructor
    Created --> Initialized: initialize()
    Initialized --> Rendered: First render()
    Rendered --> Updating: First update()
    Updating --> Suspended: suspend()
    Suspended --> Updating: resume()
    Updating --> Destroyed: cleanup()
    Suspended --> Destroyed: cleanup()
    Destroyed --> [*]
```

## Game Execution Flow

This sequence illustrates a typical game execution flow:

```mermaid
sequenceDiagram
    participant App as Game Application
    participant Engine as Game Engine
    participant Scene as Scene Manager
    participant Entity as Entity Components
    participant Resource as ResourceHub
    
    App->>Engine: Initialize()
    Engine->>Resource: setMemoryBudget(512MB)
    Engine->>Resource: setWorkerThreadCount(4)
    
    Engine->>Scene: LoadScene("level1")
    Scene->>Resource: load<SceneData>("scenes", "level1")
    Resource->>Resource: ResourceLoader.load()
    Resource-->>Scene: SceneData
    
    Scene->>Scene: CreateEntities()
    loop For each entity definition
        Scene->>Entity: CreateComponent()
        Entity->>Entity: initialize()
        Entity->>Resource: loadAsync<Texture>("textures", "entity1_diffuse")
        Entity->>Resource: loadAsync<Model>("models", "entity1_mesh")
        Resource-->>Entity: Resources loaded callback
    end
    
    App->>Engine: StartGameLoop()
    
    loop Game Loop
        Engine->>Scene: Update(deltaTime)
        Scene->>Entity: update(deltaTime)
        
        Engine->>Scene: Render()
        Scene->>Entity: render()
    end
    
    App->>Engine: Shutdown()
    Engine->>Scene: UnloadAllScenes()
    Scene->>Entity: cleanup()
    Engine->>Resource: shutdown()
```

## Resource Management

The resource system handles dependencies and memory management:

```mermaid
flowchart TD
    subgraph "Resource Loading"
        Game["Game"] --> LoadScene["Load Scene"]
        LoadScene --> SceneResource["SceneResource"]
        SceneResource --> |"Dependencies"| ModelResources["Model Resources"]
        SceneResource --> |"Dependencies"| TextureResources["Texture Resources"]
        SceneResource --> |"Dependencies"| MaterialResources["Material Resources"]
        
        ModelResources --> |"Uses"| TextureResources
        MaterialResources --> |"Uses"| TextureResources
    end
    
    subgraph "ResourceHub Components"
        ResourceLoader["ResourceLoader"]
        ResourceDependencyManager["ResourceDependencyManager"]
        ResourceMemoryManager["ResourceMemoryManager"]
        ResourceThreadPool["ResourceThreadPool"]
        
        LoadScene --> ResourceLoader
        ResourceLoader --> ResourceDependencyManager
        ResourceDependencyManager --- ResourceMemoryManager
        ResourceLoader --> ResourceThreadPool
    end
    
    subgraph "Memory Management"
        ResourceMemoryManager --> |"Enforces"| MemoryBudget["Memory Budget"]
        ResourceMemoryManager --> |"If needed"| Eviction["Resource Eviction"]
        Eviction --> |"Based on"| LeastRecentlyUsed["LRU Algorithm"]
        Eviction --> |"Respects"| Dependencies["Resource Dependencies"]
    end
```

## Concurrency Model

Our concurrency system uses intent-based locking with timeouts:

```mermaid
stateDiagram-v2
    direction LR
    
    [*] --> ReadIntent: Request Read Lock
    [*] --> NodeModifyIntent: Request Node Modify Lock
    [*] --> GraphStructureIntent: Request Graph Structure Lock
    
    ReadIntent --> Waiting: Other locks exist
    NodeModifyIntent --> Waiting: Higher priority locks exist
    GraphStructureIntent --> Waiting: Wait for other locks to release
    
    Waiting --> Timeout: Wait time exceeds timeout
    Waiting --> Acquired: Lock granted
    
    Acquired --> Released: Explicit release
    Acquired --> Preempted: Higher priority request
    
    Timeout --> BackOff: Retry with exponential backoff
    Timeout --> Failed: Give up
    
    Preempted --> Waiting: Wait for higher priority to finish
    
    Released --> [*]
    Failed --> [*]
```

## Resource Lifecycle

Resources follow a predictable lifecycle with memory management:

```mermaid
stateDiagram-v2
    [*] --> Requested: ResourceHub.load() called
    Requested --> Loading: ResourceLoader begins loading
    Loading --> Loaded: Resource data available
    
    Loaded --> Active: Resource in use
    Active --> Inactive: No active references
    Inactive --> Active: Resource used again
    
    Inactive --> MemoryPressure: Memory budget exceeded
    MemoryPressure --> MarkedForEviction: LRU algorithm selects
    
    MarkedForEviction --> CheckingDependents: Check for dependents
    CheckingDependents --> Evicted: No dependents or all can be evicted
    CheckingDependents --> Protected: Has active dependents
    
    Protected --> Inactive: Memory pressure resolved
    
    Evicted --> [*]: Memory reclaimed
    
    Loaded --> Failed: Error during load
    Failed --> [*]
```

## Event System Flow

The event system provides loose coupling between components:

```mermaid
sequenceDiagram
    participant Input as Input System
    participant EventSystem as Event System
    participant PlayerEntity as Player Entity
    participant EnemyEntity as Enemy Entity
    participant AudioSystem as Audio System
    
    Input->>EventSystem: KeyPressEvent(SPACE)
    EventSystem->>PlayerEntity: dispatchEvent(KeyPressEvent)
    PlayerEntity->>PlayerEntity: handleKeyPress(SPACE)
    PlayerEntity->>EventSystem: PlayerJumpEvent
    EventSystem->>AudioSystem: dispatchEvent(PlayerJumpEvent)
    AudioSystem->>AudioSystem: playSound("jump.wav")
    
    Input->>EventSystem: MouseClickEvent(x, y)
    EventSystem->>PlayerEntity: dispatchEvent(MouseClickEvent)
    PlayerEntity->>EventSystem: PlayerAttackEvent
    EventSystem->>EnemyEntity: dispatchEvent(PlayerAttackEvent)
    EnemyEntity->>EnemyEntity: takeDamage(10)
    EnemyEntity->>EventSystem: EnemyDamagedEvent
    EventSystem->>AudioSystem: dispatchEvent(EnemyDamagedEvent)
```