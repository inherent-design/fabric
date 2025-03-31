#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "SyntaxTree.h"
#include <map>
#include <optional>
#include <variant>

// ArgumentParser
class ArgumentParser {
public:
  ArgumentParser() = default;

  const std::map<std::string, Token> &getArguments() const;
  const std::optional<Token> getArgument(const std::string &name) const;

  void parse(int argc, char *argv[]);
  void parse(const std::string &args);

  // Static methods for help and version
  static void printHelp();
  static void printVersion();

private:
  std::map<std::string, Token> arguments;

  // Allow ArgumentParserBuilder to access private members
  friend class ArgumentParserBuilder;
};

// ArgumentParserBuilder
//
// Builder pattern for ArgumentParser
class ArgumentParserBuilder {
public:
  ArgumentParserBuilder &addOption(const std::string &name, TokenType type,
                                   bool optional = false);
  ArgumentParser build() const;

private:
  std::map<std::string, std::pair<TokenType, bool>> options;
};

#endif // ARGUMENT_PARSER_H
