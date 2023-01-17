// https://wiki.qt.io/Spell-Checking-with-Hunspell

#include "hunspell.hxx"
#include "SpellChecker.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

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
      QRegularExpression enc_detector("^\\s*SET\\s+([A-Z0-9\\-]+)\\s*",
                                      QRegularExpression::CaseInsensitiveOption);
      QString line = stream.readLine();
      while (!line.isEmpty()) {
        auto match = enc_detector.match(line);
        if (match.hasMatch() >= 0) {
          encoding = match.captured(1);
          break;
        }
        line = stream.readLine();
      }
      affixFile.close();
      mValid = true;
    }

    auto conv = QStringConverter::encodingForName(encoding.toLocal8Bit().data());
    mEncoding = conv ? conv.value() : QStringConverter::System;

    // Add user dictionary words to spell checker.
    if (!mUserDictionary.isEmpty()) {
      QFile userDictonaryFile(mUserDictionary);
      if (userDictonaryFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&userDictonaryFile);
        QString line = stream.readLine();
        while (!line.isEmpty()) {
          QByteArray ba = QStringEncoder{mEncoding}.encode(line);
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
  QByteArray ba = QStringEncoder{mEncoding}.encode(word);
  return mHunspell->spell(ba.toStdString());
}

QStringList SpellChecker::suggest(const QString &word) {
  QStringList suggestions;

  // Retrive suggestions for word.
  QByteArray ba = QStringEncoder{mEncoding}.encode(word);
  std::vector<std::string> suggestion =
      mHunspell->suggest(ba.toStdString());

  // Decode from the encoding used by current dictionary to Unicode.
  auto decoder = QStringDecoder{mEncoding};
  foreach (const std::string &str, suggestion)
    suggestions.append(decoder.decode(str.data()));

  return suggestions;
}

void SpellChecker::ignoreWord(const QString &word) {
  QByteArray ba = QStringEncoder{mEncoding}.encode(word);
  mHunspell->add(ba.toStdString());
}

void SpellChecker::addToUserDict(const QString &word) {
  QByteArray ba = QStringEncoder{mEncoding}.encode(word);
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
