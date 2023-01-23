//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REFERENCELIST_H
#define REFERENCELIST_H

#include "ReferenceView.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include <QComboBox>

class ReferenceList : public QComboBox {
  Q_OBJECT

public:
  ReferenceList(const git::Repository &repo,
                ReferenceView::Kinds kinds = ReferenceView::AllRefs,
                QWidget *parent = nullptr);

  git::Commit target() const;
  git::Reference currentReference() const;

  void setCommit(const git::Commit &commit);
  void select(const git::Reference &ref, bool spontaneous = true);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

signals:
  void referenceSelected(const git::Reference &ref);

protected:
  void wheelEvent(QWheelEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  QSize calculateSizeHint() const;
  void initIndex();

  git::Repository mRepo;
  git::Reference mStoredRef;

  // direct commit
  git::Commit mCommit;

  ReferenceView *mView;
};

#endif
