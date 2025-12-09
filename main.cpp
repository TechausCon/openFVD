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
#include <QLoggingCategory>
#include "mainwindow.h"
#include "lenassert.h"
#include "core/logging.h"

QApplication* application;

#ifdef Q_OS_MAC
#include "osx/NSApplicationMain.h"
#endif

int main(int argc, char *argv[])
{
    application = new QApplication(argc, argv);

    Logging::initialize();

#ifdef Q_OS_MAC
    QGLFormat fmt;
    fmt.setProfile(QGLFormat::CoreProfile);
    fmt.setVersion(3,2);
    fmt.setSampleBuffers(true);
    fmt.setSamples(4);
    QGLFormat::setDefaultFormat(fmt);
#endif

    MainWindow w;
    qCInfo(Logging::logApp) << "Main window created";
    w.show();
    if(argc == 2) {
        QString fileName(argv[1]);
        if(fileName.endsWith(".fvd")) {
            qCInfo(Logging::logApp, "starting FVD++ with project %s", argv[1]);
            w.loadProject(argv[1]);
        }
    }


#ifdef Q_OS_MAC
    return OwnNSApplicationMain(argc, (const char **)argv);
#else
    return application->exec();
#endif
}
