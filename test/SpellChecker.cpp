#include "Test.h"

#include "ui/SpellChecker.h"
#include <QFile>

using namespace QTest;

namespace {

// Writes a minimal Hunspell affix/dictionary pair (no affix rules, just a
// flat word list) so tests can control the exact bytes Hunspell sees.
QString writeDictionary(const QTemporaryDir &dir, const QString &name,
                        const QByteArray &affContents,
                        const QByteArray &dicContents) {
  QString base = dir.filePath(name);

  QFile aff(base + ".aff");
  (void)aff.open(QIODevice::WriteOnly);
  aff.write(affContents);
  aff.close();

  QFile dic(base + ".dic");
  (void)dic.open(QIODevice::WriteOnly);
  dic.write(dicContents);
  dic.close();

  return base;
}

} // namespace

class TestSpellChecker : public QObject {
  Q_OBJECT

private slots:
  void detects_set_option_after_leading_comments();
  void converts_words_outside_the_latin1_charset();
};

// Regression test for a bug where the SET option was only ever recognized on
// the first non-empty line of the affix file: a stray `>= 0` on
// QRegularExpressionMatch::hasMatch() made the "does this line match" check
// always true, so the loop stopped after the very first (non-matching)
// comment line and silently kept scanning no further, discarding the real
// encoding.
void TestSpellChecker::detects_set_option_after_leading_comments() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  QByteArray aff = "# a comment line before the SET option\n"
                   "SET UTF-8\n";
  QByteArray dic = "1\n" + QString::fromUtf8("caf\xC3\xA9").toUtf8() + "\n";

  QString path = writeDictionary(dir, "leading_comment", aff, dic);
  SpellChecker checker(path, QString());

  QVERIFY(checker.isValid());
  QVERIFY(checker.spell(QString::fromUtf8("caf\xC3\xA9"))); // "café"
  QVERIFY(!checker.spell("nonexistentword"));
}

// Regression test for a bug where dictionaries without a recognized SET
// option (such as the bundled de_DE_frami affix file, which has no SET line
// at all) silently fell back to the system encoding (UTF-8 on Linux)
// instead of the single-byte encoding their .dic file actually uses. Byte
// 0xBD is the "oe" ligature (U+0153) in ISO-8859-15/Latin-9, but the
// fraction "1/2" in Latin-1/UTF-8, so this only round-trips correctly if
// the fallback encoding is genuinely ISO-8859-15.
void TestSpellChecker::converts_words_outside_the_latin1_charset() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  QByteArray aff = "# no SET option on purpose, like de_DE_frami.aff\n";
  QByteArray dic;
  dic += "1\n";
  dic += 's';
  dic += char(0xBD); // "oe" ligature (U+0153) in ISO-8859-15
  dic += "ur\n";

  QString path = writeDictionary(dir, "no_set", aff, dic);
  SpellChecker checker(path, QString());

  QVERIFY(checker.isValid());
  QVERIFY(checker.spell(QString::fromUtf8("s\xC5\x93ur")));   // "sœur"
  QVERIFY(!checker.spell(QString::fromUtf8("s\xC5\x93urs"))); // "sœurs"
}

TEST_MAIN(TestSpellChecker)

#include "SpellChecker.moc"
