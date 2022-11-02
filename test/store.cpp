#include "Test.h"

#include "cred/Store.h"

class TestStore : public QObject {
  Q_OBJECT

private:
  QTemporaryDir mTempDir;

  QString getTestFilePath();

private slots:
  void initTestCase();
  void readUserPassTestCase();
  void wrongProtocolTestCase();
  void wrongUrlTestCase();
  void wrongUsernameTestCase();
  void saveUserPassTestCase();
  void wrongFilePathTestCase();
  void cleanupTestCase();
};

QString TestStore::getTestFilePath() {
  return mTempDir.path() + "/fakeCred.txt";
}

void TestStore::initTestCase() {
  QFile file(getTestFilePath());
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream fout(&file);
    fout << "http://joneDoe:secretPassword@192.168.1.1\n";
    fout << "https://janeDoe:securePassword@192.168.2.2:3000\n";
    file.close();
  }
}

void TestStore::readUserPassTestCase() {
  Store store(getTestFilePath());

  auto url = "https://192.168.2.2:3000/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(result);
  QCOMPARE(username, "janeDoe");
  QCOMPARE(password, "securePassword");
}

void TestStore::wrongProtocolTestCase() {
  Store store(getTestFilePath());

  auto url = "https://192.168.1.1/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::wrongUrlTestCase() {
  Store store(getTestFilePath());

  auto url = "http://192.168.1.2/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::wrongUsernameTestCase() {
  Store store(getTestFilePath());

  auto url = "http://192.168.1.1/h-4nd-h/fake.git";
  QString username = "janeDoe";
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::saveUserPassTestCase() {
  Store store(getTestFilePath());

  auto url = "http://192.168.1.2/h-4nd-h/fake.git";
  QString username = "NewUser";
  QString password = "NewPassword";
  auto result = store.store(url, username, password);

  QVERIFY(result);

  QString readBackUsername;
  QString readBackpassword;
  auto result2 = store.get(url, readBackUsername, readBackpassword);

  QVERIFY(result2);
  QCOMPARE(readBackUsername, "NewUser");
  QCOMPARE(readBackpassword, "NewPassword");
}

void TestStore::wrongFilePathTestCase() {
  Store store("veryWrongPath");

  auto url = "http://192.168.1.2/h-4nd-h/fake.git";
  QString username = "";
  QString password = "";
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::cleanupTestCase() {
  QFile file(getTestFilePath());
  file.remove();
}

TEST_MAIN(TestStore)
#include "store.moc"
