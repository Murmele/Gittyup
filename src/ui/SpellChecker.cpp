// https://wiki.qt.io/Spell-Checking-with-Hunspell

#include "hunspell.hxx"
#include "SpellChecker.h"
#include <QFile>
#include <QTextStream>
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
      QRegExp enc_detector("^\\s*SET\\s+([A-Z0-9\\-]+)\\s*",
                           Qt::CaseInsensitive);
      QString line = stream.readLine();
      while (!line.isEmpty()) {
        if (enc_detector.indexIn(line) >= 0) {
          encoding = enc_detector.cap(1);
          break;
        }
        line = stream.readLine();
      }
      affixFile.close();
      mValid = true;
    }

    mCodec = QTextCodec::codecForName(encoding.toLatin1().constData());

    // Add user dictionary words to spell checker.
    if (!mUserDictionary.isEmpty()) {
      QFile userDictonaryFile(mUserDictionary);
      if (userDictonaryFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&userDictonaryFile);
        QString line = stream.readLine();
        while (!line.isEmpty()) {
          mHunspell->add(mCodec->fromUnicode(line).constData());
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
  return mHunspell->spell(mCodec->fromUnicode(word).toStdString());
}

QStringList SpellChecker::suggest(const QString &word) {
  QStringList suggestions;

  // Retrive suggestions for word.
  std::vector<std::string> suggestion =
      mHunspell->suggest(mCodec->fromUnicode(word).toStdString());

  // Decode from the encoding used by current dictionary to Unicode.
  foreach (const std::string &str, suggestion)
    suggestions.append(mCodec->toUnicode(str.data()));

  return suggestions;
}

void SpellChecker::ignoreWord(const QString &word) {
  mHunspell->add(mCodec->fromUnicode(word).constData());
}

void SpellChecker::addToUserDict(const QString &word) {
  mHunspell->add(mCodec->fromUnicode(word).constData());

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
