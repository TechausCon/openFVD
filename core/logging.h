#ifndef FVD_LOGGING_H
#define FVD_LOGGING_H

#include <QLoggingCategory>

namespace Logging {

Q_DECLARE_LOGGING_CATEGORY(logApp)
Q_DECLARE_LOGGING_CATEGORY(logCore)
Q_DECLARE_LOGGING_CATEGORY(logRenderer)
Q_DECLARE_LOGGING_CATEGORY(logUi)

QString rulesForLevel(const QString &level);
void initialize();

} // namespace Logging

#endif // FVD_LOGGING_H
