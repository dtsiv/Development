#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QObject>
#include <QOpenGLWidget>

#define MAP_WINDOW_MINIMUM_WIDTH  200
#define MAP_WINDOW_MINIMUM_HEIGHT 200

class QTargetsMap;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MapWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    MapWidget(QTargetsMap *owner, QWidget *parent = 0);
    ~MapWidget();

protected:
    void paintEvent(QPaintEvent *qpeEvent);
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QTargetsMap *m_pOwner;
    // used to translate coordinate grid with mouse drag
    QPoint m_qpLastPoint;
    bool m_bMapDragging;
    bool m_bFormularMoving;
};

#endif // MAPWIDGET_H
