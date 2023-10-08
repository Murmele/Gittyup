//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Query.h"
#include "GenericLexer.h"
#include <QDate>
#include <QMap>
#include <QSet>
#include <QRegularExpression>

namespace {

class StarredQuery : public Query {
public:
  QString toString() const override { return "is:starred"; }

  QList<Index::Term> terms() const override { return QList<Index::Term>(); }

  QList<git::Commit> commits(const Index *index) const override {
    return index->repo().starredCommits();
  }
};

class TermQuery : public Query {
public:
  TermQuery(const Index::Term &term) : mTerm(term) {}

  QString toString() const override {
    return QString("%1:%2").arg(Index::fieldName(mTerm.field), mTerm.text);
  }

  QList<Index::Term> terms() const override { return {mTerm}; }

  QList<git::Commit> commits(const Index *index) const override {
    return index->commits(index->postings(mTerm));
  }

protected:
  Index::Term mTerm;
};

class DateRangeQuery : public TermQuery {
public:
  DateRangeQuery(const Index::Term &term) : TermQuery(term) {}

  QList<git::Commit> commits(const Index *index) const override {
    Index::Field field = mTerm.field;
    if (field != Index::Before && field != Index::After)
      return QList<git::Commit>();

    QDate date = QDate::fromString(mTerm.text, Index::dateFormat());
    if (!date.isValid())
      return QList<git::Commit>();

    Index::Predicate pred = [field, date](const QByteArray &word) -> bool {
      // Skip words that don't look like dates.
      if (!word.startsWith("19") && !word.startsWith("20"))
        return false;

      // Convert to date.
      QDate tmp = QDate::fromString(word, Index::dateFormat());
      if (!tmp.isValid())
        return false;

      // Compare dates.
      switch (field) {
        case Index::Before:
          return (tmp < date);
        case Index::After:
          return (tmp > date);
        default:
          Q_ASSERT(false);
          return false;
      }
    };

    return index->commits(index->postings(pred, Index::Date));
  }
};

class WildcardQuery : public TermQuery {
public:
  WildcardQuery(const Index::Term &term) : TermQuery(term) {}

  QList<git::Commit> commits(const Index *index) const override {
    QRegularExpression re{
        QRegularExpression::fromWildcard(mTerm.text, Qt::CaseInsensitive)};
    Index::Predicate pred = [re](const QByteArray &word) {
      return re.match(word).hasMatch();
    };

    return index->commits(index->postings(pred, mTerm.field));
  }
};

class PhraseQuery : public Query {
public:
  PhraseQuery(const QList<Index::Term> &terms) : mTerms(terms) {}

  QString toString() const override {
    QStringList terms;
    foreach (const Index::Term &term, mTerms)
      terms.append(term.text);
    Index::Field field = mTerms.first().field;
    return QString("%1:\"%2\"").arg(Index::fieldName(field), terms.join(" "));
  }

  QList<Index::Term> terms() const override { return mTerms; }

  QList<git::Commit> commits(const Index *index) const override {
    if (mTerms.isEmpty())
      return QList<git::Commit>();

    // Start with the commits that match the first term.
    bool multiple = (mTerms.size() > 1);
    QList<Index::Posting> postings = index->postings(mTerms.first(), multiple);
    if (!multiple)
      return index->commits(postings);

    // Remove commits that don't match subsequent terms.
    int offset = 1;
    for (int i = 1; i < mTerms.size(); ++i) {
      const Index::Term &term = mTerms.at(i);
      QMap<quint32, QMap<quint8, QSet<quint32>>> ids;
      foreach (const Index::Posting &posting, index->postings(term, true)) {
        foreach (quint32 pos, posting.positions)
          ids[posting.id][posting.field].insert(pos);
      }

      QMutableListIterator<Index::Posting> it(postings);
      while (it.hasNext()) {
        const Index::Posting &posting = it.next();
        if (!ids.contains(posting.id)) {
          it.remove();
        } else {
          bool found = false;
          foreach (quint32 pos, posting.positions) {
            if (ids[posting.id][posting.field].contains(pos + offset)) {
              found = true;
              break;
            }
          }

          if (!found)
            it.remove();
        }
      }

      ++offset;
    }

    return index->commits(postings);
  }

private:
  QList<Index::Term> mTerms;
};

class BooleanQuery : public Query {
public:
  enum Kind { And, Or };

  BooleanQuery(Kind kind, const QueryRef &lhs, const QueryRef &rhs)
      : mKind(kind), mLhs(lhs), mRhs(rhs) {}

  QString toString() const override {
    QString kind = (mKind == And) ? "AND" : "OR";
    return QString("%1 %2 %3").arg(mLhs->toString(), kind, mRhs->toString());
  }

  QList<Index::Term> terms() const override {
    return mLhs->terms() + mRhs->terms();
  }

  QList<git::Commit> commits(const Index *index) const override {
    // Start with the commits that match the left hand side.
    QList<git::Commit> rhs = mRhs->commits(index);
    QList<git::Commit> commits = mLhs->commits(index);
    if (mKind == And) {
      // Remove commits that don't match the right hand side.
      QSet<git::Commit> set(rhs.begin(), rhs.end());
#else
#endif
      QMutableListIterator<git::Commit> it(commits);
      while (it.hasNext()) {
        if (!set.contains(it.next()))
          it.remove();
      }
    } else {
      // Add commits that aren't already in the result set.
      QSet<git::Commit> set(commits.begin(), commits.end());
#else
#endif
      foreach (const git::Commit &commit, rhs) {
        if (!set.contains(commit))
          commits.append(commit);
      }
    }

    return commits;
  }

private:
  Kind mKind;
  QueryRef mLhs;
  QueryRef mRhs;
};

class PathspecQuery : public TermQuery {
public:
  PathspecQuery(const Index::Term &term) : TermQuery(term) {}

  QList<git::Commit> commits(const Index *index) const override {
    QByteArray term = mTerm.text.toUtf8();
    QByteArray prefix = term.endsWith('/') ? term : term + '/';
    QRegularExpression re{
        QRegularExpression::fromWildcard(mTerm.text, Qt::CaseInsensitive)};
    Index::Predicate pred = [prefix, re](const QByteArray &word) {
      return word.startsWith(prefix) || re.match(word).hasMatch();
    };

    return index->commits(index->postings(pred, Index::Path));
  }
};

bool isOperator(const Lexer::Lexeme &lexeme, const QByteArray &chars) {
  return (lexeme.token == Lexer::Operator && chars.contains(lexeme.text));
}

QueryRef parse(QList<Lexer::Lexeme> &lexemes, Index::Field start = Index::Any) {
  QueryRef result;
  while (!lexemes.isEmpty()) {
    QueryRef query;
    Lexer::Lexeme lexeme = lexemes.takeFirst();

    // Parse OR operator.
    BooleanQuery::Kind kind = BooleanQuery::And;
    if (lexeme.token == Lexer::Identifier && lexeme.text == "OR") {
      // Advance to next token.
      if (lexemes.isEmpty())
        break;
      lexeme = lexemes.takeFirst();

      // Set kind.
      kind = BooleanQuery::Or;
    }

    // Parse field qualifier.
    Index::Field field = start;
    if (!lexemes.isEmpty() && isOperator(lexemes.first(), ":")) {
      QByteArray key = lexeme.text;

      // Discard : and advance to next token.
      lexemes.removeFirst();
      if (lexemes.isEmpty())
        break;
      lexeme = lexemes.takeFirst();

      // Set field.
      if (key == Index::fieldName(Index::Id)) {
        field = Index::Id;
      } else if (key == Index::fieldName(Index::Author)) {
        field = Index::Author;
      } else if (key == Index::fieldName(Index::Email)) {
        field = Index::Email;
      } else if (key == Index::fieldName(Index::Message) || key == "msg") {
        field = Index::Message;
      } else if (key == Index::fieldName(Index::Date)) {
        field = Index::Date;
      } else if (key == Index::fieldName(Index::Path)) {
        field = Index::Path;
      } else if (key == Index::fieldName(Index::File)) {
        field = Index::File;
      } else if (key == Index::fieldName(Index::Scope)) {
        field = Index::Scope;
      } else if (key == Index::fieldName(Index::Context)) {
        field = Index::Context;
      } else if (key == Index::fieldName(Index::Addition) || key == "added") {
        field = Index::Addition;
      } else if (key == Index::fieldName(Index::Deletion) || key == "deleted") {
        field = Index::Deletion;
      } else if (key == Index::fieldName(Index::Comment)) {
        field = Index::Comment;
      } else if (key == Index::fieldName(Index::String)) {
        field = Index::String;
      } else if (key == Index::fieldName(Index::Identifier) || key == "ident") {
        field = Index::Identifier;
      } else if (key == Index::fieldName(Index::Is)) {
        field = Index::Is;
      } else if (key == Index::fieldName(Index::Before)) {
        field = Index::Before;
      } else if (key == Index::fieldName(Index::After)) {
        field = Index::After;
      } else if (key == Index::fieldName(Index::Pathspec)) {
        field = Index::Pathspec;
      } else {
        continue;
      }
    }

    if (isOperator(lexeme, "(")) {
      query = parse(lexemes, field);
    } else if (isOperator(lexeme, ")")) {
      return result;
    } else if (lexeme.token == Lexer::Number) {
      // Parse date.
      QByteArray text = lexeme.text;
      while (!lexemes.isEmpty() && isOperator(lexemes.first(), "/")) {
        text += lexemes.takeFirst().text;
        if (lexemes.isEmpty() || lexemes.first().token != Lexer::Number)
          break;
        text += lexemes.takeFirst().text;
      }

      Index::Term term(field, text);
      if (field == Index::Before || field == Index::After) {
        query = QSharedPointer<DateRangeQuery>::create(term);
      } else {
        query = QSharedPointer<TermQuery>::create(term);
      }

    } else if (lexeme.token == Lexer::String) {
      // Strip off quotes.
      QByteArray unquoted = lexeme.text.mid(1);
      unquoted.chop(1);

      GenericLexer lexer;
      if (lexer.lex(unquoted)) {
        QList<Index::Term> terms;
        while (lexer.hasNext()) {
          Lexer::Lexeme lexeme = lexer.next();
          if (lexeme.token == Lexer::Identifier)
            terms.append({field, lexeme.text});
        }

        if (!terms.isEmpty())
          query = QSharedPointer<PhraseQuery>::create(terms);
      }

    } else if (lexeme.token == Lexer::Identifier) {
      // Parse file path and wildcards.
      QByteArray text = lexeme.text;
      while (!lexemes.isEmpty() && isOperator(lexemes.first(), "/.*?")) {
        while (!lexemes.isEmpty() && isOperator(lexemes.first(), "/.*?"))
          text += lexemes.takeFirst().text;
        if (!lexemes.isEmpty() && lexemes.first().token == Lexer::Identifier)
          text += lexemes.takeFirst().text;
      }

      Index::Term term(field, text);
      if (field == Index::Is) {
        if (text == "starred")
          query = QSharedPointer<StarredQuery>::create();
      } else if (field == Index::Pathspec) {
        query = QSharedPointer<PathspecQuery>::create(term);
      } else if (text.contains('*') || text.contains('?')) {
        query = QSharedPointer<WildcardQuery>::create(term);
      } else {
        query = QSharedPointer<TermQuery>::create(term);
      }
    }

    // Form boolean query.
    if (query)
      result = !result ? query
                       : QueryRef(QSharedPointer<BooleanQuery>::create(
                             kind, result, query));
  }

  return result;
}

} // namespace

QueryRef Query::parseQuery(const QString &query) {
  // Parse into list of terms.
  // Handle key:() and key:"" with embedded spaces.
  GenericLexer lexer;
  QList<Lexer::Lexeme> lexemes;
  if (lexer.lex(query.toUtf8())) {
    while (lexer.hasNext())
      lexemes.append(lexer.next());
  }

  return parse(lexemes);
}
