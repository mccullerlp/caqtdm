/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#include "caimage.h"
#include "searchfile.h"
#include "fileFunctions.h"
#include <QPainter>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QResizeEvent>
//#include <QElapsedTimer>

Q_LOGGING_CATEGORY(caImageLog, "caqtdm.widgets.caimage")

caImage::caImage(QWidget* parent) : QWidget(parent)
{
    messagequeue = new messageQueue();
    _container = new QLabel();
    _layout = new QVBoxLayout(this);
    thisAngle = 0;
    thisFrame = prevFrame = 0;
    setVisibility(StaticV);
    timerId = 0;
    thisDelay = 500;
    thisIsSvg = false;
    _svg = Q_NULLPTR;
}

caImage::~caImage() {

   delete _animation;
   delete messagequeue;
}

QString caImage::getMessages()
{
    if(!messagequeue->isEmpty()) {
        return messagequeue->dequeue();
    } else {
      return NULL;
    }
}

bool caImage::anyMessages()
{
    return !messagequeue->isEmpty();
}

void caImage::init(const QString& filename, const bool isProvisional) {
    // this will check for file existence and when an url is defined, download the file from a http server
    fileFunctions filefunction;
    int success = filefunction.checkFileAndDownload(filename);
    if(filefunction.lastInfo().length() > 0) messagequeue->enqueue(filefunction.lastInfo());
    if(!success) {
        if (isProvisional) {
            qCDebug(caImageLog) << "caimage:" << tr("Info: could not find or download provisional file %1, however continue; %2").arg(filename).arg(qasc(filefunction.lastInfo()));
        } else {
            if(filefunction.lastError().length() > 0) messagequeue->enqueue(filefunction.lastError());
            messagequeue->enqueue(tr("Info: could not find or download file %1, however continue").arg(filename));
            qCWarning(caImageLog) << "caimage:" << tr("Info: could not find or download file %1, however continue; %2").arg(filename).arg(qasc(filefunction.lastInfo()));
        }
    }

    searchFile *s = new searchFile(filename);
    QString fileNameFound = s->findFile();
    if(fileNameFound.isNull()) {
        if (isProvisional) {
            qCDebug(caImageLog) << "provisional file" << filename << "could not be found";
        } else {
            qCCritical(caImageLog) << "file" << filename << "does not exist";
        }
        delete s;
        return;
    }

    delete s;

    // Scalable images (svg/svgz) are rendered from their vector description at the size the
    // widget actually has. Going through QMovie/QImageReader would rasterize them once at the
    // size stored in the file and then scale that bitmap up, which looks blocky on large panels.
    if(initSvg(fileNameFound)) return;

    _animation = new QMovie(fileNameFound, 0, this);
    _animation->setCacheMode(QMovie::CacheAll);
    _animation->jumpToFrame(0);
    connect(_animation, SIGNAL(frameChanged(int)), this, SLOT(OnFrameChanged(int)));
    if( _animation.isNull()) return;
    // display the movie
    _container->setScaledContents(true);
    _container->setMovie(_animation);

    pixmap = _animation->currentPixmap();
    pix    = _animation->currentPixmap();

    if(thisAngle != 0) OnFrameChanged(0);

    _layout->setSpacing(0);
    SETMARGIN_QT456(_layout,0);
    _layout->addWidget(_container);
    setLayout(_layout);

    setHidden(false);
}

bool caImage::isSvgFileName(const QString& filename)
{
    const QString suffix = QFileInfo(filename).suffix().toLower();
    return (suffix == "svg" || suffix == "svgz");
}

/**
  * set up rendering straight from the vector description; returns false when the file is not
  * a (valid) svg, so that the caller can fall back to the bitmap path
  */
bool caImage::initSvg(const QString& fileNameFound)
{
    thisIsSvg = false;
    // a renderer of a previously shown svg is not needed any more, whatever the new file is
    if(_svg != Q_NULLPTR) {
        delete _svg;
        _svg = Q_NULLPTR;
    }
    if(!isSvgFileName(fileNameFound)) return false;

    _svg = new QSvgRenderer(fileNameFound, this);
    if(!_svg->isValid()) {
        qCWarning(caImageLog) << "caimage:" << tr("file %1 is not a valid svg, falling back to bitmap rendering").arg(fileNameFound);
        delete _svg;
        _svg = Q_NULLPTR;
        return false;
    }
    thisIsSvg = true;

    // no bitmap scaling, we rasterize at exactly the size we are given
    _container->setScaledContents(false);
    _container->setAlignment(Qt::AlignCenter);
    if(!_animation.isNull()) {
        _container->setMovie(Q_NULLPTR);
        delete _animation;
    }

    _layout->setSpacing(0);
    SETMARGIN_QT456(_layout,0);
    if(_layout->indexOf(_container) < 0) _layout->addWidget(_container);
    setLayout(_layout);

    renderSvg();
    setHidden(false);
    return true;
}

void caImage::renderSvg()
{
    if(!thisIsSvg || _svg == Q_NULLPTR || _container.isNull()) return;

    // make sure the label already has the geometry the layout gives it, otherwise the first
    // rendering after loading the file would use a stale size
    if(_layout) _layout->activate();
    QSize target = _container->size();
    if(target.isEmpty()) target = size();
    if(target.isEmpty()) return;

    // rasterize for the physical pixels of the screen, so that the result stays sharp
    // on high dpi displays as well
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    const qreal dpr = devicePixelRatioF();
#else
    const qreal dpr = devicePixelRatio();
#endif
    QPixmap pm(QSize(qRound(target.width() * dpr), qRound(target.height() * dpr)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pm.setDevicePixelRatio(dpr);
#endif
    pm.fill(Qt::transparent);

    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if(thisAngle != 0) {
        painter.translate(target.width() / 2.0, target.height() / 2.0);
        painter.rotate(thisAngle);
        painter.translate(-target.width() / 2.0, -target.height() / 2.0);
    }
    // the svg is stretched into the widget rectangle, as the bitmap path does
    _svg->render(&painter, QRectF(0, 0, target.width(), target.height()));
    painter.end();

    pixmap = pm;
    _container->setPixmap(pm);
}

void caImage::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    // vector images have to be rasterized again for the new size
    if(thisIsSvg) renderSvg();
}

int caImage::getFrameCount()
{
    if(thisIsSvg) return 1;
    if( _animation.isNull()) return 0;
    return _animation->frameCount();
}

void caImage::timerEvent(QTimerEvent *)
{
    if( _animation.isNull()) return;
    if(thisFrame > (_animation->frameCount()-1)) {
        thisFrame=0;
    }
    // display only when frame changed
    if(thisFrame != prevFrame) {
      (void)_animation->jumpToFrame(thisFrame);
      prevFrame = thisFrame;
    }
    thisFrame++;
}

void caImage::startMovie()
{
    // kill default timer
    if(timerId != 0) killTimer(timerId);
    //start timer, but 0 milliseconds means no timer
    if(thisDelay > 0) timerId = startTimer(thisDelay);
}

void caImage::setInvalid(QColor c)
{
    if(c != oldColor) {
      QString style = "color: rgb(%1, %2, %3); background-color: rgb(%4, %5, %6);";
      style = style.arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.red()).arg(c.green()).arg(c.blue());
      _container->setStyleSheet(style);
      _container->setMovie(NULL);
      // the pixmap would cover the invalid color
      if(thisIsSvg) _container->setPixmap(QPixmap());
      oldColor = c;
    }
}

void caImage::setValid()
{
    QColor c;
    if(oldColor == Qt::gray) return;
    if(thisIsSvg) renderSvg();
    else _container->setMovie(_animation);
    c = oldColor = Qt::gray;
    QString style = "color: rgb(%1, %2, %3); background-color: rgba(%4, %5, %6, %7);";
    style = style.arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.red()).arg(c.green()).arg(c.blue()).arg(0);
    _container->setStyleSheet(style);
}

void caImage::setFileName(QString filename, bool isProvisional)
{
    thisFileName = filename;
    init(thisFileName, isProvisional);
}

void caImage::setFrame(int frame)
{
    thisFrame = frame;
    if(thisIsSvg) return;          // a vector image has a single frame
    if( _animation.isNull()) return;
    (void)_animation->jumpToFrame(frame);
    prevFrame= thisFrame;
}

void caImage::setAngle( int angle)
{
    if (angle >= 0 && angle <= 360) {
        thisAngle = angle;
        OnFrameChanged(thisFrame);
    }
}

void caImage::slotTiltAngle(int angle)
{
    setAngle(angle);
}

void caImage::slotTiltAngle(double angle)
{
    setAngle(qRound(angle));
}

void caImage::OnFrameChanged(int frame)
{
    Q_UNUSED(frame)
    if(thisIsSvg) {
        renderSvg();
        return;
    }
    if( _animation.isNull()) return;
    pixmap = pixmap.scaled(width(), height());
    pixmap = _animation->currentPixmap();
    if(thisAngle == 0) {
        _container->setPixmap (pixmap);
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMatrix rm;
#else
    QTransform rm;
#endif


    pix.scaled(width(), height());
    pix.fill(QColor::fromRgb(0, 0, 0, 0)); //pixmap transparent.
    QPainter* p = new QPainter(&pix);
    QSize size = pixmap.size();
    p->translate(size.height()/2,size.height()/2);
    p->rotate(thisAngle);
    p->translate(-size.height()/2,-size.height()/2);
    p->drawPixmap(0, 0, pixmap);
    p->end();
    delete p;
    _container->setPixmap(pix);
}
