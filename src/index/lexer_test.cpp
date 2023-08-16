//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GenericLexer.h"
#include "LPegLexer.h"
#include "conf/Settings.h"
#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QStringList>
#include <QTextStream>

namespace {

const QStringList kStyleNames = {
    "nothing",  "whitespace", "comment",  "string", "number",
    "keyword",  "identifier", "operator", "error",  "preprocessor",
    "constant", "variable",   "function", "class",  "type",
    "label",    "regex",      "embedded"};

void print(QTextStream &out, const Lexer::Lexeme &lexeme, int indent = 0) {
  out << QByteArray(indent, ' ');
  out << lexeme.text << " - " << lexeme.token;
  if (lexeme.token < kStyleNames.length())
    out << " (" << kStyleNames.at(lexeme.token) << ")";
  out << Qt::endl;
}

void print(QTextStream &out, Lexer *lexer, int indent = 0) {
  while (lexer->hasNext()) {
    Lexer::Lexeme lexeme = lexer->next();
    QByteArray text = lexeme.text;
    switch (lexeme.token) {
      // Lex further.
      case Lexer::String:
        text.remove(0, 1);
        text.chop(1);
        // fall through

      case Lexer::Comment:
      case Lexer::Preprocessor:
      case Lexer::Constant:
      case Lexer::Variable:
      case Lexer::Function:
      case Lexer::Class:
      case Lexer::Type:
      case Lexer::Label: {
        print(out, lexeme, indent);
        GenericLexer sublexer;
        if (sublexer.lex(text))
          print(out, &sublexer, indent + 2);
        break;
      }

      // Print directly.
      case Lexer::Keyword:
      case Lexer::Identifier:
        print(out, lexeme, indent);
        break;

      // Ignore everything else.
      default:
        break;
    }
  }
}

} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QStringList args = app.arguments();
  args.removeFirst(); // program name

  QMap<QByteArray, Lexer *> lexers;
  QByteArray home = Settings::lexerDir().path().toUtf8();

  GenericLexer generic;
  lexers.insert("null", &generic);

  QTextStream out(stdout);
  foreach (const QString &arg, args) {
    // Open file.
    QFile file(arg);
    if (!file.open(QIODevice::ReadOnly))
      continue;

    // Read file.
    QByteArray buffer = file.readAll();

    // Look up lexer.
    QByteArray name = Settings::instance()->lexer(arg).toUtf8();
    if (!lexers.contains(name))
      lexers.insert(name, new LPegLexer(home, name, &generic));

    // Lex buffer.
    Lexer *lexer = lexers.value(name);
    if (lexer->lex(buffer)) {
      out << name << " - " << arg << ":" << Qt::endl;
      print(out, lexer);
    }
  }

  return 0;
}
