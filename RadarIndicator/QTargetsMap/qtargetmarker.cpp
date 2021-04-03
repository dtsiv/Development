#include "qtargetmarker.h"
#include "qformular.h"
#include "qvoi.h"

QList<QPixmap *>QTargetMarker::m_qlPPointSymbol;            // primary point
QList<QPixmap *>QTargetMarker::m_qlCPointSymbol;            // cluster point
QList<QPixmap *>QTargetMarker::m_qlNewestPoisitionSymbol;   // current filter state
QList<QPixmap *>QTargetMarker::m_qlTrajectorySymbol;        // previous filter state

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTargetMarker::QTargetMarker(const sVoiPrimaryPointInfo &sPriPtInfo)
        : QObject(NULL)
        , m_pFormular(NULL)
        , m_qpTarPhys(sPriPtInfo.qpfDistVD)
        , m_qsMesg(sPriPtInfo.qsFormular)
        , m_iPriPtIdx(sPriPtInfo.uPriPtIndex)
        , m_uFilterIdx(sPriPtInfo.uFilterIndex) {
    m_emtType = PRIMARY_POINT;
    if (!m_qlPPointSymbol.size()) prepareTypeMarkers();
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
        , m_uFilterIdx(pFilterInfo->uFilterIndex)
        , m_bNewestFlag(true) {
    if (!pFilterInfo->bTrackingOn) {
        m_emtType = CLUSTER_POINT;
    }
    else {
        m_emtType = TRAJECTORY_POINT;
    }
    if (!m_qlPPointSymbol.size()) prepareTypeMarkers();
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
void QTargetMarker::prepareTypeMarkers() {
    int iStateIteratorLast = eMarkerState::Last;
    QColor qcPoppy(255,66,14,255);
    QColor qcSpringGreen(137,218,89,255);
    // current filter state markers
    for (int iState=0; iState<iStateIteratorLast; iState++){
        eMarkerState emsState = static_cast<eMarkerState>(iState);
        QImage iMarker(11,11,QImage::Format_ARGB32_Premultiplied);
        iMarker.fill(Qt::transparent);
        QPainter prMarker(&iMarker);
        int iMarkerThickness=2;
        QColor qcMarker = ((emsState == HIGHLIGHTED) ? qcPoppy : Qt::darkRed);
        QBrush qbMarker(qcMarker,Qt::SolidPattern);
        prMarker.setBrush(qbMarker);
        QPen qpMarkerPen(qbMarker,iMarkerThickness,Qt::SolidLine,Qt::RoundCap);
        prMarker.setPen(qpMarkerPen);
        QPoint qpTarPix(5,5);
        QPoint qpBranch(5,5);
        prMarker.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
        qpBranch=QPoint(5,-5);
        prMarker.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
        // QPixmap pm=QPixmap::fromImage(iMarker,Qt::ThresholdDither | Qt::NoFormatConversion | Qt::ThresholdAlphaDither);
        m_qlNewestPoisitionSymbol.append(new QPixmap(QPixmap::fromImage(iMarker)));
    }
    // primary point markers
    for (int iState=0; iState<iStateIteratorLast; iState++){
        eMarkerState emsState = static_cast<eMarkerState>(iState);
        QImage iMarker(7,7,QImage::Format_ARGB32_Premultiplied);
        iMarker.fill(Qt::transparent);
        QPainter prMarker(&iMarker);
        int iMarkerThickness=2;
        QColor qcMarker = ((emsState == HIGHLIGHTED) ? qcPoppy : qcSpringGreen);
        QBrush qbMarker(qcMarker,Qt::SolidPattern);
        prMarker.setBrush(qbMarker);
        QPen qpMarkerPen(qbMarker,iMarkerThickness,Qt::SolidLine,Qt::RoundCap);
        prMarker.setPen(qpMarkerPen);
        QPoint qpTarPix(3,3);
        QPoint qpBranch(2,2);
        prMarker.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
        qpBranch=QPoint(2,-2);
        prMarker.drawLine(qpTarPix-qpBranch,qpTarPix+qpBranch);
        // QPixmap pm=QPixmap::fromImage(iMarker,Qt::ThresholdDither | Qt::NoFormatConversion | Qt::ThresholdAlphaDither);
        m_qlPPointSymbol.append(new QPixmap(QPixmap::fromImage(iMarker)));
    }
    // cluster markers
    for (int iState=0; iState<iStateIteratorLast; iState++){
        eMarkerState emsState = static_cast<eMarkerState>(iState);
        QImage iMarker(9,9,QImage::Format_ARGB32_Premultiplied);
        iMarker.fill(Qt::transparent);
        QPainter prMarker(&iMarker);
        int iMarkerThickness=2;
        QColor qcMarker = ((emsState == HIGHLIGHTED) ? qcPoppy : Qt::black);
        QPen qpMarkerPen(qcMarker);
        qpMarkerPen.setWidth(iMarkerThickness);
        prMarker.setPen(qpMarkerPen);
        QPoint qpTarPix(4,4);
        prMarker.drawEllipse(qpTarPix,3,3);
        m_qlCPointSymbol.append(new QPixmap(QPixmap::fromImage(iMarker)));
    }
    // trajectory point markers
    for (int iState=0; iState<iStateIteratorLast; iState++){
        eMarkerState emsState = static_cast<eMarkerState>(iState);
        QImage iMarker(7,7,QImage::Format_ARGB32_Premultiplied);
        iMarker.fill(Qt::transparent);
        QPainter prMarker(&iMarker);
        int iMarkerThickness=1;
        QColor qcMarker = ((emsState == HIGHLIGHTED) ? qcPoppy : Qt::black);
        QBrush qbMarker(qcMarker,Qt::SolidPattern);
        prMarker.setBrush(qbMarker);
        QPen qpMarkerPen(qbMarker,iMarkerThickness,Qt::SolidLine,Qt::RoundCap);
        prMarker.setPen(qpMarkerPen);
        QPoint qpTarPix(3,3);
        prMarker.drawEllipse(qpTarPix,2,2);
        m_qlTrajectorySymbol.append(new QPixmap(QPixmap::fromImage(iMarker)));
    }
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
void QTargetMarker::drawMarker(QPainter &painter, QTransform &t, bool bHighlighted /* = false */) {
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
    QPixmap *ppm=NULL;
    int iMarkerState = (int)(bHighlighted?HIGHLIGHTED:NORMAL);
    switch (m_emtType) {
        case PRIMARY_POINT:
            ppm = m_qlPPointSymbol.at(iMarkerState);
            break;
        case CLUSTER_POINT:
            ppm = m_qlCPointSymbol.at(iMarkerState);
            break;
        case TRAJECTORY_POINT:
            ppm = (m_bNewestFlag ? m_qlNewestPoisitionSymbol.at(iMarkerState)
                                 : m_qlTrajectorySymbol.at(iMarkerState));
        break;
    }
    if (ppm) {
        QSize szPm = ppm->size();
        painter.drawPixmap(qpTarPix.x()-qFloor(0.5e0*szPm.height()),
                           qpTarPix.y()-qFloor(0.5e0*szPm.width()),
                           *ppm);
    }
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
