#include "SyntaxTree.h"

ASTNode::ASTNode(const Token &token) : token(token) {}

void ASTNode::addChild(std::shared_ptr<ASTNode> child) {
  children.push_back(child);
}

const std::vector<std::shared_ptr<ASTNode>> &ASTNode::getChildren() const {
  return children;
}

const Token &ASTNode::getToken() const { return token; }
