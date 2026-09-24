// https://wiki.qt.io/Spell-Checking-with-Hunspell

#include "hunspell.hxx"
#include "SpellChecker.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QTextCodec>

SpellChecker::SpellChecker(const QString &dictionaryPath,
                           const QString &userDictionary)
    : mUserDictionary(userDictionary) {
  QString dictFileName = dictionaryPath + ".dic";
  QString affixFileName = dictionaryPath + ".aff";
  QByteArray dictFilePath = dictFileName.toLocal8Bit();
  QByteArray affixFilePath = affixFileName.toLocal8Bit();

  mValid = false;

  QFile dictFile(dictFileName);
  if (dictFile.exists()) {
    mHunspell =
        new Hunspell(affixFilePath.constData(), dictFilePath.constData());

    // Detect encoding analyzing the SET option in the affix file.
    QString encoding = "ISO8859-15";
    QFile affixFile(affixFileName);
    if (affixFile.open(QIODevice::ReadOnly)) {
      QTextStream stream(&affixFile);
      QRegularExpression enc_detector(
          "^\\s*SET\\s+([A-Z0-9\\-]+)\\s*",
          QRegularExpression::CaseInsensitiveOption);
      QString line = stream.readLine();
      while (!line.isEmpty()) {
        auto match = enc_detector.match(line);
        if (match.hasMatch()) {
          encoding = match.captured(1);
          break;
        }
        line = stream.readLine();
      }
      affixFile.close();
      mValid = true;
    }

    // QTextCodec recognizes the full range of names Hunspell affix files use
    // in their SET option (ISO-8859-1..16, KOI8-R, Windows-125x, etc.), not
    // just the handful QStringConverter knows. Dictionaries without a
    // recognized SET are legacy single-byte ones, so fall back to Latin-1
    // rather than UTF-8.
    mTextCodec = QTextCodec::codecForName(encoding.toLocal8Bit());
    if (!mTextCodec)
      mTextCodec = QTextCodec::codecForName("ISO-8859-1");

    // Add user dictionary words to spell checker.
    if (!mUserDictionary.isEmpty()) {
      QFile userDictonaryFile(mUserDictionary);
      if (userDictonaryFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&userDictonaryFile);
        QString line = stream.readLine();
        while (!line.isEmpty()) {
          QByteArray ba = mTextCodec->fromUnicode(line);
          mHunspell->add(ba.toStdString());
          line = stream.readLine();
        }
        userDictonaryFile.close();
      }
    }
  }
}

SpellChecker::~SpellChecker() { delete mHunspell; }

bool SpellChecker::spell(const QString &word) {
  // Encode from Unicode to the encoding used by current dictionary.
  QByteArray ba = mTextCodec->fromUnicode(word);
  return mHunspell->spell(ba.toStdString());
}

QStringList SpellChecker::suggest(const QString &word) {
  QStringList suggestions;

  // Retrive suggestions for word.
  QByteArray ba = mTextCodec->fromUnicode(word);
  std::vector<std::string> suggestion = mHunspell->suggest(ba.toStdString());

  // Decode from the encoding used by current dictionary to Unicode.
  for (const std::string &str : suggestion)
    suggestions.append(mTextCodec->toUnicode(str.data()));

  return suggestions;
}

void SpellChecker::ignoreWord(const QString &word) {
  QByteArray ba = mTextCodec->fromUnicode(word);
  mHunspell->add(ba.toStdString());
}

void SpellChecker::addToUserDict(const QString &word) {
  QByteArray ba = mTextCodec->fromUnicode(word);
  mHunspell->add(ba.toStdString());

  if (!mUserDictionary.isEmpty()) {
    QFile userDictonaryFile(mUserDictionary);
    if (userDictonaryFile.open(QIODevice::Append)) {
      QTextStream stream(&userDictonaryFile);
      stream << word << "\n";
      userDictonaryFile.close();
    }
  }
}

void SpellChecker::removeUserDict(void) {
  if (!mUserDictionary.isEmpty()) {
    QFile userDictonaryFile(mUserDictionary);
    userDictonaryFile.resize(0);
  }
}
