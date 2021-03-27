#include "kalextended.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
KalExtended::KalExtended(const QCoreTraceFilter &other)
        : QTraceFilter(other),
          m_dR(0.0e0) {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
KalExtended::~KalExtended() {
    // qDebug() << "inside ~KalExtended(): " << m_dMeasNoise;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool KalExtended::eatPrimaryPoint(int iPtIdx) {
    sPrimaryPt *pPriPt = dynamic_cast<struct KalExtended::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
    if (!pPriPt) return false; // not valid list index or failed dynamic_cast
    // from now on we can guarantee that pPriPt has type <QTraceFilter::sPrimaryPt*>

    // ... process the primary point sPrimaryPt *pPriPt

    // filter accepts the primary point
    pPriPt->pFlt = this;
    m_qlPriPts.append(iPtIdx);
    // dummy state
    m_dR=m_qlAllPriPts.at(iPtIdx)->dR;
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double KalExtended::calcProximity(const QCoreTraceFilter::sPrimaryPt *pPrimaryPoint) const {
    return abs(pPrimaryPoint->dR-m_dR);
}
