// https://wiki.qt.io/Spell-Checking-with-Hunspell

#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <QString>

class Hunspell;

class SpellChecker {
public:
  SpellChecker(const QString &dictionaryPath, const QString &userDictionary);
  ~SpellChecker();

  bool spell(const QString &word);
  QStringList suggest(const QString &word);

  void ignoreWord(const QString &word);
  void addToUserDict(const QString &word);
  void removeUserDict(void);

  bool isValid(void) const { return mValid; }

private:
  Hunspell *mHunspell = nullptr;

  QTextCodec *mCodec;
  QString mUserDictionary;

  bool mValid = false;
};

#endif // SPELLCHECKER_H
