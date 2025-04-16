#include "fabric/core/ResourceHub.hh"
#include "fabric/utils/ErrorHandling.hh"
#include "fabric/utils/Logging.hh"
#include <algorithm>
#include <iostream>

namespace Fabric {

// Initialize static members if needed
std::mutex ResourceHub::mutex_;

// Worker thread function
void ResourceHub::workerThreadFunc() {
  // Call the process queue function
  this->processLoadQueue();
}

// Enforce budget wrapper
void ResourceHub::enforceBudget() { enforceMemoryBudget(); }

// Destructor implementation
ResourceHub::~ResourceHub() { shutdown(); }

// Shutdown implementation
void ResourceHub::shutdown() {
  // Signal worker threads to stop
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    shutdown_ = true;
  }
  queueCondition_.notify_all();

  // Wait for threads to finish
  for (auto &thread : workerThreads_) {
    if (thread && thread->joinable()) {
      thread->join();
    }
  }
  workerThreads_.clear();

  // Unload all resources in proper dependency order
  clear();
}

// Implementation of other non-template methods
bool ResourceHub::addDependency(const std::string &dependentId,
                                const std::string &dependencyId) {
  try {
    return resourceGraph_.addEdge(dependentId, dependencyId);
  } catch (const CycleDetectedException &e) {
    // Log the cycle detection
    return false;
  }
}

bool ResourceHub::removeDependency(const std::string &dependentId,
                                   const std::string &dependencyId) {
  return resourceGraph_.removeEdge(dependentId, dependencyId);
}

bool ResourceHub::unload(const std::string &resourceId, bool cascade) {
  if (cascade) {
    // Unload in dependency order
    return unloadRecursive(resourceId);
  } else {
    auto resourceNode = resourceGraph_.getNode(resourceId);
    if (!resourceNode) {
      // Resource not found
      return false;
    }

    auto nodeLock = resourceNode->tryLock(
        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::NodeModify,
        true);
    if (!nodeLock || !nodeLock->isLocked()) {
      return false;
    }

    auto resource = nodeLock->getNode()->getData();

    // Check if there are dependencies on this resource
    auto dependents = resourceGraph_.getInEdges(resourceId);
    if (!dependents.empty()) {
      // Can't unload if other resources depend on this one
      return false;
    }

    // Unload the resource
    ResourceState state = resource->getState();
    if (state == ResourceState::Loaded) {
      resource->unload();
    }

    // Release the lock before removing the node to avoid deadlocks
    nodeLock->release();

    // Remove from graph
    return resourceGraph_.removeNode(resourceId);
  }
}

bool ResourceHub::unload(const std::string &resourceId) {
  return unload(resourceId, false);
}

bool ResourceHub::unloadRecursive(const std::string &resourceId) {
  // Get dependencies in topological order to ensure proper unloading
  std::vector<std::string> unloadOrder;

  // Helper function for DFS traversal
  std::function<void(const std::string &, std::unordered_set<std::string> &)>
      collectDependents;
  collectDependents = [&](const std::string &id,
                          std::unordered_set<std::string> &visited) {
    visited.insert(id);

    // First recurse to all dependents
    auto dependents = resourceGraph_.getInEdges(id);
    for (const auto &dependent : dependents) {
      if (visited.find(dependent) == visited.end()) {
        collectDependents(dependent, visited);
      }
    }

    // Then add this resource
    unloadOrder.push_back(id);
  };

  // Collect dependents of this resource
  std::unordered_set<std::string> visited;
  collectDependents(resourceId, visited);

  // Unload in topological order
  bool success = true;
  for (const auto &id : unloadOrder) {
    auto node = resourceGraph_.getNode(id);
    if (node) {
      auto nodeLock = resourceGraph_.tryLockNode(
          id,
          CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::NodeModify,
          true);
      if (nodeLock && nodeLock->isLocked()) {
        auto res = nodeLock->getNode()->getData();
        if (res->getState() == ResourceState::Loaded) {
          res->unload();
        }
        nodeLock->release();
        success &= resourceGraph_.removeNode(id);
      }
    }
  }

  return success;
}

void ResourceHub::preload(const std::vector<std::string> &typeIds,
                          const std::vector<std::string> &resourceIds,
                          ResourcePriority priority) {
  if (typeIds.size() != resourceIds.size()) {
    throw std::invalid_argument(
        "typeIds and resourceIds must have the same size");
  }

  for (size_t i = 0; i < resourceIds.size(); ++i) {
    ResourceLoadRequest request;
    request.typeId = typeIds[i];
    request.resourceId = resourceIds[i];
    request.priority = priority;

    // Add the request to the queue
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      loadQueue_.push(request);
    }
  }

  // Signal the worker thread
  queueCondition_.notify_one();
}

void ResourceHub::setMemoryBudget(size_t bytes) {
  memoryBudget_ = bytes;
  // When we set a new budget, check if we need to enforce it
  enforceBudget();
}

size_t ResourceHub::getMemoryUsage() const {
  size_t total = 0;

  // Get all resources from the graph
  auto allResourceIds = resourceGraph_.getAllNodes();

  for (const auto &id : allResourceIds) {
    auto node = resourceGraph_.getNode(id);
    if (!node) {
      continue;
    }

    auto nodeLock = resourceGraph_.tryLockNode(
        id,
        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);
    if (!nodeLock || !nodeLock->isLocked()) {
      continue;
    }

    auto resource = nodeLock->getNode()->getData();
    if (resource->getState() == ResourceState::Loaded) {
      total += resource->getMemoryUsage();
    }
  }

  return total;
}

size_t ResourceHub::getMemoryBudget() const { return memoryBudget_; }

bool ResourceHub::isLoaded(const std::string &resourceId) const {
  auto node = resourceGraph_.getNode(resourceId);
  if (!node) {
    return false;
  }

  auto nodeLock = resourceGraph_.tryLockNode(
      resourceId,
      CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);
  if (!nodeLock || !nodeLock->isLocked()) {
    return false;
  }

  auto resource = nodeLock->getNode()->getData();
  return resource->getState() == ResourceState::Loaded;
}

std::vector<std::string>
ResourceHub::getDependentResources(const std::string &resourceId) const {
  auto dependents = resourceGraph_.getInEdges(resourceId);
  return std::vector<std::string>(dependents.begin(), dependents.end());
}

std::vector<std::string>
ResourceHub::getDependencyResources(const std::string &resourceId) const {
  auto dependencies = resourceGraph_.getOutEdges(resourceId);
  return std::vector<std::string>(dependencies.begin(), dependencies.end());
}

std::unordered_set<std::string>
ResourceHub::getDependents(const std::string &resourceId) {
  return resourceGraph_.getInEdges(resourceId);
}

std::unordered_set<std::string>
ResourceHub::getDependencies(const std::string &resourceId) {
  return resourceGraph_.getOutEdges(resourceId);
}

bool ResourceHub::hasResource(const std::string &resourceId) {
  return resourceGraph_.hasNode(resourceId);
}

size_t ResourceHub::enforceMemoryBudget() {
  // This mutex ensures only one thread performs eviction at a time
  static std::mutex enforceBudgetMutex;

  // Use try_lock with timeout to avoid deadlocks
  auto start = std::chrono::steady_clock::now();
  std::unique_lock<std::mutex> budgetLock(enforceBudgetMutex, std::try_to_lock);

  // If another thread is already enforcing the budget, try with timeout
  if (!budgetLock.owns_lock()) {
    while (!budgetLock.owns_lock()) {
      // Try to get the lock
      budgetLock =
          std::unique_lock<std::mutex>(enforceBudgetMutex, std::try_to_lock);
      if (budgetLock.owns_lock()) {
        break;
      }

      // Check if we've exceeded the timeout
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
      if (elapsed >= 200) { // 200ms timeout
        // Skip this invocation if we couldn't get the lock in time
        return 0;
      }

      // Short sleep to avoid CPU spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  // Use a timeout for the overall operation to avoid hanging
  const int ENFORCE_TIMEOUT_MS = 1000; // 1 second timeout
  auto enforceStart = std::chrono::steady_clock::now();
  auto enforceTimeout = [&]() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - enforceStart)
               .count() > ENFORCE_TIMEOUT_MS;
  };

  // Check if we need to enforce the budget
  size_t currentUsage = getMemoryUsage();
  if (currentUsage <= memoryBudget_) {
    return 0;
  }

  // Calculate how much memory we need to free
  size_t toFree = currentUsage - memoryBudget_;

  // Get all resources with timeout protection
  auto allResourceIds = resourceGraph_.getAllNodes();

  // Collect eviction candidates sorted by access time
  struct EvictionCandidate {
    std::string id;
    std::chrono::steady_clock::time_point lastAccessTime;
    size_t size;
  };

  std::vector<EvictionCandidate> candidates;
  candidates.reserve(allResourceIds.size());

  // First phase: collect candidates with proper locking and timeout protection
  for (const auto &id : allResourceIds) {
    // Check timeout to avoid hanging
    if (enforceTimeout()) {
      return 0;
    }

    // Get the node with timeout protection
    auto node = resourceGraph_.getNode(id, 50); // 50ms timeout
    if (!node) {
      continue;
    }

    // Use short timeout for node locks to prevent hanging
    auto nodeLock = resourceGraph_.tryLockNode(
        id,
        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read, 
        false, 50);

    if (!nodeLock || !nodeLock->isLocked()) {
      // Skip this node if we couldn't get a lock quickly
      continue;
    }

    auto resource = nodeLock->getNode()->getData();
    auto resourceSize = resource->getMemoryUsage();
    auto lastAccess = node->getLastAccessTime();

    // Only consider resources that are loaded and have a single reference
    bool isLoaded = resource->getState() == ResourceState::Loaded;
    bool hasSingleRef = resource.use_count() == 1;

    // Release the lock before checking dependencies
    nodeLock->release();

    if (isLoaded && hasSingleRef) {
      // Check for dependencies with timeout protection
      bool hasDependents = false;
      try {
        // Check if other resources depend on this one
        auto dependents = resourceGraph_.getInEdges(id);
        hasDependents = !dependents.empty();
      } catch (const std::exception &e) {
        // If there's an error checking dependencies, be conservative and don't
        // evict
        hasDependents = true;
      }

      if (!hasDependents) {
        candidates.push_back({id, lastAccess, resourceSize});
      }
    }
  }

  // Sort by last access time (oldest first)
  std::sort(candidates.begin(), candidates.end(),
            [](const EvictionCandidate &a, const EvictionCandidate &b) {
              return a.lastAccessTime < b.lastAccessTime;
            });

  // Second phase: evict resources until we've freed enough memory
  size_t evictedCount = 0;
  size_t freedMemory = 0;

  for (const auto &candidate : candidates) {
    // Check timeout to avoid hanging
    if (enforceTimeout()) {
      return evictedCount;
    }

    // Get the node again with timeout protection
    auto node = resourceGraph_.getNode(candidate.id, 50); // 50ms timeout
    if (!node) {
      continue;
    }

    // Try to get an exclusive lock with short timeout
    auto nodeLock = resourceGraph_.tryLockNode(
        candidate.id,
        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::NodeModify,
        true, 50);

    if (!nodeLock || !nodeLock->isLocked()) {
      // Skip this node if we couldn't get a lock quickly
      continue;
    }

    auto resource = nodeLock->getNode()->getData();

    // Double-check conditions under exclusive lock
    if (resource.use_count() > 1 ||
        resource->getState() != ResourceState::Loaded) {
      continue;
    }

    // Release the lock before checking dependencies to avoid lock order issues
    nodeLock->release();

    // Check dependencies again
    bool hasDependents = false;
    try {
      auto dependents = resourceGraph_.getInEdges(candidate.id);
      hasDependents = !dependents.empty();
    } catch (const std::exception &e) {
      // If there's an error checking dependencies, be conservative and don't
      // evict
      hasDependents = true;
    }

    if (hasDependents) {
      continue;
    }

    // Re-acquire the lock to unload the resource
    nodeLock = resourceGraph_.tryLockNode(
        candidate.id,
        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::NodeModify,
        true, 50);

    if (!nodeLock || !nodeLock->isLocked()) {
      continue;
    }

    // Attempt to unload the resource
    try {
      // Double-check state again after re-acquiring lock
      resource = nodeLock->getNode()->getData();
      if (resource->getState() == ResourceState::Loaded &&
          resource.use_count() == 1) {
        resource->unload();

        // Release the lock before removing node to avoid deadlocks
        nodeLock->release();

        // Remove from graph
        if (resourceGraph_.removeNode(candidate.id)) {
          // Update stats
          freedMemory += candidate.size;
          evictedCount++;
        }
      }
    } catch (const std::exception &e) {
      // Log error but continue with other resources
      Logger::logError("Error evicting resource " + candidate.id + ": " + e.what());
    }

    // If we've freed enough memory, we can stop
    if (freedMemory >= toFree) {
      break;
    }
  }

  return evictedCount;
}

void ResourceHub::processLoadQueue() {
  try {
    while (true) {
      ResourceLoadRequest request;
      bool hasRequest = false;

      // Get a request from the queue
      {
        std::unique_lock<std::mutex> lock(queueMutex_);

        // Wait for a request or shutdown signal
        // Add a timeout to periodically check the shutdown status even if the
        // condition variable isn't notified
        auto waitResult = queueCondition_.wait_for(
            lock, std::chrono::milliseconds(500),
            [this] { return !loadQueue_.empty() || shutdown_; });

        // Check for shutdown first
        if (shutdown_) {
          break;
        }

        // Skip this iteration if no request is available
        if (!waitResult || loadQueue_.empty()) {
          continue;
        }

        // Get the next request
        request = loadQueue_.top();
        loadQueue_.pop();
        hasRequest = true;
      }

      // Skip processing if we didn't get a valid request
      if (!hasRequest) {
        continue;
      }

      try {
        // Process the request
        auto resourceNode = resourceGraph_.getNode(request.resourceId);
        std::shared_ptr<Resource> resource;

        if (!resourceNode) {
          // Create a new resource
          resource =
              ResourceFactory::create(request.typeId, request.resourceId);
          if (resource) {
            if (!resourceGraph_.addNode(request.resourceId, resource)) {
              // Node may have been added by another thread, try to get it again
              resourceNode = resourceGraph_.getNode(request.resourceId);
              if (resourceNode) {
                auto nodeLock = resourceNode->tryLock(
                    CoordinatedGraph<
                        std::shared_ptr<Resource>>::LockIntent::Read);

                if (nodeLock && nodeLock->isLocked()) {
                  resource = nodeLock->getNode()->getData();
                }
              }
            } else {
              resourceNode = resourceGraph_.getNode(request.resourceId);
            }
          }
        } else {
          // Take a shared lock to read the data
          auto nodeLock = resourceNode->tryLock(
              CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);

          if (nodeLock && nodeLock->isLocked()) {
            resource = nodeLock->getNode()->getData();
          }
        }

        // Load the resource if needed
        if (resource) {
          ResourceState state = resource->getState();
          if (state != ResourceState::Loaded) {
            // Try to load - handle exceptions that might occur
            try {
              resource->load();

              // Update the last access time
              if (resourceNode) {
                resourceNode->touch();
              }
            } catch (const std::exception &e) {
              Logger::logError("Error loading resource " + request.resourceId + ": " + e.what());
            }
          }
        }

        // Enforce memory budget - handle any exceptions
        try {
          enforceBudget();
        } catch (const std::exception &e) {
          Logger::logError("Error enforcing memory budget: " + std::string(e.what()));
        }

        // Call the callback - handle any exceptions
        if (request.callback && resource) {
          try {
            request.callback(resource);
          } catch (const std::exception &e) {
            Logger::logError("Error in resource callback for " + request.resourceId + ": " + e.what());
          }
        }
      } catch (const std::exception &e) {
        // Catch any exception during request processing to keep the thread
        // alive
        Logger::logError("Error processing request for " + request.resourceId + ": " + e.what());
      } catch (...) {
        // Catch any unknown exception
        Logger::logError("Unknown error processing request for " + request.resourceId);
      }
    }
  } catch (const std::exception &e) {
    // Log the error but don't propagate it - this would terminate the thread
    Logger::logCritical("Fatal worker thread error: " + std::string(e.what()));
  } catch (...) {
    // Catch any unknown exception
    Logger::logCritical("Unknown fatal worker thread error");
  }
}

void ResourceHub::disableWorkerThreadsForTesting() {
  // Use mutex to synchronize thread shutdown
  std::unique_lock<std::mutex> qLock(threadControlMutex_);

  // Return early if already shut down
  if (workerThreadCount_ == 0 && workerThreads_.empty()) {
    return;
  }

  // Signal threads to exit
  {
    std::lock_guard<std::mutex> qLock(queueMutex_);
    shutdown_ = true;
    // Clear any pending requests to prevent blocked workers
    while (!loadQueue_.empty()) {
      loadQueue_.pop();
    }
  }
  queueCondition_.notify_all();

  // Join all worker threads with timeout to prevent hangs
  for (auto &thread : workerThreads_) {
    if (thread && thread->joinable()) {
      // Create a timeout thread that interrupts if joining takes too long
      std::atomic<bool> joinCompleted{false};
      std::thread timeoutThread([&]() {
        for (int i = 0; i < 50; i++) { // Wait up to 5 seconds
          if (joinCompleted)
            return;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!joinCompleted) {
          // Log timeout warning - thread may be deadlocked
          Logger::logWarning("Thread join timeout in disableWorkerThreadsForTesting");
          // Thread is likely deadlocked, detach it
          return;
        }
      });

      // Try to join with a timeout
      try {
        if (thread->joinable()) {
          thread->join();
          joinCompleted = true;
        }
      } catch (const std::exception& e) {
        Logger::logError("Error joining thread: " + std::string(e.what()));
      }

      // Clean up timeout thread
      if (timeoutThread.joinable()) {
        timeoutThread.join();
      }
    }
  }

  workerThreads_.clear();
  workerThreadCount_ = 0;
}

void ResourceHub::restartWorkerThreadsAfterTesting() {
  // Use mutex to synchronize thread creation
  std::unique_lock<std::mutex> qLock(threadControlMutex_);

  // Make sure any existing threads are properly shut down
  if (!workerThreads_.empty()) {
    // Signal threads to exit
    {
      std::lock_guard<std::mutex> qLock(queueMutex_);
      shutdown_ = true;
      // Clear any pending requests to prevent blocked workers
      while (!loadQueue_.empty()) {
        loadQueue_.pop();
      }
    }
    queueCondition_.notify_all();

    // Join with timeout to prevent hangs
    for (auto &thread : workerThreads_) {
      if (thread && thread->joinable()) {
        // Create a timeout thread
        std::atomic<bool> joinCompleted{false};
        std::thread timeoutThread([&]() {
          for (int i = 0; i < 50; i++) { // Wait up to 5 seconds
            if (joinCompleted)
              return;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
          if (!joinCompleted) {
            // Log timeout warning
            Logger::logWarning("Thread join timeout in restartWorkerThreadsAfterTesting");
          }
        });

        // Join the worker thread
        thread->join();
        joinCompleted = true;

        // Clean up timeout thread
        if (timeoutThread.joinable()) {
          timeoutThread.join();
        }
      }
    }
    workerThreads_.clear();
  }

  // Start fresh
  shutdown_ = false;
  workerThreadCount_ = std::thread::hardware_concurrency();

  // Start worker threads
  workerThreads_.reserve(workerThreadCount_);
  for (unsigned int i = 0; i < workerThreadCount_; ++i) {
    workerThreads_.push_back(
        std::make_unique<std::thread>(&ResourceHub::workerThreadFunc, this));
  }
}

unsigned int ResourceHub::getWorkerThreadCount() const {
  return workerThreadCount_;
}

void ResourceHub::setWorkerThreadCount(unsigned int count) {
  if (count == 0) {
    throw std::invalid_argument("Worker thread count must be at least 1");
  }

  // Use thread control mutex to synchronize thread adjustments
  std::unique_lock<std::mutex> controlLock(threadControlMutex_);

  // If no change, return early
  if (count == workerThreadCount_) {
    return;
  }

  // If decreasing thread count, signal specific threads to stop
  if (count < workerThreadCount_) {
    unsigned int threadsToStop = workerThreadCount_ - count;
    std::vector<std::unique_ptr<std::thread>> threadsToJoin;

    // Move threads to be stopped to a separate vector
    for (unsigned int i = 0; i < threadsToStop; ++i) {
      if (i < workerThreads_.size()) {
        threadsToJoin.push_back(std::move(workerThreads_.back()));
        workerThreads_.pop_back();
      }
    }

    // Signal threads to check their shutdown status
    queueCondition_.notify_all();

    // Join the threads we're removing with timeout protection
    for (auto &thread : threadsToJoin) {
      if (thread && thread->joinable()) {
        // Create a timeout thread
        std::atomic<bool> joinCompleted{false};
        std::thread timeoutThread([&]() {
          for (int i = 0; i < 30; i++) { // 3 second timeout
            if (joinCompleted)
              return;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
          if (!joinCompleted) {
            // Log timeout warning
            Logger::logWarning("Thread join timeout in setWorkerThreadCount");
          }
        });

        // Join the worker thread
        thread->join();
        joinCompleted = true;

        // Clean up timeout thread
        if (timeoutThread.joinable()) {
          timeoutThread.join();
        }
      }
    }
  }

  // If increasing thread count, create new threads
  if (count > workerThreadCount_) {
    // Ensure we're not in shutdown state
    if (shutdown_) {
      shutdown_ = false; // Reset shutdown flag
    }

    // Create new threads
    unsigned int threadsToAdd = count - workerThreadCount_;
    workerThreads_.reserve(workerThreads_.size() + threadsToAdd);

    for (unsigned int i = 0; i < threadsToAdd; ++i) {
      try {
        workerThreads_.push_back(std::make_unique<std::thread>(
            &ResourceHub::workerThreadFunc, this));
      } catch (const std::exception &e) {
        // Log thread creation error
        Logger::logError("Error creating worker thread: " + std::string(e.what()));
      }
    }
  }

  // Update the thread count
  workerThreadCount_ = count;
}

ResourceHub &ResourceHub::instance() {
  static ResourceHub manager;
  return manager;
}

// Constructor implementation
ResourceHub::ResourceHub()
    : memoryBudget_(1024 * 1024 * 1024), // 1 GB default
      workerThreadCount_(std::thread::hardware_concurrency()),
      shutdown_(false) {
  // Optional debug message
  std::cout << "ResourceHub initialized with " << workerThreadCount_
            << " worker threads" << std::endl;

  // Start worker threads - but only if we're not in a test environment
  // Tests should call disableWorkerThreadsForTesting() in their setup
  // which will be a no-op if no threads are running
  if (workerThreadCount_ > 0) {
    for (unsigned int i = 0; i < workerThreadCount_; ++i) {
      workerThreads_.push_back(
          std::make_unique<std::thread>(&ResourceHub::workerThreadFunc, this));
    }
  }
}

void ResourceHub::clear() {
  // Use timeout protection
  const int CLEAR_TIMEOUT_MS = 2000; // 2 second timeout
  auto startTime = std::chrono::steady_clock::now();
  auto checkTimeout = [&]() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - startTime)
               .count() > CLEAR_TIMEOUT_MS;
  };

  // Get all resource IDs from the graph with timeout protection
  auto allResourceIds = resourceGraph_.getAllNodes();

  // First, find resources with no dependents (leaf resources)
  std::vector<std::string> leafResources;
  for (const auto &id : allResourceIds) {
    // Check for timeout
    if (checkTimeout()) {
      Logger::logWarning("clear() timed out during leaf resource collection");
      return;
    }

    try {
      auto dependents = resourceGraph_.getInEdges(id);
      if (dependents.empty()) {
        leafResources.push_back(id);
      }
    } catch (const std::exception &e) {
      // Log error but continue
      Logger::logError("Error checking dependents for " + id + ": " + e.what());
    }
  }

  // Process leaf resources first in reverse topological order
  for (const auto &id : leafResources) {
    // Check for timeout
    if (checkTimeout()) {
      Logger::logWarning("clear() timed out during resource unloading");
      return;
    }

    try {
      // Use a timeout for each unload operation to prevent hanging on a single
      // resource
      auto unloadStart = std::chrono::steady_clock::now();
      bool unloadTimedOut = false;

      // Run unload with timeout protection
      std::thread unloadThread([this, &id, &unloadTimedOut]() {
        try {
          unloadRecursive(id);
        } catch (const std::exception &e) {
          // Log error but continue
          Logger::logError("Error unloading resource " + id + ": " + e.what());
        }
      });

      // Wait for unload thread with timeout
      while (unloadThread.joinable()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - unloadStart)
                           .count();

        if (elapsed > 200) { // 200ms timeout per resource
          unloadTimedOut = true;
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      if (unloadTimedOut) {
        // Log warning but continue with other resources
        Logger::logWarning("Unloading resource " + id + " timed out");
        unloadThread
            .detach(); // Detach the thread to let it continue in background
      } else {
        // Thread finished normally, join it
        if (unloadThread.joinable()) {
          unloadThread.join();
        }
      }
    } catch (const std::exception &e) {
      // Log error but continue
      Logger::logError("Error in unload thread for " + id + ": " + e.what());
    }
  }

  // Check for any remaining resources
  if (checkTimeout()) {
    Logger::logWarning("clear() timed out before cleaning remaining resources");
    return;
  }

  // Get fresh list of resources that might remain
  allResourceIds = resourceGraph_.getAllNodes();

  // If there are still resources left, directly remove them
  for (const auto &id : allResourceIds) {
    // Check for timeout
    if (checkTimeout()) {
      Logger::logWarning("clear() timed out during final cleanup");
      return;
    }

    // Get the node with timeout protection
    auto node = resourceGraph_.getNode(id, 50); // 50ms timeout
    if (!node) {
      continue;
    }

    try {
      // Try to acquire an exclusive lock with short timeout
      auto nodeLock = resourceGraph_.tryLockNode(
          id,
          CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::NodeModify,
          true, 50);

      if (!nodeLock || !nodeLock->isLocked()) {
        // Skip this node if we couldn't lock it quickly
        continue;
      }

      // Get the resource data
      auto resource = nodeLock->getNode()->getData();

      // If the resource is loaded, try to unload it
      if (resource && resource->getState() == ResourceState::Loaded) {
        resource->unload();
      }

      // Release the lock before removing from graph to prevent deadlocks
      nodeLock->release();

      // Remove the node from the graph
      resourceGraph_.removeNode(id);
    } catch (const std::exception &e) {
      // Log error but continue
      Logger::logError("Error cleaning up resource " + id + ": " + e.what());
    }
  }
}

} // namespace Fabric
