//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BADGE_H
#define BADGE_H

#include <QWidget>

class QStyleOption;

/*!
 * \brief The Badge class
 * Blue widget (M -> Modified, ? -> New added)
 */
class Badge : public QWidget {
public:
  struct Label {
    enum class Type {
      Ref,    // Badges in the CommitList
      Log,    // Badges in the Log View
      Status, // Badges in the TreeView // Changed, Added, Removed, Conflicted,
              // ...
      Conflict, // Conflict badges in the FileWidget next to the status
      LFS,
    };
    Label(Type t, const QString &text = QString(), bool bold = false,
          bool tag = false)
        : text(text), bold(bold), tag(tag) {}

    QString text;
    bool bold;
    bool tag;
    Type type;
  };

  Badge(const QList<Label> &labels, QWidget *parent = nullptr);

  void appendLabel(const Label &label);
  void setLabels(const QList<Label> &labels);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  static QSize size(const QFont &font, const QList<Label> &labels);
  static QSize size(const QFont &font, const Label &label);

  static int paint(QPainter *painter, const QList<Label> &labels,
                   const QRect &rect, QStyleOption *opt = nullptr,
                   Qt::Alignment alignment = Qt::AlignRight);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  static void paint(QPainter *painter, const Label &label, const QRect &rect,
                    bool selected, bool active);

  QList<Label> mLabels;
};

#endif
