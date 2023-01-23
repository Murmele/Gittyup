//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "git/Repository.h"
#include <QObject>
#include <QSharedPointer>
#include <QVariant>

class TextEditor;

typedef struct lua_State lua_State;

using PluginRef = QSharedPointer<class Plugin>;

class Plugin : public QObject {
  Q_OBJECT

public:
  enum OptionKind { Boolean, Integer, String, List };

  enum DiagnosticKind { Note, Warning, Error };

  Plugin(const QString &file, const git::Repository &repo = git::Repository(),
         QObject *parent = nullptr);
  ~Plugin() override;

  bool isValid() const;
  QString name() const;
  QString scriptDir() const;
  QString errorString() const;

  bool isEnabled() const;
  bool isEnabled(const QString &key) const;
  void setEnabled(const QString &key, bool enabled);

  void defineOption(const QString &key, OptionKind kind, const QString &text,
                    const QVariant &value,
                    const QStringList &opts = QStringList());
  void setOptionValue(const QString &key, const QVariant &value);

  QStringList optionKeys() const;
  QString optionText(const QString &key) const;
  QVariant optionValue(const QString &key) const;
  OptionKind optionKind(const QString &key) const;
  QStringList optionOpts(const QString &key) const;

  void defineDiagnostic(const QString &key, DiagnosticKind kind,
                        const QString &name, const QString &msg,
                        const QString &desc, bool enabled = false);
  void setDiagnosticKind(const QString &key, DiagnosticKind kind);

  QStringList diagnosticKeys() const;
  DiagnosticKind diagnosticKind(const QString &key) const;
  QString diagnosticName(const QString &key) const;
  QString diagnosticMessage(const QString &key) const;
  QString diagnosticDescription(const QString &key) const;

  bool hunk(TextEditor *editor) const;

  static QList<PluginRef>
  plugins(const git::Repository &repo = git::Repository());

signals:
  void error(const QString &msg) const;

private:
  struct Option {
    OptionKind kind;
    QString text;
    QVariant value;
    QStringList opts;
  };

  struct Diagnostic {
    DiagnosticKind kind;
    QString name;
    QString message;
    QString description;
    bool enabled;
  };

  git::Config config() const;
  void setError(const QString &err);

  git::Repository mRepo;

  lua_State *L;
  QString mDir;
  QString mName;
  QString mError;

  QMap<QString, Option> mOptions;
  QMap<QString, Diagnostic> mDiagnostics;
};
