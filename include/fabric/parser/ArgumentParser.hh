#pragma once

#include "fabric/parser/SyntaxTree.hh"
#include <map>
#include <optional>
#include <variant>

namespace Fabric {
// ArgumentParser
class ArgumentParser {
public:
  ArgumentParser() = default;

  std::string getErrorMsg() const { return errorMsg; }
  bool isValid() const { return valid; }

  const std::map<std::string, Token> &getArguments() const;
  const std::optional<Token> getArgument(const std::string &name) const;

  // Add a command-line argument definition
  void addArgument(const std::string &name, const std::string &description,
                   bool required = false);

  // Check if an argument was provided in the command line
  bool hasArgument(const std::string &name) const;

  void parse(int argc, char *argv[]);
  void parse(const std::string &args);

  bool validateArgs(
      const std::map<std::string, std::pair<TokenType, bool>> &options);

private:
  std::map<std::string, Token> arguments;
  std::map<std::string, std::pair<TokenType, bool>> availableArgs;
  std::map<std::string, std::string> argumentDescriptions;

  std::string errorMsg;
  bool valid = true;

  // Allow ArgumentParserBuilder to access private members
  friend class ArgumentParserBuilder;
};

// ArgumentParserBuilder
//
// Builder pattern for ArgumentParser
class ArgumentParserBuilder {
public:
  const std::map<std::string, std::pair<TokenType, bool>> &getOptions() const {
    return options;
  }

  ArgumentParserBuilder &addOption(const std::string &name, TokenType type,
                                   bool optional = false);
  ArgumentParser build() const;

private:
  std::map<std::string, std::pair<TokenType, bool>> options;
};
} // namespace Fabric
