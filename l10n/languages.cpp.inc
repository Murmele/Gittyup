#include "languages.h"

#include <QList>
#include <QString>

namespace Languages {
  const QString system = QStringLiteral("@SYSTEM_LANG_KEY@");
  const QMap<const char*, const char*> languages = { @SUPPORTED_LANGUAGES@ };
}
