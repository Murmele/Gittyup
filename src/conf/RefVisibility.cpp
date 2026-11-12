#include "RefVisibility.h"
#include "git/Reference.h"
#include <QDebug>
#include <QFileInfo>

namespace {
const QString kHiddenRefs = "refs.hidden";
}

RefVisibility::Rules::Rules(const QStringList &patterns) {
  for (const QString &pattern : patterns) {
    QString expression = QRegularExpression::escape(pattern);
    expression.replace("\\*", ".*");
    expression.replace("\\?", ".");
    mPatterns.append(
        QRegularExpression(QRegularExpression::anchoredPattern(expression)));
  }
}

bool RefVisibility::Rules::hides(const QString &qualifiedName) const {
  for (const QRegularExpression &pattern : mPatterns) {
    if (pattern.match(qualifiedName).hasMatch())
      return true;
  }
  return false;
}

bool RefVisibility::Rules::hides(const git::Reference &ref) const {
  // Keep the current checkout and stash accessible even with broad rules.
  return ref.isValid() && !ref.isHead() && !ref.isDetachedHead() &&
         !ref.isStash() && hides(ref.qualifiedName());
}

RefVisibility *RefVisibility::instance() {
  static RefVisibility visibility;
  return &visibility;
}

QStringList RefVisibility::defaults() { return {"refs/agents/*"}; }

QString RefVisibility::validationError(const QStringList &patterns) {
  for (const QString &pattern : patterns) {
    if (!pattern.startsWith("refs/") || pattern.size() <= 5 ||
        pattern.contains(QRegularExpression("[\\s\\x00-\\x1f\\x7f\\\\]")))
      return tr("Use full ref names beginning with refs/, without spaces or "
                "backslashes. Only * and ? are wildcards.");
  }
  return QString();
}

std::optional<QStringList>
RefVisibility::configured(const git::Config &config) {
  QString value;
  git::Result result = config.readString(kHiddenRefs, value);
  if (result)
    return value.split('\n', Qt::SkipEmptyParts);
  if (result.error() != GIT_ENOTFOUND)
    qWarning() << "Cannot read ref visibility settings:"
               << result.errorString();
  return std::nullopt;
}

QStringList RefVisibility::patterns(const git::Repository &repo) {
  return configured(repo ? repo.appConfig() : git::Config::appGlobal())
      .value_or(defaults());
}

bool RefVisibility::hasOverride(const git::Repository &repo) {
  const QString path = repo.appDir().filePath("config");
  return QFileInfo::exists(path) &&
         configured(git::Config::open(path)).has_value();
}

git::Result RefVisibility::save(const QStringList &patterns,
                                const git::Repository &repo) {
  const QString error = validationError(patterns);
  if (!error.isEmpty()) {
    qWarning() << error;
    return git::Result(GIT_EINVALIDSPEC, error);
  }

  git::Config config = repo ? repo.appConfig() : git::Config::appGlobal();
  git::Result result = config.writeString(kHiddenRefs, patterns.join('\n'));
  if (result)
    emit instance()->changed();
  return result;
}

git::Result RefVisibility::reset(const git::Repository &repo) {
  git::Config config = repo ? repo.appConfig() : git::Config::appGlobal();
  git::Result result = config.removeEntry(kHiddenRefs);
  if (result.error() == GIT_ENOTFOUND)
    result = git::Result(0);
  if (result)
    emit instance()->changed();
  return result;
}
