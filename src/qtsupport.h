/// Backwards compatibility for older versions of QT
/// Several symbols have been deprecated in QT5.14 and moved to new
/// namespaces. This file provides support for older versions of QT.

#include <QString>
#include <QTextStream>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt
{
    static auto endl = ::endl;
    static auto KeepEmptyParts = QString::KeepEmptyParts;
    static auto SkipEmptyParts = QString::SkipEmptyParts;
}

// QButtonGroup::buttonClicked is deprecated in favor of QButtonGroup::idClicked
#define idClicked buttonClicked
#endif