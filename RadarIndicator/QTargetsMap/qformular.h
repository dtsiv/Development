#ifndef QFORMULAR_H
#define QFORMULAR_H

#include <QObject>
#include <QTime>
#include <QPoint>
#include <QPainter>
#include <QRect>

#include "qtargetmarker.h"

class MapWidget;

class QFormular : public QObject
{
    Q_OBJECT
public:
    struct sMouseStillPos {
        QTime last;
        QPoint pos;
    };

public:
    explicit QFormular(struct sMouseStillPos *pMouseStill,
                       QObject *parent = 0);

    void drawFormular(QPainter &painter, QTransform &t);
    bool selectTarg(QList<QTargetMarker*> ql_Targets, QTransform &t);
    void setStale();
    bool isStale();
    static const quint64 m_uFormularLifetime;
    static const quint64 m_uFormularCutoffPix;
    bool contains(QPoint qp);
    QPoint m_qpOffset;

signals:

public slots:

private:
    // pointer to QTargetMarker (if any)
    QTargetMarker *m_pTargetMarker;
    bool m_bStale;
    struct sMouseStillPos m_mouseStill;
    QRect m_qrFormularRect;
};

#endif // QFORMULAR_H
