//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMAND_H
#define COMMAND_H

#include <QString>

class QProcessEnvironment;

namespace git {

class Command {
public:
  static QString bashPath();
  static QString substitute(const QProcessEnvironment &env,
                            const QString &command);
};

} // namespace git

#endif
