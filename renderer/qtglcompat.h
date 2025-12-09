#pragma once

#include <QImage>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QSurfaceFormat>

using QtGLWidget = QOpenGLWidget;
using QtGLBuffer = QOpenGLBuffer;
using QtGLShaderProgram = QOpenGLShaderProgram;
using QtGLFormat = QSurfaceFormat;

inline QImage qtConvertToGLFormat(const QImage &image)
{
    return QOpenGLTexture::convertToGLFormat(image);
}

inline QtGLFormat qtDefaultSurfaceFormat()
{
    QtGLFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 2);
    format.setSamples(4);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    return format;
}
#else
#include <QGLBuffer>
#include <QGLContext>
#include <QGLShaderProgram>
#include <QGLWidget>

using QtGLWidget = QGLWidget;
using QtGLBuffer = QGLBuffer;
using QtGLShaderProgram = QGLShaderProgram;
using QtGLFormat = QGLFormat;

inline QImage qtConvertToGLFormat(const QImage &image)
{
    return QGLWidget::convertToGLFormat(image);
}

inline QtGLFormat qtDefaultSurfaceFormat()
{
    QtGLFormat format = QGLFormat::defaultFormat();
    format.setProfile(QGLFormat::CoreProfile);
    format.setVersion(3, 2);
    format.setSampleBuffers(true);
    format.setSamples(4);
    return format;
}
#endif
