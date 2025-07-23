//
//          Copyright (c) 2025
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#ifndef MMAPFILEREADER_H
#define MMAPFILEREADER_H

#include <memory>
#include <QFile>
#include <QtEndian>

/// @brief Reads file by memory mapping it and skipping all Qt layers to provide
/// better performance
class MmapFileReader : public QObject {
private:
  QFile proxInFile;
  quint8 *data;
  const quint8 *ptr;
  const quint8 *end;

public:
  MmapFileReader(QString filepath);
  ~MmapFileReader();
  bool open();
  void close();
  void seek(quint32 pos);
  quint32 readVInt();

  template <class T> void read(T &value) {
    std::memcpy(&value, ptr, sizeof(T));
    ptr += sizeof(T);
  }

  MmapFileReader &operator>>(quint8 &obj) {
    read(obj);
    return *this;
  }

  MmapFileReader &operator>>(quint32 &obj) {
    read(obj);
    // Convert from big endian to machine endian to mimic default
    //  QDataStream behaviour
    obj = qFromBigEndian(obj);
    return *this;
  }
};

#endif