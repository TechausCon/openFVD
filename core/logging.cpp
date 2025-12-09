#include "logging.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>
#include <QThread>

namespace Logging {

Q_LOGGING_CATEGORY(logApp, "fvd.app")
Q_LOGGING_CATEGORY(logCore, "fvd.core")
Q_LOGGING_CATEGORY(logRenderer, "fvd.renderer")
Q_LOGGING_CATEGORY(logUi, "fvd.ui")

namespace {

class FileLogger
{
public:
    FileLogger();
    ~FileLogger();

    void installHandler();
    QString logFilePath() const { return m_logPath; }

    static FileLogger &instance();
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    void writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    QString levelName(QtMsgType type) const;
    QString threadIdString() const;
    void rotateIfNeeded();
    void reopen();
    void writeHeader();

    QString m_logPath;
    QFile m_logFile;
    QTextStream m_logStream;
    QtMessageHandler m_previousHandler;
    mutable QMutex m_mutex;
    const qint64 m_maxSizeBytes;
};

FileLogger::FileLogger()
    : m_logPath(QStringLiteral("fvd.log")),
      m_previousHandler(nullptr),
      m_maxSizeBytes(1024 * 512) // 512 KiB
{
}

FileLogger::~FileLogger()
{
    QMutexLocker locker(&m_mutex);
    if (m_previousHandler) {
        qInstallMessageHandler(m_previousHandler);
    }
    m_logStream.flush();
    m_logFile.close();
}

void FileLogger::installHandler()
{
    QMutexLocker locker(&m_mutex);
    if (m_previousHandler) {
        return;
    }
    rotateIfNeeded();
    reopen();
    m_previousHandler = qInstallMessageHandler(&FileLogger::messageHandler);
}

FileLogger &FileLogger::instance()
{
    static FileLogger logger;
    return logger;
}

void FileLogger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    FileLogger &logger = FileLogger::instance();
    if (!logger.m_previousHandler) {
        logger.installHandler();
    }

    logger.writeMessage(type, context, msg);
}

QString FileLogger::levelName(QtMsgType type) const
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("CRIT");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("LOG");
}

QString FileLogger::threadIdString() const
{
    quintptr id = reinterpret_cast<quintptr>(QThread::currentThreadId());
    return QString::number(id, 16);
}

void FileLogger::rotateIfNeeded()
{
    QFileInfo info(m_logPath);
    if (info.exists() && info.size() > m_maxSizeBytes) {
        const QString rotatedPath = m_logPath + QStringLiteral(".1");
        QFile::remove(rotatedPath);
        QFile::rename(m_logPath, rotatedPath);
    }
}

void FileLogger::reopen()
{
    m_logFile.close();

    if (!QFileInfo::exists(m_logPath)) {
        writeHeader();
    }

    m_logFile.setFileName(m_logPath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_logStream.setDevice(&m_logFile);
    m_logStream.setCodec("UTF-8");
}

void FileLogger::writeHeader()
{
    QSaveFile saver(m_logPath);
    if (!saver.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream header(&saver);
    header.setCodec("UTF-8");
    header << "FVD++ Logfile\n";
    header << "Started at " << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "\n";
    header.flush();
    saver.commit();
}

void FileLogger::writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen() && m_logFile.size() > m_maxSizeBytes) {
        rotateIfNeeded();
        reopen();
    }

    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const qint64 pid = QCoreApplication::applicationPid();
    const QString threadId = threadIdString();
    const QString category = context.category ? QString::fromUtf8(context.category) : QStringLiteral("default");

    const QString line = QStringLiteral("%1 [pid:%2 tid:%3] [%4] (%5) %6")
            .arg(timestamp)
            .arg(pid)
            .arg(threadId)
            .arg(levelName(type))
            .arg(category)
            .arg(msg);

    m_logStream << line << '\n';
    m_logStream.flush();

    QTextStream console(stderr);
    console << line << '\n';
    console.flush();

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

void initialize()
{
    FileLogger &logger = FileLogger::instance();
    logger.installHandler();

    const QString defaultRules = QStringLiteral(
        "fvd.app.debug=true\n"
        "fvd.core.debug=true\n"
        "fvd.renderer.debug=true\n"
        "fvd.ui.debug=true\n");

    const QString rules = qEnvironmentVariableIsSet("FVD_LOG_RULES")
            ? QString::fromUtf8(qgetenv("FVD_LOG_RULES"))
            : defaultRules;

    QLoggingCategory::setFilterRules(rules);

    qCInfo(logApp) << "Logging initialized at" << logger.logFilePath();
}

} // namespace Logging

