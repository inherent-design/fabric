#include "ArgumentParser.h"
#include "Constants.g.h"
#include "ErrorHandling.h"
#include "Logging.h"
#include <iostream>
#include <sstream>

// Constructor is defaulted in the header, no need to define here

// Parse arguments using SyntaxTree
void ArgumentParser::parse(int argc, char *argv[]) {
  try {
    // TODO: parse
  } catch (const std::exception &e) {
    ErrorHandler::handleError(std::runtime_error(e.what()));
    Logger::logError("Error parsing arguments: " + std::string(e.what()));
  }
}

void ArgumentParser::parse(const std::string &args) {
  try {
    std::istringstream stream(args);
    std::string token;
    while (stream >> token) {
      // TODO: parse
    }
    Logger::logInfo("Arguments parsed successfully.");
  } catch (const std::exception &e) {
    ErrorHandler::handleError(std::runtime_error(e.what()));
    Logger::logError("Error parsing arguments: " + std::string(e.what()));
  }
}

// Get argument value
const std::optional<Token>
ArgumentParser::getArgument(const std::string &name) const {
  // TODO: stub
  return std::nullopt;
}

const std::map<std::string, Token> &ArgumentParser::getArguments() const {
  return arguments;
}

// Builder pattern implementation
ArgumentParserBuilder &ArgumentParserBuilder::addOption(const std::string &name,
                                                        TokenType type,
                                                        bool optional) {
  options[name] = {type, optional};
  Logger::logInfo("Option added: " + name);
  return *this;
}

ArgumentParser ArgumentParserBuilder::build() const {
  ArgumentParser parser;
  // Initialize parser with options
  for (const auto &[name, option] : options) {
    // TODO: build parser
  }
  Logger::logInfo("ArgumentParser built successfully.");
  return parser;
}

// Static method to print help information
void ArgumentParser::printHelp() {
  std::cout << "Usage: fabric [options]\n"
            << "Options:\n"
            << "  --help       Show this help message\n"
            << "  --version    Show version information\n"
            << "  ...          Other options\n";
}

// Static method to print version information
void ArgumentParser::printVersion() {
  std::cout << "Fabric Engine Version: " << VERSION << std::endl;
}
