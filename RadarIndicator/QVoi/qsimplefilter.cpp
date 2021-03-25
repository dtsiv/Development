#include "qsimplefilter.h"
#include "qtracefilter.h"

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "nr.h"
using namespace std;

#include "cmatrix"
typedef techsoft::matrix<double> Matrix;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimpleFilter::sPrimaryPt::sPrimaryPt(QCoreTraceFilter::sPrimaryPt &other)
        : QTraceFilter::sPrimaryPt(other) {
    // calculate the fields of derived class
    dCosE = cos(dElRad);
    dSinE = sin(dElRad);
    dCosB = cos(dAzRad);
    dSinB = sin(dAzRad);
    // coordinates in stationary (radar-based) EUN Cartesian system
    dX = dR*dCosE*dCosB;
    dY = dR*dSinE;
    dZ = dR*dCosE*dSinB;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimpleFilter::QSimpleFilter(const QCoreTraceFilter &other)
        : QTraceFilter(other)
        , m_dR(0.0e0)
        , m_dV_D(0.0e0) {
    if (!m_qlAllPriPts.isEmpty()) {
        int i = m_qlAllPriPts.size()-1;
        qDebug() << "Filter created : (" << m_qlAllPriPts.at(i)->dR << ", " <<m_qlAllPriPts.at(i)->dV_D << ")";
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimpleFilter::~QSimpleFilter() {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::eatPrimaryPoint(int iPtIdx) {
    sPrimaryPt *pPriPt = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
    if (!pPriPt) return false; // not valid list index or failed dynamic_cast
    // from now on we can guarantee that pPriPt has type <QTraceFilter::sPrimaryPt*>
    if (m_dR!=0.0e0 && (calcProximity(*pPriPt)>500 || abs(pPriPt->dV_D-m_dV_D)>30.0e0)) { // filter rejects this pt
        return false;
    }
    // primary point accepted
    pPriPt->pFlt = this;
    m_qlPriPts.append(iPtIdx);
    // if trajectory was started ...
    if (m_bTrackingOn) {
        return filterStep(*pPriPt);
    }
    // .. else append primary point to cluster
    m_qlCluster.append(iPtIdx);
    m_sFS.dX=pPriPt->dX;
    m_sFS.dY=pPriPt->dY;
    m_sFS.dZ=pPriPt->dZ;
    m_sFS.dVX=0.0e0;
    m_sFS.dVY=0.0e0;
    m_sFS.dVZ=0.0e0;
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double QSimpleFilter::calcProximity(const QCoreTraceFilter::sPrimaryPt &primaryPoint) const {
    return abs(primaryPoint.dR-m_dR);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::filterStep(const struct sPrimaryPt &pp) {
    double dDeltaT = pp.dTs-m_sFS.dT;
    if (dDeltaT < 1.0e-6*dEPS) return false;
    m_sFS.dT=pp.dTs;
    m_sFS.dX+=m_sFS.dVX*dDeltaT;
    m_sFS.dY+=m_sFS.dVY*dDeltaT;
    m_sFS.dZ+=m_sFS.dVZ*dDeltaT;
    return true;
}



























//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//void QSimpleFilter::print() {
//    QListIterator<int> i(m_qlPriPts);
//    while (i.hasNext()) {
//        sPrimaryPt *pPriPt = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(i.next(),pPriPtNULL));
//        if (!pPriPt) {
//            qDebug() << "QSimpleFilter::print() - dynamic cast failed";
//            return; // not valid list index or failed dynamic_cast
//        }
//        QSimpleFilter *pFlt=dynamic_cast<QSimpleFilter *>(pPriPt->pFlt);
//        if (!pFlt) {
//            qDebug() << "QSimpleFilter::print() - dynamic cast failed #2";
//            return; // not valid list index or failed dynamic_cast
//        }
//        qDebug() << (pFlt==this)
//                 << " X=" << pPriPt->dX
//                 << " Y=" << pPriPt->dY
//                 << " Z=" << pPriPt->dZ;
//    }
//}
