#pragma once

#include "SyntaxTree.hh"
#include <map>
#include <optional>
#include <variant>

namespace Fabric
{
  // ArgumentParser
  class ArgumentParser
  {
  public:
    ArgumentParser() = default;

    std::string getErrorMsg() const { return errorMsg; }
    bool isValid() const { return valid; }

    const std::map<std::string, Token> &getArguments() const;
    const std::optional<Token> getArgument(const std::string &name) const;

    void parse(int argc, char *argv[]);
    void parse(const std::string &args);

    bool validateArgs(
        const std::map<std::string, std::pair<TokenType, bool>> &options);

  private:
    std::map<std::string, Token> arguments;
    std::map<std::string, std::pair<TokenType, bool>> availableArgs;

    std::string errorMsg;
    bool valid = true;

    // Allow ArgumentParserBuilder to access private members
    friend class ArgumentParserBuilder;
  };

  // ArgumentParserBuilder
  //
  // Builder pattern for ArgumentParser
  class ArgumentParserBuilder
  {
  public:
    const std::map<std::string, std::pair<TokenType, bool>> &getOptions() const
    {
      return options;
    }

    ArgumentParserBuilder &addOption(const std::string &name, TokenType type,
                                     bool optional = false);
    ArgumentParser build() const;

  private:
    std::map<std::string, std::pair<TokenType, bool>> options;
  };
}