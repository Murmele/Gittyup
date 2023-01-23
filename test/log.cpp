//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "log/LogEntry.h"
#include "log/LogView.h"
#include <QtTest/QtTest>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QTextEdit>

using namespace QTest;

class TestLog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void copy();
  void copyAll();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  QSplitter *mSplitter = nullptr;
  bool firstEntry;
  void copyEachEntry(LogView *logView, QList<QAbstractScrollArea *> qTextEdits,
                     int entries);
};

void TestLog::initTestCase() {
  // Create LogView
  LogEntry *rootEntry = new LogEntry;
  rootEntry->addEntry("Entry", "Title");
  for (LogEntry::Kind kind :
       {LogEntry::File, LogEntry::Hint, LogEntry::Warning, LogEntry::Error})
    rootEntry->addEntry(kind, "Entry");
  LogEntry *nestedEntry = rootEntry->addEntry("Entry", "Nested 1");
  nestedEntry->addEntry(LogEntry::Entry, "Nested 2")
      ->addEntry(LogEntry::File, "Nested 3")
      ->addEntry(LogEntry::File, "Message");

  LogView *logView = new LogView(rootEntry);
  logView->expandAll();
  logView->show();
  QVERIFY(qWaitForWindowActive(logView));

  // Simulate global copy shortcut (Ctrl + C) available in application context
  QShortcut *copy = new QShortcut(QKeySequence::Copy, logView);
  connect(copy, &QShortcut::activated, logView, &LogView::copy);

  // Create plain text edit box
  QPlainTextEdit *plainTextEdit = new QPlainTextEdit();
  plainTextEdit->show();
  QVERIFY(qWaitForWindowActive(plainTextEdit));

  // Create rich text edit box
  QTextEdit *richTextEdit = new QTextEdit();
  richTextEdit->show();
  QVERIFY(qWaitForWindowActive(richTextEdit));

  // Assemble parent widget
  mSplitter = new QSplitter();
  mSplitter->addWidget(logView);
  mSplitter->addWidget(plainTextEdit);
  mSplitter->addWidget(richTextEdit);
  mSplitter->setMinimumHeight(400);
  mSplitter->show();
  QVERIFY(qWaitForWindowActive(mSplitter));
}

void TestLog::copy() {
  LogView *logView = mSplitter->findChild<LogView *>();
  QList<LogEntry *> entries = logView->model()->findChildren<LogEntry *>();
  QList<QAbstractScrollArea *> qTextEdits =
      mSplitter->findChildren<QAbstractScrollArea *>();
  copyEachEntry(logView, qTextEdits, entries.size());
}

void TestLog::copyAll() {
  LogView *logView = mSplitter->findChild<LogView *>();
  QList<LogEntry *> entries = logView->model()->findChildren<LogEntry *>();
  QList<QAbstractScrollArea *> qTextEdits =
      mSplitter->findChildren<QAbstractScrollArea *>();
  QAbstractScrollArea *plainTextEditor = qTextEdits.at(1);
  QAbstractScrollArea *richTextEditor = qTextEdits.at(0);
  for (int i = 1; i < entries.size() - 1; i++) {
    keyClick(logView, Qt::Key_Up, Qt::ShiftModifier, inputDelay);
  }
  keyClick(logView, 'c', Qt::ControlModifier, inputDelay);
  keyClick(plainTextEditor, Qt::Key_Return);
  keyClick(plainTextEditor, 'v', Qt::ControlModifier, inputDelay);
  keyClick(richTextEditor, Qt::Key_Return);
  keyClick(richTextEditor, 'v', Qt::ControlModifier, inputDelay);
}

void TestLog::copyEachEntry(LogView *logView,
                            QList<QAbstractScrollArea *> qTextEdits,
                            int entries) {
  if (firstEntry) {
    keyClick(logView, Qt::Key_Down);
    keyClick(logView, Qt::Key_Up);
    firstEntry = false;
  } else {
    keyClick(logView, Qt::Key_Down);
  }
  keyClick(logView, 'c', Qt::ControlModifier, inputDelay);
  keyClick(qTextEdits.at(1), 'v', Qt::ControlModifier, inputDelay);
  keyClick(qTextEdits.at(0), 'v', Qt::ControlModifier, inputDelay);
  entries--;
  if (entries == 1) // Skip root entry
    return;
  copyEachEntry(logView, qTextEdits, entries);
}

void TestLog::cleanupTestCase() {
  qWait(closeDelay);
  mSplitter->close();
}

TEST_MAIN(TestLog)

#include "log.moc"
