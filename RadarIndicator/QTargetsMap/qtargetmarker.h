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
        TRAJECTORY_POINT
    };
    enum eMarkerState {
        NORMAL,
        HIGHLIGHTED,
        Last
    };

    explicit QTargetMarker(const sVoiPrimaryPointInfo &sPriPtInfo);
    explicit QTargetMarker(sVoiFilterInfo *pFilterInfo);
    ~QTargetMarker();
    QPointF tar();
    bool hasFormular();
    void setFormular(QFormular *pFormular);
    void drawMarker(QPainter &painter, QTransform &t, bool bHighlighted = false);
    QString &mesgString();
    void prepareTypeMarkers();
    void clearNewestFlag() { m_bNewestFlag = false; }
    bool isPrimaryPoint() { return (m_emtType == PRIMARY_POINT); }

    // index of the primary point in module QVoi
    int m_iPriPtIdx;
    quint32 m_uFilterIdx;
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
    // for trajectory points only
    bool m_bNewestFlag;
    // pre-defined markers
    static QList<QPixmap *>m_qlPPointSymbol;   // primary point
    static QList<QPixmap *>m_qlCPointSymbol;   // cluster point
    static QList<QPixmap *>m_qlNewestPoisitionSymbol;  // current filter state
    static QList<QPixmap *>m_qlTrajectorySymbol;  // previous filter state
};

#endif // QTARGETMARKER_H
