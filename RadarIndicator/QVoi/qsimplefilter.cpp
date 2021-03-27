#include "qsimplefilter.h"
#include "qtracefilter.h"
#include "qexceptiondialog.h"

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
    // qDebug() << QString("PriPt (%1,%2,%3) el=%4").arg(dX).arg(dY).arg(dZ).arg(dElRad);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimpleFilter::QSimpleFilter(const QCoreTraceFilter &other)
        : QTraceFilter(other)
        , m_pFS(NULL) {
    //    if (!m_qlAllPriPts.isEmpty()) {
    //        int i = m_qlAllPriPts.size()-1;
    //        qDebug() << "Filter created : (" << m_qlAllPriPts.at(i)->dR << ", " <<m_qlAllPriPts.at(i)->dV_D << ")";
    //    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimpleFilter::~QSimpleFilter() {
    if (m_pFS) delete m_pFS; m_pFS=NULL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::eatPrimaryPoint(int iPtIdx) {
    sPrimaryPt *pPriPt = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
    if (!pPriPt) return false; // not valid list index or failed dynamic_cast
    // from now on we can guarantee that pPriPt has type <QTraceFilter::sPrimaryPt*>
    if (!isInsideSpaceStrobe(pPriPt)) { // filter rejects this pt
        return false;
    }
    // if trajectory was started ...
    if (m_bTrackingOn) {
        if (filterStep(*pPriPt)) {
            // primary point accepted
            m_qlPriPts.append(iPtIdx);
            pPriPt->pFlt = this;
            return true;
        }
        else {
            qDebug() << "QSimpleFilter::eatPrimaryPoint() filterStep failed";
            return false;
        }
    }
    // ... else append primary point to cluster
    if (m_qlCluster.isEmpty() // first point in cluster or point in line of cluster
         || (!m_qlCluster.isEmpty() && linearRegression(pPriPt))) { // primary point accepted
        pPriPt->pFlt = this;
        m_qlCluster.append(iPtIdx);
        m_qlPriPts.append(iPtIdx);
        // qDebug() << "Pt appended to cluster";
        return true;
    }
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::linearRegression(const struct sPrimaryPt *pPriPt) {
    int N = m_qlCluster.size();
    if (N<1) throw RmoException("QSimpleFilter::linearRegression: N=0");
    double dTavr=0.0e0,dTsigma2=0.0e0;
    double dXavr=0.0e0,dXT=0.0e0;
    double dYavr=0.0e0,dYT=0.0e0;
    double dZavr=0.0e0,dZT=0.0e0;
    double dV_D, dVDWin;
    QList<double> qlT,qlX,qlY,qlZ;
    for (int i=0; i<N; i++) {
        int iPtIdx = m_qlCluster.value(i,-1);
        if (iPtIdx<0) {
            throw RmoException("QSimpleFilter::linearRegression: iPtIdx>=m_qlAllPriPts.size()");
            return false;
        }
        sPrimaryPt *pp = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
        if (!pp) {
            throw RmoException("QSimpleFilter::linearRegression: dynamic_cast failed");
            return false;
        }
        qlT<<pp->dTs; qlX<<pp->dX; qlY<<pp->dY; qlZ<<pp->dZ;
        dV_D = pp->dV_D;
        dVDWin = pp->dVDWin;
    }
    double dVV = remainder(pPriPt->dV_D - dV_D, pPriPt->dVDWin);
    dVV = abs(dVV) / pPriPt->dVDWin;
    // cannot accept primary point with different Doppler velocity
    if (pPriPt->dVDWin != dVDWin) {
        qDebug() << "Rejecting different Doppler window: pPriPt->dVDWin != dVDWin" << pPriPt->dVDWin << " <> " << dVDWin;
        return false;
    }
    if (dVV > 0.2) {
        // qDebug() << "Rejecting different Doppler velocity: " << pPriPt->dV_D << " <> " << dV_D << " dVV=" << dVV;
        return false;
    }
    // append pPriPt to the lists
    qlT<<pPriPt->dTs; qlX<<pPriPt->dX; qlY<<pPriPt->dY; qlZ<<pPriPt->dZ; N++;
    // calculate averages
    for (int i=0; i<N; i++) {
        dTavr+=qlT.at(i); dXavr+=qlX.at(i); dYavr+=qlY.at(i); dZavr+=qlZ.at(i);
    }
    dTavr/=N; dXavr/=N; dYavr/=N; dZavr/=N;
    // calculate sigma2
    for (int i=0; i<N; i++) {
        double dT=qlT.at(i)-dTavr;
        double dX=qlX.at(i)-dXavr;
        double dY=qlY.at(i)-dYavr;
        double dZ=qlZ.at(i)-dZavr;
        dTsigma2+=dT*dT; dXT+=dX*dT; dYT+=dY*dT; dZT+=dZ*dT;
    }
    double dTsigma=sqrt(dTsigma2);
    if(dTsigma<dEPS) {
        throw RmoException("QSimpleFilter::linearRegression: sqrt(dTsigma2)<dEPS");
        return false;
    }
    // linear regression coefficients
    double dVX=dXT/dTsigma2; double dVY=dYT/dTsigma2; double dVZ=dZT/dTsigma2;
    // velocity direction unit vector
    double dVX2=dVX*dVX; double dVY2=dVY*dVY; double dVZ2=dVZ*dVZ;
    double dV=sqrt(dVX2+dVY2+dVZ2);
    if (dV<dEPS) return true; // accept targets with no motion
    double dnVX=dVX/dV; double dnVY=dVY/dV; double dnVZ=dVZ/dV;
    // correlation coefficient
    if (N>2) {
        QList<double> dRp;
        for (int i=0; i<N; i++) {
            dRp<<(qlX.at(i)*dnVX+qlY.at(i)*dnVY+qlZ.at(i)*dnVZ);
        }
        double dRpavr=0.0e0;
        double dRpsigma2=0.0e0;
        double dRpT=0.0e0;
        for (int i=0; i<N; i++) {
            dRpavr+=dRp.at(i);
        }
        dRpavr/=N;
        for (int i=0; i<N; i++) {
            dRpT+=(dRp.at(i)-dRpavr)*(qlT.at(i)-dTavr);
            dRpsigma2+=(dRp.at(i)-dRpavr)*(dRp.at(i)-dRpavr);
        }
        double dCorrCoef=dRpT/dTsigma/sqrt(dRpsigma2);
        // reject low correlation -- IMPROVE with significance level!!!
        if (abs(dCorrCoef)<0.5) {
            qDebug() << "Rejecting CorrCoef = " << dCorrCoef;
            return false;
        }
    }
    m_pFS->dT=pPriPt->dTs; m_pFS->dVX=dVX; m_pFS->dVY=dVY; m_pFS->dVZ=dVZ;
    double dDeltaT=pPriPt->dTs-dTavr;
    m_pFS->dX=dXavr+dVX*dDeltaT;
    m_pFS->dY=dYavr+dVY*dDeltaT;
    m_pFS->dZ=dZavr+dVZ*dDeltaT;
    if (N>6) {
        qDebug() << QString("Bind (%1,%2,%3) V (%4,%5,%6)")
                    .arg(m_pFS->dX).arg(m_pFS->dY).arg(m_pFS->dZ)
                    .arg(m_pFS->dVX).arg(m_pFS->dVY).arg(m_pFS->dVZ);
        m_bTrackingOn=true;
    }
    qDebug() << QString("Appending pt to cluster: N=%1 R(%2,%3,%4) V(%5,%6,%7)")
                .arg(N).arg(m_pFS->dX).arg(m_pFS->dY).arg(m_pFS->dZ)
                .arg(m_pFS->dVX).arg(m_pFS->dVY).arg(m_pFS->dVZ);
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::isInsideSpaceStrobe(const struct sPrimaryPt *pPriPt) {
    if (!m_pFS) {
        m_pFS = new sFilterState(pPriPt);
        return true;
    }
    double dDeltaR = calcProximity(pPriPt);
    double dDeltaV = 0.0e0;
    double dVn;
    if (m_qlCluster.size()>1) {
        double xx=m_pFS->dX; xx=xx*xx;
        double yy=m_pFS->dY; yy=yy*yy;
        double zz=m_pFS->dZ; zz=zz*zz;
        double rr=sqrt(xx+yy+zz);
        if (rr<dEPS) return false;
        double dnX = m_pFS->dX/rr;
        double dnY = m_pFS->dY/rr;
        double dnZ = m_pFS->dZ/rr;
        dVn = m_pFS->dVX*dnX+m_pFS->dVY*dnY+m_pFS->dVZ*dnZ;
        dDeltaV = remainder(pPriPt->dV_D - dVn, pPriPt->dVDWin);
        dDeltaV = abs(dDeltaV)/pPriPt->dVDWin;
    }
    if (dDeltaR < 100.0 && (dDeltaV < 0.2 || (!m_bTrackingOn && m_qlCluster.size()==2))) {
        return true;
    }
    // qDebug() << "Rejecting space strobe: " << dDeltaR << " Vold " << dVn << " Vnew " << pPriPt->dV_D << " dDeltaV=" << dDeltaV;
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double QSimpleFilter::calcProximity(const QCoreTraceFilter::sPrimaryPt *pPrimaryPoint) const {
    const sPrimaryPt *pp = dynamic_cast<const sPrimaryPt *>(pPrimaryPoint);
    if (!pp || !m_pFS) {
        qDebug() << "QSimpleFilter::calcProximity() cast error";
        throw RmoException("QSimpleFilter::calcProximity() cast error");
    }
    double dDeltaT = pp->dTs - m_pFS->dT;
    double dDeltaX = pp->dX - (m_pFS->dX+m_pFS->dVX*dDeltaT);
    double dXX=dDeltaX*dDeltaX;
    double dDeltaY = pp->dY - (m_pFS->dY+m_pFS->dVY*dDeltaT);
    double dYY=dDeltaY*dDeltaY;
    double dDeltaZ = pp->dZ - (m_pFS->dZ+m_pFS->dVZ*dDeltaT);
    double dZZ=dDeltaZ*dDeltaZ;
    return sqrt(dXX+dYY+dZZ);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::filterStep(const struct sPrimaryPt &pp) {
    if (!m_pFS) return false;
    double dDeltaT = pp.dTs-m_pFS->dT;
    if (dDeltaT < 1.0e-6*dEPS) return false;
    m_pFS->dT=pp.dTs;
    m_pFS->dX+=m_pFS->dVX*dDeltaT;
    m_pFS->dY+=m_pFS->dVY*dDeltaT;
    m_pFS->dZ+=m_pFS->dVZ*dDeltaT;
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
