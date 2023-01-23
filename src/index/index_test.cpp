//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "IndexModel.h"
#include "git/Repository.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTreeView>

namespace {

class RepoInit {
public:
  RepoInit() { git::Repository::init(); }

  ~RepoInit() { git::Repository::shutdown(); }
};

} // namespace

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addPositionalArgument("repo", "path to repository", "repo");
  parser.addOption({{"f", "filter"}, "Filter terms by prefix.", "prefix"});
  parser.addOption({{"l", "limit"}, "Limit child terms.", "limit", "-1"});
  parser.process(app);

  QStringList args = parser.positionalArguments();
  if (args.isEmpty())
    parser.showHelp(1);

  // Initialize global git state.
  RepoInit init;
  (void)init;

  git::Repository repo = git::Repository::open(args.last());
  if (!repo.isValid())
    parser.showHelp(1);

  Index index(repo);
  IndexModel model(&index, parser.value("limit").toInt());
  model.setFilter(parser.value("filter"));

  QTreeView view;
  view.setHeaderHidden(true);
  view.setModel(&model);
  view.show();

  return app.exec();
}
