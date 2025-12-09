/*
#    FVD++, an advanced coaster design tool for NoLimits
#    Copyright (C) 2012-2015, Stephan "Lenny" Alt <alt.stephan@web.de>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QtDebug>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QSurfaceFormat>
#include "mainwindow.h"
#include "lenassert.h"
#include "core/logging.h"
#include "renderer/qtglcompat.h"

#ifdef Q_OS_MAC
#include "osx/NSApplicationMain.h"
#endif

namespace {

void configureSurfaceFormat()
{
#ifdef Q_OS_MAC
    QtGLFormat fmt = qtDefaultSurfaceFormat();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QSurfaceFormat::setDefaultFormat(fmt);
#else
    QGLFormat::setDefaultFormat(fmt);
#endif
#endif
}

QString requestedProjectFile(const QCommandLineParser &parser)
{
    if (parser.isSet(QStringLiteral("project"))) {
        return parser.value(QStringLiteral("project"));
    }

    const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        return positional.first();
    }

    return {};
}

void configureLoggingFromParser(const QCommandLineParser &parser)
{
    if (parser.isSet(QStringLiteral("log-rules"))) {
        qputenv("FVD_LOG_RULES", parser.value(QStringLiteral("log-rules")).toUtf8());
        return;
    }

    if (parser.isSet(QStringLiteral("log-level"))) {
        const QString rules = Logging::rulesForLevel(parser.value(QStringLiteral("log-level")));
        if (!rules.isEmpty()) {
            qputenv("FVD_LOG_RULES", rules.toUtf8());
        }
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    configureSurfaceFormat();

    QCoreApplication::setApplicationName(QStringLiteral("FVD++"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("FVD++, an advanced coaster design tool for NoLimits"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption projectOption({QStringLiteral("p"), QStringLiteral("project")},
                                     QStringLiteral("Project file to open"),
                                     QStringLiteral("file"));
    parser.addOption(projectOption);

    QCommandLineOption logRulesOption(QStringLiteral("log-rules"),
                                      QStringLiteral("Qt logging filter rules (overrides defaults)"),
                                      QStringLiteral("rules"));
    parser.addOption(logRulesOption);

    QCommandLineOption logLevelOption(QStringLiteral("log-level"),
                                      QStringLiteral("Set log verbosity: debug, info, warning, critical, off"),
                                      QStringLiteral("level"));
    parser.addOption(logLevelOption);

    parser.addPositionalArgument(QStringLiteral("project"),
                                 QStringLiteral("Project file to load"),
                                 QStringLiteral("[project]"));

    parser.process(application);

    configureLoggingFromParser(parser);

    Logging::initialize();

    MainWindow w;
    qCInfo(Logging::logApp) << "Main window created";
    w.show();
    const QString projectFile = requestedProjectFile(parser);
    if (projectFile.endsWith(QStringLiteral(".fvd"))) {
        qCInfo(Logging::logApp, "starting FVD++ with project %s", qPrintable(projectFile));
        w.loadProject(projectFile);
    }


#ifdef Q_OS_MAC
    return OwnNSApplicationMain(argc, (const char **)argv);
#else
    return application.exec();
#endif
}
