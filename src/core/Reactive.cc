#include "fabric/core/Reactive.hh"
#include "fabric/utils/Logging.hh"

namespace Fabric {

// Define any internal variables needed
// No extra implementation needed since all is defined in the header

// ReactiveContext implementation
void ReactiveContext::execute(const std::function<void()>& func) {
    std::unordered_set<const void*> deps;
    auto scope = current().trackDependencies(deps);
    func();
}

std::unordered_set<const void*> ReactiveContext::collectCurrentDependencies() {
    return internal::currentDependencies;
}

void ReactiveContext::reset() {
    ReactiveContext& context = current();
    context.currentTracker_ = nullptr;
    internal::currentDependencies.clear();
}

// ReactiveTransaction implementation
ReactiveTransaction ReactiveTransaction::begin() {
    return ReactiveTransaction();
}

ReactiveTransaction::ReactiveTransaction() {
    // Check if a transaction is already active (for nesting)
    internal::activeTransactionCount++;
    
    if (internal::activeTransactionCount == 1) {
        // This is the root transaction
        isRootTransaction_ = true;
    }
}

ReactiveTransaction::~ReactiveTransaction() {
    if (!committed_ && !rolledBack_) {
        commit();
    }
    
    internal::activeTransactionCount--;
}

void ReactiveTransaction::commit() {
    if (committed_ || rolledBack_) {
        throw std::runtime_error("Transaction already committed or rolled back");
    }
    
    committed_ = true;
}

void ReactiveTransaction::rollback() {
    if (committed_ || rolledBack_) {
        throw std::runtime_error("Transaction already committed or rolled back");
    }
    
    rolledBack_ = true;
}

bool ReactiveTransaction::isTransactionActive() {
    return internal::activeTransactionCount > 0;
}

} // namespace Fabric