#pragma once

#include <string>
#include <variant>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace Fabric {

/**
 * @brief Common variant type used throughout the Fabric framework
 * 
 * This variant defines the basic types that can be stored in various
 * data structures like Tokens, Events, and Configuration values.
 */
using Variant = std::variant<std::nullptr_t, bool, int, float, double, std::string>;

/**
 * @brief Type for callback functions that don't return a value
 */
using VoidCallback = std::function<void()>;

/**
 * @brief Type for a list of shared pointers to objects of type T
 */
template <typename T>
using SharedPtrList = std::vector<std::shared_ptr<T>>;

/**
 * @brief Type for a unique pointer to an object of type T
 */
template <typename T>
using UniquePtr = std::unique_ptr<T>;

/**
 * @brief Type for a shared pointer to an object of type T
 */
template <typename T>
using SharedPtr = std::shared_ptr<T>;

/**
 * @brief Type for a map with string keys and values of type T
 */
template <typename T>
using StringMap = std::map<std::string, T>;

/**
 * @brief Type for an unordered map with string keys and values of type T
 */
template <typename T>
using StringHashMap = std::unordered_map<std::string, T>;

/**
 * @brief Type for an unordered set of type T
 */
template <typename T>
using HashSet = std::unordered_set<T>;

/**
 * @brief Type for an optional value of type T
 */
template <typename T>
using Optional = std::optional<T>;

// Forward declarations for use in type aliases
struct Token;
enum class TokenType;

/**
 * @brief Type for a map of string to Token pairs
 */
using TokenMap = StringMap<Token>;

/**
 * @brief Type for an optional Token
 */
using OptionalToken = Optional<Token>;

/**
 * @brief Type for a pair containing a token type and a boolean flag
 */
using TokenTypeOptionPair = std::pair<TokenType, bool>;

/**
 * @brief Type for a map of string to token type option pairs
 */
using TokenTypeOptionsMap = StringMap<TokenTypeOptionPair>;

/**
 * @brief Type for a map of string to string
 */
using StringStringMap = StringMap<std::string>;


} // namespace Fabric