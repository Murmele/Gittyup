//
//          Copyright (c) 2022, Gittyup Team
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#include "Test.h"
#include "git/Remote.h"
#include "git2/buffer.h"
#include "qtestcase.h"

class Callbacks : public git::Remote::Callbacks {
private:
  QString config;

public:
  Callbacks(const QString &url, const QString &config)
      : git::Remote::Callbacks(url), config(config) {}

  virtual QString configFilePath() const override {
    return QDir{SSH_CONFIG}.filePath(config);
  }
};

class TestSshConfig : public QObject {
  Q_OBJECT

private slots:
  void resolveSsh();
  void resolveSshUrl();
  void resolveGitSshUrl();
  void resolveGitUrl();
  void resolveHttpUrl();
  void resolvePath();

private:
  static QString transformUrl(const QString &url, const QString config) {
    auto callbacks = Callbacks(url, config);
    git_buf buf;
    buf.asize = 0;
    buf.size = 0;
    buf.ptr = nullptr;

    Callbacks::url(&buf, url.toUtf8().data(), 0, &callbacks);

    QString res = QString::fromUtf8(buf.ptr, (int)buf.size);
    git_buf_dispose(&buf);
    return res;
  }
};

void TestSshConfig::resolveSsh() {
  QCOMPARE(transformUrl("testhost:repo", "ssh_config"),
           QString("ssh://anotheruser@resolvedhost:12345/repo"));
  QCOMPARE(transformUrl("specified@testhost:/repo", "ssh_config"),
           "ssh://specified@resolvedhost:12345/repo");
  QCOMPARE(transformUrl("diffport:", "ssh_config"),
           "ssh://defaultuser@diffport:4321/");
  QCOMPARE(transformUrl("diffuser:", "ssh_config"),
           "ssh://seconduser@diffuser:2222/");
  QCOMPARE(transformUrl("diffhost:", "ssh_config"),
           "ssh://defaultuser@secondhost:2222/");
  QCOMPARE(transformUrl("unconfigured:", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/");
  QCOMPARE(transformUrl("unconfigured:/", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/");
  QCOMPARE(transformUrl("unconfigured:/repo", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/repo");
  QCOMPARE(transformUrl("unconfigured:/repo/", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/repo/");
  QCOMPARE(transformUrl("unconfigured:repo/", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/repo/");

  QCOMPARE(transformUrl("testhost:repo", "ssh_config2"),
           QString("ssh://anotheruser@resolvedhost:12345/repo"));
  QCOMPARE(transformUrl("testhost:/repo", "ssh_config2"),
           "ssh://anotheruser@resolvedhost:12345/repo");
  QCOMPARE(transformUrl("specified@testhost:/repo", "ssh_config2"),
           "ssh://specified@resolvedhost:12345/repo");
  QCOMPARE(transformUrl("diffport:", "ssh_config2"), "ssh://diffport:4321/");
  QCOMPARE(transformUrl("diffuser:", "ssh_config2"),
           "ssh://seconduser@diffuser/");
  QCOMPARE(transformUrl("diffhost:", "ssh_config2"), "ssh://secondhost/");
  QCOMPARE(transformUrl("unconfigured:", "ssh_config2"), "ssh://unconfigured/");
  QCOMPARE(transformUrl("unconfigured:/", "ssh_config2"),
           "ssh://unconfigured/");
  QCOMPARE(transformUrl("unconfigured:/repo", "ssh_config2"),
           "ssh://unconfigured/repo");
  QCOMPARE(transformUrl("unconfigured:/repo/", "ssh_config2"),
           "ssh://unconfigured/repo/");
  QCOMPARE(transformUrl("unconfigured:repo/", "ssh_config2"),
           "ssh://unconfigured/repo/");
}

void TestSshConfig::resolveSshUrl() {
  QCOMPARE(transformUrl("ssh://testhost/repo", "ssh_config"),
           QString("ssh://anotheruser@resolvedhost:12345/repo"));
  QCOMPARE(transformUrl("ssh://specified@testhost/repo", "ssh_config"),
           "ssh://specified@resolvedhost:12345/repo");
  QCOMPARE(transformUrl("ssh://diffport", "ssh_config"),
           "ssh://defaultuser@diffport:4321");
  QCOMPARE(transformUrl("ssh://diffuser", "ssh_config"),
           "ssh://seconduser@diffuser:2222");
  QCOMPARE(transformUrl("ssh://diffhost", "ssh_config"),
           "ssh://defaultuser@secondhost:2222");
  QCOMPARE(transformUrl("ssh://unconfigured", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222");
  QCOMPARE(transformUrl("ssh://unconfigured/", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/");
  QCOMPARE(transformUrl("ssh://unconfigured/repo", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/repo");
  QCOMPARE(transformUrl("ssh://unconfigured/repo/", "ssh_config"),
           "ssh://defaultuser@unconfigured:2222/repo/");

  QCOMPARE(transformUrl("ssh://testhost/repo", "ssh_config2"),
           QString("ssh://anotheruser@resolvedhost:12345/repo"));
  QCOMPARE(transformUrl("ssh://specified@testhost/repo", "ssh_config2"),
           "ssh://specified@resolvedhost:12345/repo");
  QCOMPARE(transformUrl("ssh://diffport", "ssh_config2"),
           "ssh://diffport:4321");
  QCOMPARE(transformUrl("ssh://diffuser", "ssh_config2"),
           "ssh://seconduser@diffuser");
  QCOMPARE(transformUrl("ssh://diffhost", "ssh_config2"), "ssh://secondhost");
  QCOMPARE(transformUrl("ssh://unconfigured", "ssh_config2"),
           "ssh://unconfigured");
  QCOMPARE(transformUrl("ssh://unconfigured/", "ssh_config2"),
           "ssh://unconfigured/");
  QCOMPARE(transformUrl("ssh://unconfigured/repo", "ssh_config2"),
           "ssh://unconfigured/repo");
  QCOMPARE(transformUrl("ssh://unconfigured/repo/", "ssh_config2"),
           "ssh://unconfigured/repo/");
}

void TestSshConfig::resolveGitSshUrl() {
  QCOMPARE(transformUrl("git+ssh://testhost/repo", "ssh_config"),
           QString("git+ssh://testhost/repo"));
  QCOMPARE(transformUrl("git+ssh://unconfigured/repo", "ssh_config"),
           QString("git+ssh://unconfigured/repo"));

  QCOMPARE(transformUrl("git+ssh://testhost/repo", "ssh_config2"),
           QString("git+ssh://testhost/repo"));
  QCOMPARE(transformUrl("git+ssh://unconfigured/repo", "ssh_config2"),
           QString("git+ssh://unconfigured/repo"));
}

void TestSshConfig::resolveGitUrl() {
  QCOMPARE(transformUrl("git://testhost/repo", "ssh_config"),
           QString("git://testhost/repo"));
  QCOMPARE(transformUrl("git://unconfigured/repo", "ssh_config"),
           QString("git://unconfigured/repo"));

  QCOMPARE(transformUrl("git://testhost/repo", "ssh_config2"),
           QString("git://testhost/repo"));
  QCOMPARE(transformUrl("git://unconfigured/repo", "ssh_config2"),
           QString("git://unconfigured/repo"));
}

void TestSshConfig::resolveHttpUrl() {
  QCOMPARE(transformUrl("http://testhost/repo", "ssh_config"),
           QString("http://testhost/repo"));
  QCOMPARE(transformUrl("http://unconfigured/repo", "ssh_config"),
           QString("http://unconfigured/repo"));

  QCOMPARE(transformUrl("http://testhost/repo", "ssh_config2"),
           QString("http://testhost/repo"));
  QCOMPARE(transformUrl("http://unconfigured/repo", "ssh_config2"),
           QString("http://unconfigured/repo"));
}

void TestSshConfig::resolvePath() {
  QCOMPARE(transformUrl("/some/path", "ssh_config"),
           QString("file:///some/path"));
  QCOMPARE(transformUrl("some/path", "ssh_config"), QString("file:some/path"));

#ifdef Q_OS_WIN
  QCOMPARE(transformUrl("A:/some/path", "ssh_config"),
           QString("file:///A:/some/path"));
  QCOMPARE(transformUrl("A:\\some\\path", "ssh_config"),
           QString("file:///A:/some/path"));
  QCOMPARE(transformUrl("some\\path", "ssh_config"), QString("file:some/path"));
#else
  QCOMPARE(transformUrl("A:/some/path", "ssh_config"),
           QString("file:///A:/some/path"));
  QCOMPARE(transformUrl("A:\\some\\path", "ssh_config"),
           QString("file:///A:%5Csome%5Cpath"));
  QCOMPARE(transformUrl("some\\path", "ssh_config"),
           QString("file:some%5Cpath"));
#endif
}

TEST_MAIN(TestSshConfig)

#include "SshConfig.moc"
