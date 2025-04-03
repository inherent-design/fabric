#include "ArgumentParser.hh"
#include "Constants.g.hh"
#include "ErrorHandling.hh"
#include "Logging.hh"
#include <iostream>
#include <sstream>

// Constructor is defaulted in the header, no need to define here

// Get argument value
const std::optional<Token>
ArgumentParser::getArgument(const std::string &name) const {
  auto it = arguments.find(name);
  if (it != arguments.end()) {
    return it->second;
  }
  return std::nullopt;
}

const std::map<std::string, Token> &ArgumentParser::getArguments() const {
  return arguments;
}

// Parse arguments using SyntaxTree
void ArgumentParser::parse(int argc, char *argv[]) {
  try {
    // Skip the program name (argv[0])
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];

      // Check if it's an option (starts with --)
      if (arg.substr(0, 2) == "--") {
        std::string optionName = arg;

        // Check if the next argument is a value (not an option)
        if (i + 1 < argc && argv[i + 1][0] != '-') {
          // Create token with appropriate type
          TokenType type = determineTokenType(argv[i + 1]);
          Variant value = parseValue(argv[i + 1], type);
          arguments[optionName] = Token(type, value);
          i++; // Skip the value in the next iteration
        } else {
          // Flag without value, set to true
          arguments[optionName] = Token(TokenType::CLIFlag, true);
        }
      } else {
        // Handle positional arguments if needed
        // For now, store them with a special prefix
        std::string posName = "pos" + std::to_string(i);
        TokenType type = determineTokenType(arg);
        Variant value = parseValue(arg, type);
        arguments[posName] = Token(type, value);
      }
    }

    // After parsing, validate required options
    validateArgs(availableArgs);
    if (!valid)
      return;
  } catch (const std::exception &e) {
    Logger::logError("Error parsing arguments: " + std::string(e.what()));
  }
}

void ArgumentParser::parse(const std::string &args) {
  try {
    std::istringstream stream(args);
    std::string arg;
    std::vector<std::string> argv;

    // Split the string into tokens
    while (stream >> arg) {
      argv.push_back(arg);
    }

    // Process the tokens similar to the other overload
    for (size_t i = 0; i < argv.size(); i++) {
      // Check if it's an option (starts with --)
      if (argv[i].substr(0, 2) == "--") {
        std::string optionName = argv[i];

        // Check if the next argument is a value (not an option)
        if (i + 1 < argv.size() && argv[i + 1][0] != '-') {
          // Create token with appropriate type
          TokenType type = determineTokenType(argv[i + 1]);
          Variant value = parseValue(argv[i + 1], type);
          arguments[optionName] = Token(type, value);
          i++; // Skip the value in the next iteration
        } else {
          // Flag without value, set to true
          arguments[optionName] = Token(TokenType::CLIFlag, true);
        }
      } else {
        // Handle positional arguments if needed
        std::string posName = "pos" + std::to_string(i);
        TokenType type = determineTokenType(argv[i]);
        Variant value = parseValue(argv[i], type);
        arguments[posName] = Token(type, value);
      }
    }

    // After parsing, validate required options
    validateArgs(availableArgs);
    if (!valid)
      return;
  } catch (const std::exception &e) {
    Logger::logError("Error parsing arguments: " + std::string(e.what()));
  }
}

bool ArgumentParser::validateArgs(
    const std::map<std::string, std::pair<TokenType, bool>> &options) {
  bool valid = true;
  std::vector<std::string> missingArgs;

  for (const auto &arg_pair : options) {
    const std::string &name = arg_pair.first;
    const bool optional = arg_pair.second.second;

    if (!optional && arguments.find(name) == arguments.end()) {
      valid = false;
      missingArgs.push_back(name);
    }
  }

  if (!valid) {
    std::string errorMsg = "Missing required arguments: ";
    for (size_t i = 0; i < missingArgs.size(); ++i) {
      errorMsg += missingArgs[i];
      if (i < missingArgs.size() - 1) {
        errorMsg += ", ";
      }
    }

    this->errorMsg = errorMsg;
  }

  this->valid = valid;
  return valid;
}

// Builder pattern implementation
ArgumentParserBuilder &ArgumentParserBuilder::addOption(const std::string &name,
                                                        TokenType type,
                                                        bool optional) {
  options[name] = {type, optional};
  return *this;
}

ArgumentParser ArgumentParserBuilder::build() const {
  ArgumentParser parser;
  parser.availableArgs = options;
  return parser;
}
