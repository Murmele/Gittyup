//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef CUSTOMTHEME_H
#define CUSTOMTHEME_H

#include "Theme.h"
#include <QApplication>
#include <QDir>
#include <QPalette>

class CustomTheme : public Theme
{
public:
  CustomTheme(const QString &name);

  QDir dir() const override;
  QString name() const override;
  QStyle *style() const override;
  QString styleSheet() const override;
  void polish(QPalette &palette) const override;

  QColor badge(BadgeRole role, BadgeState state) override;
  QList<QColor> branchTopologyEdges() override;
  QColor buttonChecked() override;
  QPalette commitList() override;
  QColor commitEditor(CommitEditor color) override;
  QColor diff(Diff color) override;
  QColor heatMap(HeatMap color) override;
  QColor remoteComment(Comment color) override;
  QColor star() override;

  QVariantMap checkbox() const;
  void polishWindow(QWindow *window) const;

  static void drawCloseButton(
    const QStyleOption *option,
    QPainter *painter);

  static QDir userDir(bool create = false, bool *exists = nullptr);
  static bool isValid(const QString &name);

private:
  QString mName;
  QDir mDir;
  QVariantMap mMap;
};

#endif
