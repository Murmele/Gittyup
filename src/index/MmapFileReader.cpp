//
//          Copyright (c) 2025
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#include "MmapFileReader.h"
#include <QIODevice>

MmapFileReader::MmapFileReader(QString filepath)
    : proxInFile(filepath), ptr(nullptr), end(nullptr) {}

MmapFileReader::~MmapFileReader() { close(); }

bool MmapFileReader::open() {
  if (proxInFile.open(QIODevice::ReadOnly)) {
    quint64 size = proxInFile.size();
    data = proxInFile.map(0, size);
    if (data == nullptr) {
      throw std::runtime_error("Failed to map file");
    }
    ptr = data;
    end = ptr + size;
    return true;
  }
  return false;
}

void MmapFileReader::close() {
  if (proxInFile.isOpen()) {
    proxInFile.unmap(data);
    proxInFile.close();
  }
}

void MmapFileReader::seek(quint32 pos) { ptr = data + pos; }

quint32 MmapFileReader::readVInt() {
  quint32 result = 0;
  quint32 shift = 0;
  quint8 byte;

  do {
    byte = *ptr++;
    result |= (byte & 0x7F) << shift;
    shift += 7;
  } while (byte & 0x80);
  return result;
}
