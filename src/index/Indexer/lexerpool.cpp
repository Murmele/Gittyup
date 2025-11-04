#include "lexerpool.h"
#include "index/LPegLexer.h"

LexerPool::LexerPool() : mHome(Settings::lexerDir().path().toUtf8()) {}

Lexer* LexerPool::acquire(const QByteArray &name) {
  QMutexLocker locker(&mMutex);
  (void)locker;

  return mLexers.contains(name) ? mLexers.take(name)
                                : new LPegLexer(mHome, name);
}

void LexerPool::release(Lexer *lexer) {
  QMutexLocker locker(&mMutex);
  (void)locker;

  mLexers.insert(lexer->name(), lexer);
}
