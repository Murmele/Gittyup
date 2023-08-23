#include "Test.h"

#include "conf/Setting.h"

using namespace QTest;

class TestSetting : public QObject {
  Q_OBJECT

private slots:
  void defines_a_non_empty_settings_key_for_each_id();
  void defines_each_settings_key_only_once();

private:
  template <typename TId> QList<TId> ids() const {
    QList<TId> ids;
    QMetaEnum metaEnum = QMetaEnum::fromType<TId>();
    for (int i = 0; i < metaEnum.keyCount(); i++) {
      ids << static_cast<TId>(metaEnum.value(i));
    }
    return ids;
  }

  template <class T, typename TId> QStringList settingsKeys() {
    QStringList settingsKeys;
    foreach (const TId id, ids<TId>()) { settingsKeys.append(T::key(id)); }
    return settingsKeys;
  }

  template <class T, typename TId> void verifyNonEmptySettingsKeyForEachId() {
    QMetaEnum metaEnum = QMetaEnum::fromType<TId>();

    foreach (const TId id, ids<TId>()) {
      const QString settingsKey = T::key(id);

      QVERIFY2(!settingsKey.isEmpty(),
               qPrintable(QString("no settings key defined for %1::%2::%3")
                              .arg(metaEnum.scope(), metaEnum.name(),
                                   metaEnum.valueToKey(static_cast<int>(id)))));
    }
  }
};

void TestSetting::defines_a_non_empty_settings_key_for_each_id() {
  verifyNonEmptySettingsKeyForEachId<Setting, Setting::Id>();
  verifyNonEmptySettingsKeyForEachId<Prompt, Prompt::Kind>();
}

void TestSetting::defines_each_settings_key_only_once() {
  QStringList allSettingsKeys;
  allSettingsKeys.append(settingsKeys<Setting, Setting::Id>());
  allSettingsKeys.append(settingsKeys<Prompt, Prompt::Kind>());

  QStringList uniqueSettingsKeys;
  foreach (const QString &settingsKey, allSettingsKeys) {
    QVERIFY2(!uniqueSettingsKeys.contains(settingsKey),
             qPrintable(
                 QString("the settings key '%1' is used for multiple settings")
                     .arg(settingsKey)));
    uniqueSettingsKeys.append(settingsKey);
  }
}

TEST_MAIN(TestSetting)

#include "Setting.moc"
