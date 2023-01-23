//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GenericLexer.h"

namespace {

const QByteArray kOperators = ":/.()*?";

char safeAt(const QByteArray &buffer, int i) {
  return (i < buffer.length()) ? buffer.at(i) : 0;
}

bool isDigit(char ch) { return (ch >= '0' && ch <= '9'); }

bool isAlpha(char ch) {
  return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

} // namespace

GenericLexer::GenericLexer(QObject *parent) : Lexer(parent) {}

bool GenericLexer::lex(const QByteArray &buffer) {
  mIndex = 0;
  mBuffer = buffer;

  return true;
}

bool GenericLexer::hasNext() { return (mIndex < mBuffer.length()); }

Lexer::Lexeme GenericLexer::next() {
  // Whitespace isn't a valid state itself. It indicates a
  // string of underscores that might become an identifier.
  int startPos = 0;
  Token state = Nothing;
  for (; mIndex <= mBuffer.length(); ++mIndex) {
    char ch = safeAt(mBuffer, mIndex);
    bool alpha = isAlpha(ch);
    bool digit = isDigit(ch);
    switch (state) {
      case Nothing:
        if (kOperators.contains(ch)) {
          ++mIndex; // advance
          return {Operator, QByteArray(1, ch)};
        } else if (ch == '_' || alpha) {
          startPos = mIndex;
          state = alpha ? Identifier : Whitespace;
        } else if (digit) {
          // FIXME: Support -/+?
          startPos = mIndex;
          state = Number;
        } else if (ch == '"') {
          startPos = mIndex;
          state = String;
        }
        break;

      case Whitespace:
        if (alpha || digit) {
          state = Identifier;
        } else if (ch != '_') {
          state = Nothing;
        }
        break;

      case Identifier: {
        char nextCh = safeAt(mBuffer, mIndex + 1);
        bool compound = ((ch == '\'' && isAlpha(nextCh)) ||
                         (ch == '-' && (isAlpha(nextCh) || isDigit(nextCh))));
        if (ch != '_' && !alpha && !digit && !compound)
          return {Identifier, mBuffer.mid(startPos, mIndex - startPos)};
        break;
      }

      case Number:
        // FIXME: Support .?
        if (!alpha && !digit)
          return {Number, mBuffer.mid(startPos, mIndex - startPos)};
        break;

      case String:
        if (ch == '"') {
          ++mIndex; // advance
          return {String, mBuffer.mid(startPos, mIndex - startPos)};
        }
        break;

      default:
        Q_ASSERT(false);
    }
  }

  return {Nothing, QByteArray()};
}
