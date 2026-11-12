#ifndef REFVISIBILITY_H
#define REFVISIBILITY_H

#include "git/Config.h"
#include <QObject>
#include <QRegularExpression>
#include <optional>

namespace git {
class Reference;
}

class RefVisibility : public QObject {
  Q_OBJECT

public:
  class Rules {
  public:
    explicit Rules(const QStringList &patterns);
    bool hides(const QString &qualifiedName) const;
    bool hides(const git::Reference &ref) const;

  private:
    QList<QRegularExpression> mPatterns;
  };

  static RefVisibility *instance();
  static QStringList defaults();
  static QString validationError(const QStringList &patterns);
  static std::optional<QStringList> configured(const git::Config &config);
  static QStringList patterns(const git::Repository &repo = git::Repository());
  static bool hasOverride(const git::Repository &repo);
  static git::Result save(const QStringList &patterns,
                          const git::Repository &repo = git::Repository());
  static git::Result reset(const git::Repository &repo = git::Repository());

signals:
  void changed();

private:
  RefVisibility() = default;
};

#endif
