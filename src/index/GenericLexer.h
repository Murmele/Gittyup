//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef GENERICLEXER_H
#define GENERICLEXER_H

#include "Lexer.h"

class GenericLexer : public Lexer {
public:
  GenericLexer(QObject *parent = nullptr);

  QByteArray name() const override { return "generic"; }
  bool lex(const QByteArray &buffer) override;
  bool hasNext() override;
  Lexeme next() override;

private:
  int mIndex;
  QByteArray mBuffer;
};

#endif
