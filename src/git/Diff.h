//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFF_H
#define DIFF_H

#include "Id.h"
#include "Index.h"
#include "git2/diff.h"
#include <QFlags>
#include <QSharedPointer>

/*!
 * \brief containsPath
 * Checks if \p str contains \p occurence from start (if it is a subfile/subfolder of \p occurence)
 * Testcases:
 * /src/testfile.txt, /src/testfile.txt1 - path: /src/testfile.txt --> only /src/testfile.txt1 is shown
 * /src/testfile.txt, /src/testfile.txt1 - path: /src --> testfile.txt and testfile.txt is shown
 * /src/test/test.txt11, /src/testfile.txt, /src/testfile.txt1 - path: /src/test --> only /src/test/testtest.txt11 is shown
 * \param str String in which should be searched
 * \param occurence Search this string in \p str
 * \param cs Case sensitive or not
 * \return True when contains, otherwise false
 */
bool containsPath(QString &str, QString &occurence, Qt::CaseSensitivity cs = Qt::CaseSensitive);


namespace git {

class Patch;

class Diff
{
public:
  enum File
  {
    NewFile,
    OldFile
  };

  enum SortRole
  {
    NameRole,
    StatusRole
  };

  class Callbacks {
  public:
    virtual bool progress(
      const QString &oldPath,
      const QString &newPath)
    {
      return false;
    }

    static int progress(
      const git_diff *diff,
      const char *oldPath,
      const char *newPath,
      void *payload);
  };

  Diff();
  /*!
   * Writes the complete diff into an array.
   * Usefull for testing and checking
   * \brief print
   * \return
   */
  QByteArray print();
  bool isValid() const { return d; }
  explicit operator bool() const { return isValid(); }

  bool isConflicted() const;
  bool isStatusDiff() const;
  Index index() const { return d->index; }

  int count() const;
  Patch patch(int index) const;
  QString name(int index) const;
  bool isBinary(int index) const;
  git_delta_t status(int index) const;
  Id id(int index, File file) const;

  int indexOf(const QString &name) const;

  // Merge the given diff into this diff.
  void merge(const Diff &diff);

  // Detect renames, copies, etc. This is expensive.
  void findSimilar();

  void sort(SortRole role, Qt::SortOrder order = Qt::AscendingOrder);

  void setAllStaged(bool staged, bool yieldFocus = true);

  static char statusChar(git_delta_t status);

private:
  struct Data
  {
    Data(git_diff *diff);
    ~Data();

    void resetMap();
    const git_diff_delta *delta(int index) const;

    git_diff *diff;
    QList<int> map;
    Index index;
  };

  Diff(git_diff *diff);
  operator git_diff *() const;
  void setIndex(const Index &index);

  QSharedPointer<Data> d;

  friend class Commit;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Diff);

#endif
