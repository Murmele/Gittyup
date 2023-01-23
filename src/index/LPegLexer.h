//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LPEGLEXER_H
#define LPEGLEXER_H

#include "Lexer.h"
#include <QSharedPointer>

struct lua_State;

class LPegLexer : public Lexer {
public:
  // Default values are provided to play nicely with map accessor functions.
  // It is an error to construct a Lexer with an empty home or lexer kind.
  LPegLexer(const QByteArray &home = QByteArray(),
            const QByteArray &lexer = QByteArray(), QObject *parent = nullptr);

  QByteArray name() const override { return mName; }
  bool lex(const QByteArray &buffer) override;
  bool hasNext() override;
  Lexeme next() override;

private:
  QSharedPointer<lua_State> mL;
  QByteArray mName;

  int mIndex;
  int mLength;
  int mStartPos;
  QByteArray mBuffer;
};

#endif
