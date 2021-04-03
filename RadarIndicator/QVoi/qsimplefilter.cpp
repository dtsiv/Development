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
/* virtual */ QCoreTraceFilter::sFilterState QSimpleFilter::getState(double dTime) {
    QCoreTraceFilter::sFilterState sState;
    double xx, yy, zz, rr;
    double dDeltaT = dTime - m_pFS->dT;
    // distance
    double dX = m_pFS->dX+m_pFS->dVX*dDeltaT;
    double dY = m_pFS->dY+m_pFS->dVY*dDeltaT;
    double dZ = m_pFS->dZ+m_pFS->dVZ*dDeltaT;
    xx = dX*dX; yy = dY*dY; zz = dZ*dZ;
    rr = sqrt(xx+yy+zz);
    sState.qpfDistVD.rx() = rr;
    double dVX = m_pFS->dVX;
    double dVY = m_pFS->dVY;
    double dVZ = m_pFS->dVZ;
    // Doppler velocity
    xx=dX*dVX; yy=dY*dVY; zz=dZ*dVZ;
    sState.qpfDistVD.ry() = (xx+yy+zz)/rr;
    sState.qsName=m_qsFilterName;
    return sState;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ bool QSimpleFilter::isStale(double dTime) {
    return (dTime-m_pFS->dT)>3.0e0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::eatPrimaryPoint(int iPtIdx) {
    sPrimaryPt *pPriPt = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
    if (!pPriPt) {
        throw RmoException("eatPrimaryPoint() dynamic cast failed"); // not valid list index or failed dynamic_cast
    }
    // empty filter always eats first point
    if (!m_pFS) {
        m_pFS = new sFilterState(pPriPt);
        pPriPt->pFlt = this;
        m_qlCluster.append(iPtIdx);
        m_qlPriPts.append(iPtIdx);
        // qDebug() << QString("%1: new cluster #%4 R=%2 VD=%3")
        //                           .arg(pPriPt->dTs).arg(pPriPt->dR).arg(pPriPt->dV_D).arg(m_qsFilterName);
        return true;
    }
    // from now on we can guarantee that pPriPt has type <QTraceFilter::sPrimaryPt*>
    if (!isInsideSpaceStrobe(pPriPt)) { // filter rejects this pt
        if (m_bTrackingOn) {
            // qDebug() << QString("%1: active filter #%4 rejected R=%2 VD=%3")
            //                           .arg(pPriPt->dTs).arg(pPriPt->dR).arg(pPriPt->dV_D).arg(m_qsFilterName);
        }
        else {
            // qDebug() << QString("%1: cluster #%4 rejected R=%2 VD=%3")
            //                            .arg(pPriPt->dTs).arg(pPriPt->dR).arg(pPriPt->dV_D).arg(m_qsFilterName);
        }
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
            // qDebug() << "QSimpleFilter::eatPrimaryPoint() filterStep failed";
            return false;
        }
    }
    // ... else append primary point to cluster
    if (linearRegression(pPriPt)) { // primary point accepted
        pPriPt->pFlt = this;
        m_qlCluster.append(iPtIdx);
        m_qlPriPts.append(iPtIdx);
        // qDebug() << "Pt appended to cluster";
        return true;
    }
    if (m_bTrackingOn) {
        // qDebug() << QString("%1: active filter #%4, linear regression failed R=%2 VD=%3")
        //                            .arg(pPriPt->dTs).arg(pPriPt->dR).arg(pPriPt->dV_D).arg(m_qsFilterName);
    }
    else {
        // qDebug() << QString("%1: cluster #%4, linear regression failed R=%2 VD=%3")
        //                            .arg(pPriPt->dTs).arg(pPriPt->dR).arg(pPriPt->dV_D).arg(m_qsFilterName);
    }
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::linearRegression(const struct sPrimaryPt *pPriPt) {
    // check existing cluster
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
        }
        sPrimaryPt *pp = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
        if (!pp) {
            throw RmoException("QSimpleFilter::linearRegression: dynamic_cast failed");
        }
        qlT<<pp->dTs; qlX<<pp->dX; qlY<<pp->dY; qlZ<<pp->dZ;
        dV_D = pp->dV_D;
        dVDWin = pp->dVDWin;
    }
    // cannot add point with same time dTs
    if (abs(pPriPt->dTs-qlT.last())<1.0e-6*dEPS) {
        // qDebug() << "Rejecting pt with same time " << pPriPt->dTs;
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
    }
    // linear regression coefficients
    double dVX=dXT/dTsigma2; double dVY=dYT/dTsigma2; double dVZ=dZT/dTsigma2;
    // velocity direction unit vector
    double dVX2=dVX*dVX; double dVY2=dVY*dVY; double dVZ2=dVZ*dVZ;
    double dV=sqrt(dVX2+dVY2+dVZ2);
    if (dV<dEPS) {
        // qDebug() << QString("%1: accepting pt with no motion").arg(pPriPt->dTs);
        return true; // accept targets with no motion
    }
    double dnVX=dVX/dV; double dnVY=dVY/dV; double dnVZ=dVZ/dV;
    // correlation coefficient
    QList<double> dRp;
    if (N>2) {
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
        if (abs(dCorrCoef)<m_qmCorrThresh.value(N,2.0e0)) {
            // qDebug() << QString("%1: cluster #%2 Rejecting CorrCoef %3")
            //          .arg(pPriPt->dTs).arg(m_qsFilterName).arg(dCorrCoef);
            return false;
        }
    }
    m_pFS->dT=pPriPt->dTs; m_pFS->dVX=dVX; m_pFS->dVY=dVY; m_pFS->dVZ=dVZ;
    double dDeltaT=pPriPt->dTs-dTavr;
    m_pFS->dX=dXavr+dVX*dDeltaT;
    m_pFS->dY=dYavr+dVY*dDeltaT;
    m_pFS->dZ=dZavr+dVZ*dDeltaT;
    if (N>=m_iMaxClusterSz) {
//####################### DEBUGGING ################################
#if 0
        QFile qfOut("lr.dat");
        bool bOut=true;
        if (qfOut.exists()) bOut=false;
        qfOut.open(QIODevice::WriteOnly);
        QTextStream tsOut(&qfOut);
        if (bOut) {
            for (int i=0; i<N; i++) {
                tsOut << qlT.at(i)
                      << "\t" << qlX.at(i)
                      << "\t" << qlY.at(i)
                      << "\t" << qlZ.at(i)
                      << "\t" << dRp.at(i)
                      << endl;
            }
        }
#endif
//##################################################################
        // qDebug() << QString("%1: cluster #%2 Bind R=%3 VD=%4")
        //             .arg(m_pFS->dT).arg(m_qsFilterName).arg(pPriPt->dR).arg(pPriPt->dV_D);
        m_bTrackingOn=true;
    }
    // qDebug() << QString("%4: cluster #%5 newpt N=%1 R(%2) V_D(%3)")
    //            .arg(N).arg(pPriPt->dR)
    //            .arg(pPriPt->dV_D)
    //            .arg(pPriPt->dTs).arg(m_qsFilterName);
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::isInsideSpaceStrobe(const struct sPrimaryPt *pPriPt) {
    if (!m_pFS || m_qlCluster.isEmpty()) { // function called before first primary point - error
        throw RmoException("QSimpleFilter::isInsideSpaceStrobe() early call!");
    }
    double dDeltaR = calcProximity(pPriPt);
    // cut distant points immediately
    if (dDeltaR>100.0e0) {
        if (m_bTrackingOn) {
//            qDebug()
//                << QString("%1: active filter #%5 rejected pt @ %4 meters away R=%2 VD=%3")
//                   .arg(pPriPt->dTs).arg(pPriPt->dR)
//                   .arg(pPriPt->dV_D)
//                   .arg(dDeltaR).arg(m_qsFilterName);
        }
        else {
//            qDebug()
//                << QString("%1: cluster #%5 rejected pt @ %4 meters away R=%2 VD=%3")
//                   .arg(pPriPt->dTs).arg(pPriPt->dR)
//                   .arg(pPriPt->dV_D)
//                   .arg(dDeltaR).arg(m_qsFilterName);
        }
        return false;
    }
    // if no trajectory was started ...
    if (!m_bTrackingOn) {
        // first compare Doppler velocity with the last point in cluster
        int iLast = m_qlCluster.last();
        sPrimaryPt *pp = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iLast,pPriPtNULL));
        if (!pp) {
            throw RmoException("QSimpleFilter::linearRegression: dynamic_cast failed");
            return false;
        }
        // compare Doppler velocity with the most recent point in cluster (if Doppler windows match!)
        if (abs(pp->dVDWin-pPriPt->dVDWin) < dEPS) {
            double dV_Dlast = pp->dV_D;
            double dDeltaV = remainder(pPriPt->dV_D - dV_Dlast, pPriPt->dVDWin);
            dDeltaV = abs(dDeltaV)/pPriPt->dVDWin;
            if (dDeltaV < 0.1) {
//                if (m_qlCluster.size()==2) qDebug() << QString("%1: cluster #%4 of two pts @ dist %2, V_D=%3")
//                                   .arg(pPriPt->dTs)
//                                   .arg(pPriPt->dR)
//                                   .arg(pPriPt->dV_D).arg(m_qsFilterName);
                return true;
            }
        }
        // no need to proceed, if cluster has only one point
        if (m_qlCluster.size()==1) return false;
    }

    // DEBUG DEBUG !!!
    int iLast = m_qlPriPts.last();
    sPrimaryPt *pp = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iLast,pPriPtNULL));
    if (!pp) {
        throw RmoException("QSimpleFilter::linearRegression: dynamic_cast failed");
        return false;
    }
    if (abs(pPriPt->dVDWin - pp->dVDWin) < dEPS) {
        double dDeltaV = remainder(pPriPt->dV_D - pp->dV_D, pPriPt->dVDWin);
        dDeltaV = abs(dDeltaV)/pPriPt->dVDWin;
        if (dDeltaV < 0.1) {
            return true;
        }
//        qDebug() << QString("%1: #%3 skipping dDeltaV=%2").arg(pPriPt->dTs).arg(dDeltaV).arg(m_qsFilterName);
    }
    // compare Doppler velocity with the filter state
    double xx=m_pFS->dX;
    double yy=m_pFS->dY;
    double zz=m_pFS->dZ;
    double rr=xx*xx+yy*yy+zz*zz;
    rr = sqrt(rr);
    if (rr<dEPS) return false; // filter state must make sense!
    // calculate radial velocity for filter state
    double dVr = m_pFS->dVX*xx+m_pFS->dVY*yy+m_pFS->dVZ*zz;
    dVr = dVr / rr;
    double dDeltaV = remainder(pPriPt->dV_D - dVr, pPriPt->dVDWin);
    dDeltaV = abs(dDeltaV)/pPriPt->dVDWin;
    if (dDeltaV < 0.1) {
        return true;
    }
    if (m_bTrackingOn) {
//        qDebug() << QString("%1: Traj #%6 rejects dDeltaV=%2 (%4 - %3 %% %5)")
//                    .arg(pPriPt->dTs)
//                    .arg(dDeltaV)
//                    .arg(dVr)
//                    .arg(pPriPt->dV_D)
//                    .arg(pPriPt->dVDWin).arg(m_qsFilterName);
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
