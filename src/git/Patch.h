//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef PATCH_H
#define PATCH_H

#include "Diff.h"
#include "FilterList.h"
#include "git/Commit.h"
#include "git2/patch.h"
#include <QBitArray>
#include <QSharedPointer>

namespace git {

class Blob;
class Id;
class Repository;

class Patch {
public:
  enum ConflictResolution { Unresolved, Ours, Theirs };

  struct LineStats {
    int additions;
    int deletions;
  };

  Patch();

  bool isValid() const { return !d.isNull(); }

  Repository repo() const;

  QString name(Diff::File file = Diff::NewFile) const;
  git_delta_t status() const;
  bool isUntracked() const;
  bool isConflicted() const;
  bool isBinary() const;
  bool isLfsPointer() const;

  Blob blob(Diff::File file);

  LineStats lineStats() const;
  /*!
   * Returns the complete patch as a list of strings.
   * Used for debugging
   * \brief print
   * \return
   */
  QList<QString> print();

  /*!
   * \brief count
   * \return Number of hunks in the patch
   */
  int count() const;
  /*!
   * Header of hunk \p index
   * \brief header
   * \param index Index of hunk
   * \return
   */
  QByteArray header(int hidx) const;

  const git_diff_hunk *header_struct(int hidx) const;

  /*!
   * Number of lines in hunk with index hidx
   * \brief Patch::lineCount
   * \param hidx Index of the hunk
   * \return Number of lines in the hunk
   */
  int lineCount(int hidx) const;
  /*!
   * Origin character of hunk hidx and line line
   * \brief lineOrigin
   * \param hidx Index of the hunnk
   * \param line
   * \return
   */
  char lineOrigin(int hidx, int line) const;
  int lineNumber(int hidx, int line, Diff::File file = Diff::NewFile) const;
  git_off_t contentOffset(int hidx) const;

  /*!
   * Returns the content of the line of hunk hidx and line line
   * \brief lineContent
   * \param hidx
   * \param line
   * \return
   */
  QByteArray lineContent(int hidx, int line) const;

  ConflictResolution conflictResolution(int hidx);
  void setConflictResolution(int hidx, ConflictResolution resolution);

  /*!
   * Reads the filecontent in the blob and stores in image
   * \brief populatePreimage
   * \param image Populated preimage
   */
  void populatePreimage(QList<QList<QByteArray>> &image);
  /*!
   * Splits the content of fileContent into lines and stores the content in
   * image \brief populatePreimage \param image Populated preimage \param
   * fileContent Content of a file in which changes should occur
   */
  static void populatePreimage(QList<QList<QByteArray>> &image,
                               QByteArray fileContent);
  // Apply the given hunk indexes to the old buffer.

  /*!
   * Apply all changes and return the edited file as ByteArray
   * \brief Patch::apply
   * \param hunks
   * \param filters
   * \return edited file
   */
  QByteArray apply(const QBitArray &hunks,
                   const FilterList &filters = FilterList());
  QByteArray apply(int hidx, QByteArray &hunkData, QByteArray fileContent,
                   const FilterList &filters = FilterList()) const;

  /*!
   * Applies changes of a hunk. Used to revert changes
   * Does not change the length of the List, so other changes can be applied too
   * \brief apply
   * \param image
   * \param hidx
   * \param hunkData
   */
  void apply(QList<QList<QByteArray>> &image, int hidx,
             QByteArray &hunkData) const;
  QByteArray generateResult(QList<QList<QByteArray>> &image,
                            const FilterList &filters = FilterList()) const;
  static Patch fromBuffers(const QByteArray &oldBuffer,
                           const QByteArray &newBuffer,
                           const QString &oldPath = QString(),
                           const QString &newPath = QString());

  static void clearConflictResolutions(const Repository &repo);

private:
  /*!
   * Applies changes to hunk and store result in image
   * \brief Patch::apply
   * \param image
   * \param hunk_idx
   * \param start_line
   * \param end_line
   */
  void apply(QList<QList<QByteArray>> &image, int hidx, int start_line,
             int end_line) const;

  struct ConflictHunk {
    int line; // start line
    int min;  // <<<<<<< line
    int mid;  // ======= line
    int max;  // >>>>>>> line
    QList<QByteArray> lines;
  };

  Patch(git_patch *patch);

  Blob mOldBlob;
  Blob mNewBlob;

  QSharedPointer<git_patch> d;
  QList<ConflictHunk> mConflicts;

  friend class Diff;
};

} // namespace git

#endif
