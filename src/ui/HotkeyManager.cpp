//
//          Copyright (c) 2021, Gittyup
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas (https://github.com/exactly-one-kas)
//

#include "HotkeyManager.h"

struct HotkeyManagerHandle {
  const char *defaultKeys;
  QKeySequence::StandardKey standardKey;
  const char *configPath;
  const char *label;
  int index;
  HotkeyManagerHandle *next;
};

static HotkeyManagerHandle *hotkeyRegistry;

#ifndef QT_NO_DEBUG
static bool consumed;
#endif

HotkeyHandle::HotkeyHandle(QMetaObject::Connection &&changeSignalConnection)
    : changeSignalConnection(std::move(changeSignalConnection)) {}

HotkeyHandle::~HotkeyHandle() { disconnect(changeSignalConnection); }

HotkeyHandle *
Hotkey::use(std::function<void(const QKeySequence &)> changeHandler,
            HotkeyManager *manager) const {
  if (!manager)
    manager = HotkeyManager::instance();

  QMetaObject::Connection connection = QObject::connect(
      manager, &HotkeyManager::changed,
      [this, changeHandler, manager](const HotkeyManagerHandle *handle) {
        if (handle == mHandle)
          changeHandler(manager->keys(mHandle));
      });

  changeHandler(manager->keys(mHandle));

  return new HotkeyHandle(std::move(connection));
}

HotkeyHandle *Hotkey::use(QAction *action, HotkeyManager *manager) const {
  HotkeyHandle *res =
      use([action](const QKeySequence &keys) { action->setShortcut(keys); },
          manager);

  res->setParent(action);

  return res;
}

HotkeyHandle *Hotkey::use(QShortcut *shortcut, HotkeyManager *manager) const {
  HotkeyHandle *res =
      use([shortcut](const QKeySequence &keys) { shortcut->setKey(keys); },
          manager);

  res->setParent(shortcut);

  return res;
}

QString Hotkey::label() const { return mHandle->label; }

QKeySequence Hotkey::currentKeys(const HotkeyManager *manager) const {
  if (!manager)
    manager = HotkeyManager::instance();

  return manager->keys(mHandle);
}

QString Hotkey::defaultKeys() const {
  return QObject::tr(mHandle->defaultKeys);
}

void Hotkey::setKeys(const QKeySequence &keys, HotkeyManager *manager) {
  if (!manager)
    manager = HotkeyManager::instance();

  manager->setKeys(mHandle, keys);
}

Hotkey::Hotkey(const HotkeyManagerHandle *handle) : mHandle(handle) {}

HotkeyManager *HotkeyManager::instance() {
  static HotkeyManager *instance = nullptr;
  if (!instance)
    instance = new HotkeyManager();

  return instance;
}

Hotkey HotkeyManager::registerHotkey(const char *defaultKeys,
                                     QKeySequence::StandardKey standardKey,
                                     const char *configPath,
                                     const char *label) {
#ifndef QT_NO_DEBUG
  Q_ASSERT_X(!consumed, "HotkeyManager::registerHotkey()",
             "Registering a hotkey after creating a hotkey manager");
#endif

  HotkeyManagerHandle *info = new HotkeyManagerHandle();
  info->defaultKeys = defaultKeys;
  info->standardKey = standardKey;
  info->configPath = configPath;
  info->label = label;
  info->next = hotkeyRegistry;

  if (hotkeyRegistry)
    info->index = hotkeyRegistry->index + 1;
  else
    info->index = 0;

  hotkeyRegistry = info;

  return Hotkey(info);
}

Hotkey HotkeyManager::registerHotkey(const char *defaultKeys,
                                     const char *configPath,
                                     const char *label) {
  if (!defaultKeys)
    defaultKeys = "";

  return registerHotkey(defaultKeys, QKeySequence::StandardKey::UnknownKey,
                        configPath, label);
}

Hotkey HotkeyManager::registerHotkey(QKeySequence::StandardKey standardKey,
                                     const char *configPath,
                                     const char *label) {
  return registerHotkey(nullptr, standardKey, configPath, label);
}

static QKeySequence getKeys(const QString &setting,
                            HotkeyManagerHandle *handle) {
  if (!setting.isEmpty())
    return QKeySequence(setting, QKeySequence::SequenceFormat::PortableText);
  else if (!handle->defaultKeys)
    return QKeySequence(handle->standardKey);
  else
    return QKeySequence(handle->defaultKeys);
}

HotkeyManager::HotkeyManager(Settings *settings)
    : mSettings(settings), mHandles(hotkeyRegistry->index + 1),
      mKeys(hotkeyRegistry->index + 1) {
#ifndef QT_NO_DEBUG
  consumed = true;
#endif

  if (!mSettings)
    mSettings = Settings::instance();

  for (HotkeyManagerHandle *i = hotkeyRegistry; i; i = i->next) {
    mHandles[i->index] = i;
    mKeys[i->index] = getKeys(mSettings->hotkey(i->configPath), i);
  }
}

QVector<Hotkey> HotkeyManager::knownHotkeys() const {
  QVector<Hotkey> res(mHandles.size());

  for (HotkeyManagerHandle *i : mHandles)
    res[i->index] = Hotkey(i);

  return res;
}

QKeySequence HotkeyManager::keys(const HotkeyManagerHandle *handle) const {
  return mKeys[handle->index];
}

void HotkeyManager::setKeys(const HotkeyManagerHandle *handle,
                            const QKeySequence &keys) {
  mKeys[handle->index] = keys;
  mSettings->setHotkey(handle->configPath, keys.toString());

  emit changed(handle);
}
