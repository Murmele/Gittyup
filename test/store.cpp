#include "Test.h"

#include "cred/Store.h"

#define PATH "./test/fakeCred.txt"

class TestStore : public QObject {
  Q_OBJECT

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

void TestStore::initTestCase() {
  QFile file(PATH);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream fout(&file);
    fout << "http://joneDoe:secretPassword@192.168.1.1\n";
    fout << "https://janeDoe:securePassword@192.168.2.2:3000\n";
    file.close();
  }
}

void TestStore::readUserPassTestCase() {
  Store store(PATH);

  auto url = "https://192.168.2.2:3000/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(result);
  QCOMPARE(username, "janeDoe");
  QCOMPARE(password, "securePassword");
}

void TestStore::wrongProtocolTestCase() {
  Store store(PATH);

  auto url = "https://192.168.1.1/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::wrongUrlTestCase() {
  Store store(PATH);

  auto url = "http://192.168.1.2/h-4nd-h/fake.git";
  QString username;
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::wrongUsernameTestCase() {
  Store store(PATH);

  auto url = "http://192.168.1.1/h-4nd-h/fake.git";
  QString username = "janeDoe";
  QString password;
  auto result = store.get(url, username, password);

  QVERIFY(!result);
}

void TestStore::saveUserPassTestCase() {
  Store store(PATH);

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
  QFile file(PATH);
  file.remove();
}

TEST_MAIN(TestStore)
#include "store.moc"
