//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include <QDialog>

class ReferenceList;
class QCheckBox;
class QPushButton;

namespace git {
class Reference;
class Repository;
} // namespace git

class CheckoutDialog : public QDialog {
  Q_OBJECT

public:
  CheckoutDialog(const git::Repository &repo, const git::Reference &ref,
                 QWidget *parent = nullptr);

  git::Reference reference() const;
  bool detach() const { return mDetach; }

private:
  void update(const git::Reference &ref);

  bool mDetach = false;

  ReferenceList *mRefs;
  QCheckBox *mDetachBox;
  QPushButton *mCheckout;
};

#endif
