//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LEXER_H
#define LEXER_H

#include <QByteArray>
#include <QObject>

class Lexer : public QObject {
public:
  enum Token {
    Nothing,
    Whitespace,
    Comment,
    String,
    Number,
    Keyword,
    Identifier,
    Operator,
    Error,
    Preprocessor,
    Constant,
    Variable,
    Function,
    Class,
    Type,
    Label,
    Regex,
    Embedded
  };

  struct Lexeme {
    Token token;
    QByteArray text;
  };

  Lexer(QObject *parent = nullptr);

  virtual QByteArray name() const = 0;
  virtual bool lex(const QByteArray &buffer) = 0;
  virtual bool hasNext() = 0;
  virtual Lexeme next() = 0;
};

#endif
