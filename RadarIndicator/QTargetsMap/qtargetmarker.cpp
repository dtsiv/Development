#include "qtargetmarker.h"
#include "qformular.h"
#include "qvoi.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTargetMarker::QTargetMarker(const sVoiPrimaryPointInfo &sPriPtInfo)
        : QObject(NULL)
        , m_pFormular(NULL)
        , m_qpTarPhys(sPriPtInfo.qpfDistVD)
        , m_qsMesg(sPriPtInfo.qsFormular)
        , m_iPriPtIdx(sPriPtInfo.uPriPtIndex)
        , m_iFltIdx(sPriPtInfo.uFilterIndex) {
    m_emtType = PRIMARY_POINT;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTargetMarker::QTargetMarker(sVoiFilterInfo *pFilterInfo)
        : QObject(NULL)
        , m_pFormular(NULL)
        , m_qpTarPhys(pFilterInfo->qpfDistVD)
        , m_qsMesg(pFilterInfo->qsFormular)
        , m_iPriPtIdx(-1)
        , m_iFltIdx(pFilterInfo->uFilterIndex) {
    if (!pFilterInfo->bTrackingOn) {
        m_emtType = CLUSTER_POINT;
    }
    else {
        m_emtType = TRAJECTORY;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTargetMarker::~QTargetMarker() {
    if (m_pFormular) m_pFormular->setStale();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QTargetMarker::hasFormular() {
    return (m_pFormular != NULL);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QTargetMarker::setFormular(QFormular *pFormular) {
    if (m_pFormular) return;
    m_pFormular = pFormular;
    QObject::connect(m_pFormular,SIGNAL(destroyed(QObject*)),SLOT(onFormularClosed(QObject*)));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QTargetMarker::drawMarker(QPainter &painter, QTransform &t) {
    // save painter state
    painter.save();

    // Position of target in pixels (view coordinates)
    // qDebug() << "m_qpTarPhys = " << m_qpTarPhys;
    QPoint qpTarPix = (m_qpTarPhys * t).toPoint();
    // qDebug() << "qpTarPix = " << qpTarPix;
    // target outside widget area - skip
    QRect qrBounding=painter.window();
    // qDebug() << "qrBounding = " << qrBounding;
    if (!qrBounding.contains(qpTarPix)) {
        painter.restore(); return;
    }

    // Draw the marker
    int iMarkerThickness=2;
    QBrush qbMarker(Qt::darkRed,Qt::SolidPattern);
    painter.setBrush(qbMarker);
    QPen qpMarkerPen(qbMarker,iMarkerThickness,Qt::SolidLine,Qt::RoundCap);
    painter.setPen(qpMarkerPen);
    QPoint qpBranch(5,5);
    painter.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
    qpBranch=QPoint(5,-5);
    painter.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
    // restore painter state
    painter.restore();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QPointF QTargetMarker::tar() {
    return m_qpTarPhys;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QString & QTargetMarker::mesgString() {
    return m_qsMesg;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QTargetMarker::onFormularClosed([[maybe_unused]]QObject *pFormular /* = Q_NULLPTR */) {
    m_pFormular = 0;
}
