#ifndef LINE_H
#define LINE_H

#include <QString>

class Line {
public:
  Line(char origin, int oldLine, int newLine) : mOrigin(origin) {
    mOldLine = (oldLine >= 0) ? QByteArray::number(oldLine) : QByteArray();
    mNewLine = (newLine >= 0) ? QByteArray::number(newLine) : QByteArray();
  }

  char origin() const { return mOrigin; }
  QByteArray oldLine() const { return mOldLine; }
  QByteArray newLine() const { return mNewLine; }

  bool newline() const { return mNewline; }
  void setNewline(bool newline) { mNewline = newline; }

  int matchingLine() const { return mMatchingLine; }
  void setMatchingLine(int line) { mMatchingLine = line; }

  QString print() {
    return QString("Origin: ") + mOrigin + QString("; OldLine: ") +
           QString(mOldLine) + QString("; NewLine: ") + QString(mNewLine);
  }

private:
  char mOrigin = -1;
  bool mNewline = true;
  int mMatchingLine = -1;
  QByteArray mOldLine;
  QByteArray mNewLine;
};

#endif // LINE_H
