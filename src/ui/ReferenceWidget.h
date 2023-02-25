//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REFERENCEWIDGET_H
#define REFERENCEWIDGET_H

#include "ReferenceView.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include <QFrame>

class QLabel;

class ReferenceWidget : public QFrame {
  Q_OBJECT

public:
  ReferenceWidget(const git::Repository &repo,
                  ReferenceView::Kinds kinds = ReferenceView::AllRefs,
                  QWidget *parent = nullptr);

  git::Reference currentReference() const;
  void select(const git::Reference &ref, bool suppress = false);

signals:
  void referenceChanged(const git::Reference &ref);
  void referenceSelected(const git::Reference &ref);

private:
  void updateLabel(const git::Reference &ref);

private:
  QLabel *mLabel;
  ReferenceView *mView;

  git::Repository mRepo;
  git::Reference mStoredRef;
  bool mSpontaneous = true;
};

#endif
