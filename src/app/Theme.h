//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef THEME_H
#define THEME_H

#include <QDir>
#include <QPalette>
#include <QString>
#include <QMap>

class QStyle;
class QStyleOption;

class Theme {
public:
  enum class BadgeRole { Foreground, Background };

  enum class BadgeState { Normal, Selected, Conflicted, Head, Notification };

  enum class CommitEditor { SpellError, SpellIgnore, LengthWarning };

  enum class Diff {
    Ours,
    Theirs,
    Addition,
    Deletion,

    WordAddition,
    WordDeletion,
    Plus,
    Minus,

    Note,
    Warning,
    Error
  };

  enum class HeatMap { Hot, Cold };

  enum class Comment { Background, Body, Author, Timestamp };

  Theme();
  virtual ~Theme() = default;

  QString diffButtonStyle(Diff role);

  virtual QDir dir() const;
  virtual QString name() const;
  virtual QStyle *style() const;
  virtual QString styleSheet() const;
  virtual void polish(QPalette &palette) const;

  virtual QColor badge(BadgeRole role, BadgeState state);
  virtual QList<QColor> branchTopologyEdges();
  virtual QColor buttonChecked();
  virtual QPalette commitList();
  virtual QColor commitEditor(CommitEditor color);
  virtual QColor diff(Diff color);
  virtual QColor heatMap(HeatMap color);
  virtual QColor remoteComment(Comment color);
  virtual QColor star();

  static Theme *create(const QString &name = QString());

private:
  bool mDark;
  QString mName;
  QDir mDir;
  QVariantMap mMap;
};

#endif
