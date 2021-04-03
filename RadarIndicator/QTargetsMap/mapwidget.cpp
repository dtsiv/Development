#include "mapwidget.h"
#include "qtargetsmap.h"

#include <QOpenGLFunctions>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MapWidget::MapWidget(QTargetsMap *owner, QWidget *parent /* =0 */)
      : QOpenGLWidget(parent)
      , m_bMapDragging(false)
      , m_pOwner(owner)
      , m_bFormularMoving(false) {
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    setMinimumWidth(MAP_WINDOW_MINIMUM_WIDTH);
    setMinimumHeight(MAP_WINDOW_MINIMUM_HEIGHT);
    setMouseTracking(true);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MapWidget::~MapWidget() {
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    // makeCurrent();
    // delete m_texture; delete m_shader; delete m_program;
    // m_vbo.destroy(); m_vao.destroy();
    // doneCurrent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MapWidget::paintEvent(QPaintEvent *qpeEvent) {
    m_pOwner->mapPaintEvent(this, qpeEvent);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MapWidget::initializeGL() {
          // Set up the rendering context, load shaders and other resources, etc.:
          QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
          f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
          // ...
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MapWidget::paintGL() {
          // Draw the scene:
          QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
          f->glClear(GL_COLOR_BUFFER_BIT);
          // ...
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MapWidget::resizeGL([[maybe_unused]]int w, [[maybe_unused]]int h) {
    // !!!doesn't work:  glViewport(0, 0, w/2, h/2);
    // glViewport() should instead appear in paintGL
    // ...
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::mousePressEvent(QMouseEvent *pe) {
    if (pe->button() == Qt::LeftButton) {
        QPoint qpPos = pe->pos();
        m_bFormularMoving = m_pOwner->getFirstFormular(qpPos);
        if (m_bFormularMoving) {
            m_bMapDragging = false;
            // qDebug() << "m_bFormularMoving = true";
        }
        else {
            m_bMapDragging = true;
        }
        m_qpLastPoint = qpPos;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::mouseMoveEvent(QMouseEvent *pe) {
    if ((pe->buttons() & Qt::LeftButton) && (m_bMapDragging || m_bFormularMoving)) {
        QPoint qpDelta = pe->pos() - m_qpLastPoint;
        m_qpLastPoint = pe->pos();
        if (m_bFormularMoving) {
            // qDebug() << "call to shiftFormular(qpDelta)";
            m_pOwner->shiftFormular(qpDelta);
        }
        else {
            m_pOwner->shiftView(qpDelta);
        }
        repaint();
        return;
    }
    // free mouse move, no associated action yet
    if (!(m_bMapDragging || m_bFormularMoving)
            && pe->button()==Qt::NoButton
            && pe->modifiers()==Qt::NoModifier) {
        quint32 uFilterIdxHighlighted = m_pOwner->m_uFilterIdxHighlighted;
        m_pOwner->getFirstMarker(pe->pos());
        if (m_pOwner->m_uFilterIdxHighlighted != uFilterIdxHighlighted) repaint();
    }
    // mark mouse last position
    if (!m_pOwner->m_pMouseStill) m_pOwner->m_pMouseStill = new struct QFormular::sMouseStillPos;
    m_pOwner->m_pMouseStill->pos = pe->pos();
    m_pOwner->m_pMouseStill->last = QTime::currentTime();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::mouseReleaseEvent(QMouseEvent *pe) {
    if (pe->button() == Qt::LeftButton && m_bMapDragging) {
        QPoint qpDelta = pe->pos() - m_qpLastPoint;
        if (m_bFormularMoving) {
            m_pOwner->shiftFormular(qpDelta);
        }
        else {
            m_pOwner->shiftView(qpDelta);
        }
        m_bMapDragging = false;
        m_bFormularMoving = false;
        repaint();
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::wheelEvent(QWheelEvent *pe) {
    if (pe->modifiers() & Qt::ControlModifier) {
        int iNumSteps = pe->delta();
        bool bMagnify = (iNumSteps>0);
        if (pe->modifiers() & Qt::ShiftModifier) {
            [[maybe_unused]]int iEventX = pe->x();
            [[maybe_unused]]int iEventY = pe->y();
            m_pOwner->zoomMap(this, iEventX, iEventY, bMagnify);
            repaint();
            return;
        }
        bool bZoomAlongD = true;
        bool bZoomAlongV = true;
        m_pOwner->zoomMap(this, bZoomAlongD, bZoomAlongV, bMagnify);
        repaint();
        return;
    }
    return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::resizeEvent([[maybe_unused]]QResizeEvent *event) {
    QOpenGLWidget::resizeEvent(event);
    if (m_pOwner->m_pMouseStill) {
        delete m_pOwner->m_pMouseStill;
        m_pOwner->m_pMouseStill = NULL;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ void MapWidget::keyReleaseEvent(QKeyEvent *pe) {
    if (     pe->key()==Qt::Key_Up
          || pe->key()==Qt::Key_Right
          || pe->key()==Qt::Key_Down
          || pe->key()==Qt::Key_Left) {
        switch (pe->key()) {
            case Qt::Key_Up:
                m_pOwner->shiftView(/*bool bVertical=*/true, /*bool bPositive=*/true); break;
            case Qt::Key_Down:
                m_pOwner->shiftView(/*bool bVertical=*/true, /*bool bPositive=*/false); break;
            case Qt::Key_Right:
                m_pOwner->shiftView(/*bool bVertical=*/false, /*bool bPositive=*/true); break;
            case Qt::Key_Left:
                m_pOwner->shiftView(/*bool bVertical=*/false, /*bool bPositive=*/false); break;
            default:
                // allow all other events
                QOpenGLWidget::keyPressEvent(pe);
                return;
        }
        repaint();
        return;
    }
    else if ((pe->key()==Qt::Key_Plus || pe->key()==Qt::Key_Minus
           || pe->key()==Qt::Key_Equal|| pe->key()==Qt::Key_Bar
           || pe->key()==Qt::Key_Underscore)
           && ((pe->modifiers() & Qt::ControlModifier)
           ||  (pe->modifiers() & Qt::ShiftModifier) )) {
        Qt::KeyboardModifiers kbmZoomAlong = pe->modifiers();
        int iKey = pe->key();
        bool bZoomAlongD = (kbmZoomAlong & Qt::ShiftModifier);
        bool bZoomIn = ((iKey==Qt::Key_Plus) || (iKey==Qt::Key_Equal));
        m_pOwner->zoomMap(this, bZoomAlongD, !bZoomAlongD, bZoomIn);
        repaint();
        return;
    }
    else if ( pe->key()==Qt::Key_0 && pe->modifiers() & Qt::ControlModifier) {
        m_pOwner->resetScale();
        repaint();
        return;
    }
    // allow all other events
    QOpenGLWidget::keyPressEvent(pe);
    return;
}
