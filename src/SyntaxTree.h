#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "Token.h"
#include <memory>
#include <vector>

// Represents a node in the abstract syntax tree
class ASTNode {
public:
  ASTNode(const Token &token);

  // Getters
  const Token &getToken() const;
  const std::vector<std::shared_ptr<ASTNode>> &getChildren() const;

  void addChild(std::shared_ptr<ASTNode> child);

private:
  Token token;
  std::vector<std::shared_ptr<ASTNode>> children;
};

// Helper functions
TokenType determineTokenType(const std::string &token);

Variant parseValue(const std::string &token, TokenType type);

#endif // SYNTAX_TREE_H
