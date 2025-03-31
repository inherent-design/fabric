#ifndef TOKEN_TYPES_H
#define TOKEN_TYPES_H

#include <string>
#include <variant>

// Enum for token types with payloads for literals
enum class TokenType {
  // Keywords
  KeywordIf,
  KeywordElse,
  KeywordFor,
  KeywordWhile,
  KeywordFunction, // func, def, fn, function
  KeywordReturn,
  KeywordClass,
  KeywordStruct,
  KeywordEnum,
  KeywordImport,  // import, include, use
  KeywordPackage, // package, module, namespace

  // Operators
  OperatorPlus,         // +
  OperatorMinus,        // -
  OperatorMultiply,     // *
  OperatorDivide,       // /
  OperatorAssign,       // =
  OperatorEqual,        // ==
  OperatorNotEqual,     // !=, <>
  OperatorLessThan,     // <
  OperatorGreaterThan,  // >
  OperatorLessEqual,    // <=
  OperatorGreaterEqual, // >=

  // Delimiters
  DelimiterSemicolon,    // ;
  DelimiterComma,        // ,
  DelimiterDot,          // .
  DelimiterColon,        // :
  DelimiterOpenParen,    // (
  DelimiterCloseParen,   // )
  DelimiterOpenBrace,    // {
  DelimiterCloseBrace,   // }
  DelimiterOpenBracket,  // [
  DelimiterCloseBracket, // ]

  // Literals
  LiteralNumber,  // num(42)
  LiteralString,  // str("hello")
  LiteralBoolean, // bool(true)
  LiteralNull,    // null, nil, None
  LiteralFloat,   // float(3.14)
  LiteralChar,    // char('a')

  // Identifiers
  Identifier,

  // Comments
  CommentLine,  // //, #
  CommentBlock, // /* */

  // End of File
  EndOfFile,
};

// Token structure with type and value
struct Token {
  TokenType type;
  std::variant<int, double, std::string, bool> value;

  Token(TokenType type, std::variant<int, double, std::string, bool> value = {})
      : type(type), value(value) {}
};

#endif // TOKEN_TYPES_H
