#include "logging.h"

#include <algorithm>
#include <functional>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QStringList>
#include <QTextStream>
#include <QThread>

namespace Logging {

Q_LOGGING_CATEGORY(logApp, "fvd.app")
Q_LOGGING_CATEGORY(logCore, "fvd.core")
Q_LOGGING_CATEGORY(logRenderer, "fvd.renderer")
Q_LOGGING_CATEGORY(logUi, "fvd.ui")

namespace {

QString normalizeRulesForLevel(const QString &level)
{
    if (level.isEmpty()) {
        return {};
    }

    const QString normalized = level.toLower();
    if (normalized != QLatin1String("debug") && normalized != QLatin1String("info") &&
        normalized != QLatin1String("warning") && normalized != QLatin1String("critical") &&
        normalized != QLatin1String("off")) {
        return {};
    }

    const bool debugEnabled = normalized == QLatin1String("debug");
    const bool infoEnabled = debugEnabled || normalized == QLatin1String("info");
    const bool warningEnabled = infoEnabled || normalized == QLatin1String("warning");
    const bool criticalEnabled = warningEnabled || normalized == QLatin1String("critical");

    const QStringList categories = {
        QStringLiteral("fvd.app"),
        QStringLiteral("fvd.core"),
        QStringLiteral("fvd.renderer"),
        QStringLiteral("fvd.ui")
    };

    QStringList rules;
    for (const QString &category : categories) {
        rules << QStringLiteral("%1.debug=%2").arg(category, debugEnabled ? QStringLiteral("true") : QStringLiteral("false"));
        rules << QStringLiteral("%1.info=%2").arg(category, infoEnabled ? QStringLiteral("true") : QStringLiteral("false"));
        rules << QStringLiteral("%1.warning=%2").arg(category, warningEnabled ? QStringLiteral("true") : QStringLiteral("false"));
        rules << QStringLiteral("%1.critical=%2").arg(category, criticalEnabled ? QStringLiteral("true") : QStringLiteral("false"));
    }

    return rules.join(QLatin1Char('\n'));
}

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
    void pruneHistory();
    void flushIfNeeded(QtMsgType type);

    QString m_logPath;
    QFile m_logFile;
    QTextStream m_logStream;
    QtMessageHandler m_previousHandler;
    mutable QMutex m_mutex;
    const qint64 m_maxSizeBytes;
    const qint64 m_maxTotalBytes;
    const qint64 m_flushIntervalMs;
    QElapsedTimer m_flushTimer;
};

FileLogger::FileLogger()
    : m_logPath(QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                    .filePath(QStringLiteral("fvd.log"))),
      m_previousHandler(nullptr),
      m_maxSizeBytes(1024 * 512), // 512 KiB
      m_maxTotalBytes(1024 * 1024 * 2),
      m_flushIntervalMs(2000)
{
    QDir dir(QFileInfo(m_logPath).dir());
    dir.mkpath(QStringLiteral("."));
    m_flushTimer.start();
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
    if (!m_logFile.isOpen() || m_logFile.size() <= m_maxSizeBytes) {
        return;
    }

    m_logStream.flush();
    m_logFile.flush();
    m_logFile.close();

    const QFileInfo baseInfo(m_logPath);
    QDir dir(baseInfo.dir());
    const QString baseName = baseInfo.fileName();
    const int maxFiles = qMax(2, static_cast<int>(m_maxTotalBytes / m_maxSizeBytes));

    const QStringList rotated = dir.entryList(QStringList{baseName + QStringLiteral(".*")}, QDir::Files, QDir::Name);
    QVector<int> indices;
    indices.reserve(rotated.size());
    for (const QString &entry : rotated) {
        const QString suffix = entry.mid(baseName.size() + 1);
        bool ok = false;
        const int value = suffix.toInt(&ok);
        if (ok) {
            indices.append(value);
        }
    }

    std::sort(indices.begin(), indices.end(), std::greater<int>());
    for (int index : indices) {
        const QString currentName = QStringLiteral("%1.%2").arg(baseName).arg(index);
        if (index >= maxFiles - 1) {
            dir.remove(currentName);
            continue;
        }

        const QString nextName = QStringLiteral("%1.%2").arg(baseName).arg(index + 1);
        dir.remove(nextName);
        dir.rename(currentName, nextName);
    }

    dir.rename(baseName, QStringLiteral("%1.1").arg(baseName));

    pruneHistory();

    reopen();
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
    m_flushTimer.restart();
}

void FileLogger::writeHeader()
{
    QDir(QFileInfo(m_logPath).dir()).mkpath(QStringLiteral("."));

    QSaveFile saver(m_logPath);
    if (!saver.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream header(&saver);
    header.setCodec("UTF-8");
    header << "FVD++ Logfile\n";
    const QString appName = QCoreApplication::applicationName();
    const QString appVersion = QCoreApplication::applicationVersion();
    header << "Application: " << appName << ' ' << appVersion << "\n";
    header << "Qt version: " << QString::fromUtf8(qVersion()) << "\n";
    header << "Started at " << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "\n";
    header.flush();
    saver.commit();
}

void FileLogger::pruneHistory()
{
    const QFileInfo baseInfo(m_logPath);
    QDir dir(baseInfo.dir());
    const QString baseName = baseInfo.fileName();
    qint64 totalSize = 0;

    const QFileInfoList files = dir.entryInfoList(QStringList{baseName + QStringLiteral(".*")},
                                                 QDir::Files,
                                                 QDir::Time | QDir::Reversed);

    for (const QFileInfo &info : files) {
        totalSize += info.size();
    }

    for (const QFileInfo &info : files) {
        if (totalSize <= m_maxTotalBytes) {
            break;
        }
        dir.remove(info.fileName());
        totalSize -= info.size();
    }
}

void FileLogger::flushIfNeeded(QtMsgType type)
{
    const bool shouldFlush = type >= QtCriticalMsg || m_flushTimer.elapsed() >= m_flushIntervalMs;
    if (shouldFlush) {
        m_logStream.flush();
        m_flushTimer.restart();
    }
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
    flushIfNeeded(type);

    QTextStream console(stderr);
    console << line << '\n';
    console.flush();

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

QString rulesForLevel(const QString &level)
{
    return normalizeRulesForLevel(level);
}

void initialize()
{
    FileLogger &logger = FileLogger::instance();
    logger.installHandler();

    const QString defaultRules = QStringLiteral(
        "fvd.app.debug=true\n"
        "fvd.core.debug=true\n"
        "fvd.renderer.debug=true\n"
        "fvd.ui.debug=true\n");

    const QString configPath = QDir(QFileInfo(logger.logFilePath()).dir()).filePath(QStringLiteral("logging.ini"));
    QSettings settings(configPath, QSettings::IniFormat);

    QString rules = settings.value(QStringLiteral("logging/rules")).toString();
    if (rules.isEmpty()) {
        const QString level = settings.value(QStringLiteral("logging/level")).toString();
        rules = rulesForLevel(level);
    }
    if (rules.isEmpty()) {
        rules = defaultRules;
    }

    if (qEnvironmentVariableIsSet("FVD_LOG_RULES")) {
        rules = QString::fromUtf8(qgetenv("FVD_LOG_RULES"));
    }

    QLoggingCategory::setFilterRules(rules);

    qCInfo(logApp) << "Logging initialized at" << logger.logFilePath();

    QSurfaceFormat::setDefaultFormat(QSurfaceFormat::defaultFormat());
    QOpenGLContext context;
    context.setFormat(QSurfaceFormat::defaultFormat());
    QString vendor;
    QString renderer;
    QString version;
    if (context.create()) {
        QOffscreenSurface surface;
        surface.setFormat(context.format());
        surface.create();
        if (surface.isValid() && context.makeCurrent(&surface)) {
            QOpenGLFunctions *functions = context.functions();
            if (functions) {
                functions->initializeOpenGLFunctions();
                vendor = reinterpret_cast<const char *>(functions->glGetString(GL_VENDOR));
                renderer = reinterpret_cast<const char *>(functions->glGetString(GL_RENDERER));
                version = reinterpret_cast<const char *>(functions->glGetString(GL_VERSION));
            }
            context.doneCurrent();
        }
    }

    qCInfo(logApp) << "Application version:" << QCoreApplication::applicationVersion();
    qCInfo(logApp) << "Qt version:" << QString::fromUtf8(qVersion());
    if (!vendor.isEmpty() || !renderer.isEmpty()) {
        qCInfo(logApp) << "GPU Vendor:" << vendor << "Renderer:" << renderer << "Version:" << version;
    } else {
        qCWarning(logApp) << "Unable to determine GPU information";
    }
}

} // namespace Logging

