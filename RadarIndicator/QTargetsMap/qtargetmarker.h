#ifndef QTARGETMARKER_H
#define QTARGETMARKER_H

#include <QtCore>
#include <QtGlobal>
#include <QObject>
#include <QPoint>
#include <QPainter>

class QFormular;
struct sVoiFilterInfo;
struct sVoiPrimaryPointInfo;

class QTargetMarker : public QObject
{
    Q_OBJECT
public:
    enum eMarkerType {
        PRIMARY_POINT,
        CLUSTER_POINT,
        TRAJECTORY
    };

    explicit QTargetMarker(const sVoiPrimaryPointInfo &sPriPtInfo);
    explicit QTargetMarker(sVoiFilterInfo *pFilterInfo);
    ~QTargetMarker();
    QPointF tar();
    bool hasFormular();
    void setFormular(QFormular *pFormular);
    void drawMarker(QPainter &painter, QTransform &t);
    QString &mesgString();

    // index of the primary point in module QVoi
    int m_iPriPtIdx;
    int m_iFltIdx;
    enum eMarkerType m_emtType;

signals:

public slots:
    void onFormularClosed(QObject*);

private:
    QFormular *m_pFormular;
    // physical coordinates of target
    QPointF m_qpTarPhys; // physical coordinate D (m), V(m/s) of target
    // comment on target
    QString m_qsMesg;
    // beam
    int m_iBeamNo;
    // power
    double m_dPower;
};

#endif // QTARGETMARKER_H
