//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Submodule.h"
#include "Config.h"
#include "Id.h"
#include "Repository.h"

namespace git {

const QString kUrl = "https://github.com/Murmele/Gittyup";

Submodule::Submodule() {}

Submodule::Submodule(git_submodule *submodule)
    : d(submodule, git_submodule_free) {}

Submodule::operator git_submodule *() const { return d.data(); }

bool Submodule::isInitialized() const {
  Repository repo(git_submodule_owner(d.data()));
  QString key = QString("submodule.%1.url").arg(name());
  return !repo.gitConfig().value<QString>(key).isEmpty();
}

void Submodule::initialize() const { git_submodule_init(d.data(), false); }

void Submodule::deinitialize() const {
  // Remove git config entry.
  Repository repo(git_submodule_owner(d.data()));
  Config config = repo.gitConfig();
  QString regex = QString("submodule\\.%1\\..*").arg(name());
  Config::Iterator it = config.glob(regex);
  while (Config::Entry entry = it.next())
    config.remove(entry.name());

  // Remove submodule workdir.
  QDir dir = repo.workdir();
  if (dir.cd(path()) && dir.removeRecursively())
    dir.mkpath(".");
}

QString Submodule::name() const { return git_submodule_name(d.data()); }

QString Submodule::path() const { return git_submodule_path(d.data()); }

QString Submodule::url() const { return git_submodule_url(d.data()); }

void Submodule::setUrl(const QString &url) {
  if (url == this->url())
    return;

  QByteArray buffer = url.toUtf8();
  const char *data = !buffer.isEmpty() ? buffer.constData() : nullptr;
  git_repository *repo = git_submodule_owner(d.data());
  git_submodule_set_url(repo, git_submodule_name(d.data()), data);
}

QString Submodule::branch() const { return git_submodule_branch(d.data()); }

void Submodule::setBranch(const QString &branch) {
  if (branch == this->branch())
    return;

  QByteArray buffer = branch.toUtf8();
  const char *data = !buffer.isEmpty() ? buffer.constData() : nullptr;
  git_repository *repo = git_submodule_owner(d.data());
  git_submodule_set_branch(repo, git_submodule_name(d.data()), data);
}

Id Submodule::headId() const { return git_submodule_head_id(d.data()); }

Id Submodule::indexId() const { return git_submodule_index_id(d.data()); }

Id Submodule::workdirId() const { return git_submodule_wd_id(d.data()); }

Result Submodule::update(Remote::Callbacks *callbacks, bool init, bool checkout_force)
{
  git_submodule_update_options opts = GIT_SUBMODULE_UPDATE_OPTIONS_INIT;
  opts.fetch_opts.callbacks.connected = &Remote::Callbacks::connected;
  opts.fetch_opts.callbacks.about_to_disconnect = &Remote::Callbacks::about_to_disconnect;
  opts.fetch_opts.callbacks.sideband_progress = &Remote::Callbacks::sideband;
  opts.fetch_opts.callbacks.credentials = &Remote::Callbacks::credentials;
  opts.fetch_opts.callbacks.certificate_check = &Remote::Callbacks::certificate;
  opts.fetch_opts.callbacks.transfer_progress = &Remote::Callbacks::transfer;
  opts.fetch_opts.callbacks.update_tips = &Remote::Callbacks::update;
  opts.fetch_opts.callbacks.remote_ready = &Remote::Callbacks::remoteReady;
  opts.fetch_opts.callbacks.payload = callbacks;

  if (checkout_force)
    opts.checkout_opts.checkout_strategy |= GIT_CHECKOUT_FORCE;
  // Use a fake URL. Submodule update doesn't have a way to
  // query a different proxy for each submodule remote.
  QByteArray proxy = Remote::proxyUrl(kUrl, opts.fetch_opts.proxy_opts.type);
  opts.fetch_opts.proxy_opts.url = proxy;

  return git_submodule_update(d.data(), init, &opts);
}

Repository Submodule::open() const {
  git_repository *repo = nullptr;
  git_submodule_open(&repo, d.data());
  return Repository(repo);
}

} // namespace git
