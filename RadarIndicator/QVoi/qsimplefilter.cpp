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
/* virtual */ QCoreTraceFilter::sFilterState QSimpleFilter::getState(double dTime, bool bGetCurrentState /*= false*/) const {
    QCoreTraceFilter::sFilterState sState;
    double dDeltaT = dTime - m_pFS->dT;
    if (bGetCurrentState) dDeltaT=0.0e0;
    double dDeltaT2= dDeltaT*dDeltaT;
    // distance
    double dX = m_pFS->dX+m_pFS->dVX*dDeltaT+0.5e0*m_pFS->dAX*dDeltaT2;
    double dY = m_pFS->dY+m_pFS->dVY*dDeltaT+0.5e0*m_pFS->dAY*dDeltaT2;
    double dZ = m_pFS->dZ+m_pFS->dVZ*dDeltaT+0.5e0*m_pFS->dAZ*dDeltaT2;
    double xx, yy, zz, rr;
    xx = dX*dX; yy = dY*dY; zz = dZ*dZ;
    rr = sqrt(xx+yy+zz);
    sState.qpfDistVD.rx() = rr;
    double dVX = m_pFS->dVX+m_pFS->dAX*dDeltaT;
    double dVY = m_pFS->dVY+m_pFS->dAY*dDeltaT;
    double dVZ = m_pFS->dVZ+m_pFS->dAZ*dDeltaT;
    // Doppler velocity
    xx=dX*dVX; yy=dY*dVY; zz=dZ*dVZ;
    sState.qpfDistVD.ry() = (xx+yy+zz)/rr;
    if (sState.qpfDistVD.ry()>550 && m_uFilterId==8) {
        qDebug() << m_qsFilterName
                 << " getState(" << dTime << ", " << bGetCurrentState << ")"
                 << " FS_VD = " << sState.qpfDistVD.ry();
    }
    sState.qsName=m_qsFilterName;
    return sState;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* virtual */ bool QSimpleFilter::isStale(double dTime) {
    if (!m_bTrackingOn && m_qlCluster.size()>m_iMaxClusterSz) {
        return true;
    }
    return (dTime-m_pFS->dT)>m_dStaleTimeout;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::eatPrimaryPoint(int iPtIdx) {
    sPrimaryPt *pPriPt = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
    if (!pPriPt) {     // from now on we can guarantee that pPriPt has type <QTraceFilter::sPrimaryPt*>
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

    // Reject big clusters
    if (!m_bTrackingOn && m_qlCluster.size()>m_iMaxClusterSz) {
        return false;
    }

    // Reject distant points
    if (!isInsideSpaceStrobe(pPriPt)) { // filter rejects this pt
        return false;
    }

    if (m_uFilterId == 8) {
        QFile qfCartCoord("cartcoord.dat");
        if (m_qlPriPts.size()<2) qfCartCoord.resize(0);
        qfCartCoord.open(QIODevice::Append);
        QTextStream tsCartCoord(&qfCartCoord);
        tsCartCoord
                << pPriPt->dTs
                << "\t" << pPriPt->dX
                << "\t" << pPriPt->dY
                << "\t" << pPriPt->dZ
                << "\t" << pPriPt->dV_D
                << endl;
        qfCartCoord.close();
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
        // qDebug() << "Pt appended to cluster: #" << m_qlCluster.size();
        return true;
    }

    // linear regression failed - cannot eat the point
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
        // reject low correlation
        const double dImpossibleCorrelationValue = 2.0e0;
        double dThreshold = m_qmCorrThresh.value(N,dImpossibleCorrelationValue);
        if (abs(dCorrCoef) < dThreshold) {
            // qDebug() << QString("%1: cluster #%2 Rejecting CorrCoef %3")
            //          .arg(pPriPt->dTs).arg(m_qsFilterName).arg(dCorrCoef);
            return false;
        }
        // DEBUG output of correlation coefficient
        // qDebug() << m_qsFilterName << ": N=" << N << "; r=" << dCorrCoef << "; Thresh=" << dThreshold;
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
    if (!m_pFS || m_qlPriPts.isEmpty()) { // function called before first primary point - error
        throw RmoException("QSimpleFilter::isInsideSpaceStrobe() early call!");
    }

    // cut distant points immediately
    double dDeltaT = pPriPt->dTs-m_pFS->dT;
    if (dDeltaT < dEPS) return false;
    double dDeltaR = calcProximity(pPriPt);
    double dCutOff = m_dSpaceStrobeCutoff + dDeltaT*m_dStrobeGrowthVelocity;
    if ((!m_bTrackingOn && (dDeltaR > dCutOff))
        ||m_bTrackingOn && (dDeltaR > 50.0e0)) {
        // qDebug() << "Rejecting space strobe: DelR=" << dDeltaR;
        return false;
    }

    // first compare Doppler velocity with the last point in cluster
    int iLast = m_qlPriPts.last();
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
        if ( (!m_bTrackingOn && (dDeltaV < m_dVD_MaxRelOffset))
           || (m_bTrackingOn && (dDeltaV < 0.03e0))) {
            return true;
        }
    }
    // no need to proceed, if cluster has only one point
    if (m_qlCluster.size()==1) return false;
    // compare Doppler velocity with the filter state
    // calculate radial velocity for filter state
    double dVX = m_pFS->dVX+m_pFS->dAX*dDeltaT;
    double dVY = m_pFS->dVY+m_pFS->dAY*dDeltaT;
    double dVZ = m_pFS->dVZ+m_pFS->dAZ*dDeltaT;
    double dDeltaT2 = dDeltaT*dDeltaT;
    double xx=m_pFS->dX+m_pFS->dVX*dDeltaT+0.5e0*m_pFS->dAX*dDeltaT2;
    double yy=m_pFS->dY+m_pFS->dVY*dDeltaT+0.5e0*m_pFS->dAY*dDeltaT2;
    double zz=m_pFS->dZ+m_pFS->dVZ*dDeltaT+0.5e0*m_pFS->dAZ*dDeltaT2;
    double rr=xx*xx+yy*yy+zz*zz;
    rr = sqrt(rr);
    if (rr<dEPS) return false; // filter state must make sense!
    double dVr = dVX*xx+dVY*yy+dVZ*zz;
    dVr = dVr / rr;
    double dDeltaV = remainder(pPriPt->dV_D - dVr, pPriPt->dVDWin);
    dDeltaV = abs(dDeltaV)/pPriPt->dVDWin;
    if (dDeltaV < m_dVD_MaxRelOffset) {
        return true;
    }
    else {
        // qDebug() << "Rejecting space strobe: DelR=" << dDeltaR << " dDeltaV=" << dDeltaV;
    }

//    if (m_uFilterId==8) {
//        double dV_Dlast = pp->dV_D;
//        qDebug() << m_qsFilterName << " rej (" << pPriPt->dR << ", " << pPriPt->dV_D << "). dV_Dlast=" << dV_Dlast;
//    }
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
    double dDeltaT2= dDeltaT*dDeltaT;
    double dDeltaX = pp->dX - (m_pFS->dX+m_pFS->dVX*dDeltaT+0.5e0*m_pFS->dAX*dDeltaT2);
    double dXX=dDeltaX*dDeltaX;
    double dDeltaY = pp->dY - (m_pFS->dY+m_pFS->dVY*dDeltaT+0.5e0*m_pFS->dAY*dDeltaT2);
    double dYY=dDeltaY*dDeltaY;
    double dDeltaZ = pp->dZ - (m_pFS->dZ+m_pFS->dVZ*dDeltaT+0.5e0*m_pFS->dAZ*dDeltaT2);
    double dZZ=dDeltaZ*dDeltaZ;
    return sqrt(dXX+dYY+dZZ);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSimpleFilter::filterStep(const struct sPrimaryPt &sPriPt) {
    // Two-sided significance       50%     60%     70%     80%     90%     95%     98%     99%     99.5%	99.8%	99.9%
    // degrees of freedom N-2=1    	1.000	1.376	1.963	3.078	6.314	12.71	31.82	63.66	127.3	318.3	636.6
    if (!m_pFS) return false;
    //    double dDeltaT = sPriPt.dTs-m_pFS->dT;
    //    double dDeltaT2= dDeltaT*dDeltaT;
    //    if (dDeltaT < 1.0e-6*dEPS) return false;
    //    m_pFS->dT=sPriPt.dTs;
    //    double dXextr=m_pFS->dX+m_pFS->dVX*dDeltaT+0.5e0*m_pFS->dAX*dDeltaT2;
    //    double dYextr=m_pFS->dY+m_pFS->dVY*dDeltaT+0.5e0*m_pFS->dAY*dDeltaT2;
    //    double dZextr=m_pFS->dZ+m_pFS->dVZ*dDeltaT+0.5e0*m_pFS->dAZ*dDeltaT2;
    //    m_pFS->dX=dXextr*(1-m_dFilterAlpha)+sPriPt.dX*m_dFilterAlpha;
    //    m_pFS->dY=dYextr*(1-m_dFilterAlpha)+sPriPt.dY*m_dFilterAlpha;
    //    m_pFS->dZ=dZextr*(1-m_dFilterAlpha)+sPriPt.dZ*m_dFilterAlpha;

    double dDeltaT = sPriPt.dTs-m_pFS->dT;
    if (dDeltaT < 1.0e-6*dEPS) return false;
    // unconditional update
    m_pFS->dT=sPriPt.dTs;
    m_pFS->dX=sPriPt.dX;
    m_pFS->dY=sPriPt.dY;
    m_pFS->dZ=sPriPt.dZ;

    // try to update velocity
    quint32 N = m_iMaxClusterSz - 1;
    if (m_qlPriPts.size() < N) { // initial cluster=m_iMaxClusterSz
        throw RmoException("QSimpleFilter::filterStep: m_qlPriPts.size() < m_iMaxClusterSz");
        return false;
    }
    QList<int> qlIdxPriPt;
    QList<double> qlT,qlX,qlY,qlZ;
    // loop from size()-N,...,size()-1 (total=N elements)
    for (int i=m_qlPriPts.size()-N; i<m_qlPriPts.size(); i++) {
        int iPtIdx = m_qlPriPts.value(i,-1);
        if (iPtIdx<0) {
            throw RmoException("QSimpleFilter::linearRegression: iPtIdx>=m_qlAllPriPts.size()");
        }
        qlIdxPriPt<<iPtIdx;
        sPrimaryPt *pp = dynamic_cast<struct QSimpleFilter::sPrimaryPt*>(m_qlAllPriPts.value(iPtIdx,pPriPtNULL));
        if (!pp) {
            throw RmoException("QSimpleFilter::linearRegression: dynamic_cast failed");
        }
        qlT<<pp->dTs; qlX<<pp->dX; qlY<<pp->dY; qlZ<<pp->dZ;
    }
    // append sPriPt to the lists AND increment N
    qlT<<sPriPt.dTs; qlX<<sPriPt.dX; qlY<<sPriPt.dY; qlZ<<sPriPt.dZ; N++;
    // calculate averages
    double dTavr=0.0e0,dTsigma2=0.0e0;
    double dXavr=0.0e0,dXT=0.0e0;
    double dYavr=0.0e0,dYT=0.0e0;
    double dZavr=0.0e0,dZT=0.0e0;
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
    if (dV>dEPS) {
        double dnVX=dVX/dV; double dnVY=dVY/dV; double dnVZ=dVZ/dV;
        // correlation coefficient
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
        const double dImpossibleCorrelationValue = 2.0e0;
        if (abs(dCorrCoef)>m_qmCorrThresh.value(N,dImpossibleCorrelationValue)) {
            //            double xx=m_pFS->dX;
            //            double yy=m_pFS->dY;
            //            double zz=m_pFS->dZ;
            //            double rr=sqrt(xx*xx+yy*yy+zz*zz);
            //            xx=xx/rr; yy=yy/rr; zz=zz/rr;
            //                qDebug() << QString("fs:%1 V_D=%2 (%9 %10 %11): pos(%3)=%4 pos(%5)=%6 pos(%7)=%8")
            //                            .arg(sPriPt.dTs)
            //                            .arg(dVX*xx+dVY*yy+dVZ*zz)
            //                            .arg(qlT.at(0)).arg(dRp.at(0))
            //                            .arg(qlT.at(1)).arg(dRp.at(1))
            //                            .arg(qlT.at(2)).arg(dRp.at(2))
            //                            .arg(qlIdxPriPt.at(0)).arg(qlIdxPriPt.at(1))
            //                            .arg(m_qlAllPriPts.size()-1)
            //                            ;
            //            m_pFS->dVX=m_pFS->dVX*(1-m_dFilterAlpha)+dVX*m_dFilterAlpha;
            //            m_pFS->dVY=m_pFS->dVY*(1-m_dFilterAlpha)+dVY*m_dFilterAlpha;
            //            m_pFS->dVZ=m_pFS->dVZ*(1-m_dFilterAlpha)+dVZ*m_dFilterAlpha;
            //                qDebug() << QString("Est6(%1)=%2 X=%3")
            //                            .arg(qlT.at(0),0,'f',2)
            //                            .arg(dVX,0,'f',2)
            //                            .arg(qlX.at(0),0,'f',2)
            //                            ;
            m_pFS->dVX=dVX;
            m_pFS->dVY=dVY;
            m_pFS->dVZ=dVZ;
            // radial direction
            double xx=m_pFS->dX;
            double yy=m_pFS->dY;
            double zz=m_pFS->dZ;
            double rr=sqrt(xx*xx+yy*yy+zz*zz);
            xx=xx/rr; yy=yy/rr; zz=zz/rr;
            // update radial component of velocity
            double dVradial = xx*dVX+yy*dVY+zz*dVZ;
            double dVradialDiff = sPriPt.dV_D-dVradial;
            dVradialDiff = remainder(dVradialDiff, sPriPt.dVDWin);

            if (abs(dVradialDiff/sPriPt.dVDWin) < m_dVD_MaxRelOffset) {
                m_pFS->dVX += xx*dVradialDiff;
                m_pFS->dVY += yy*dVradialDiff;
                m_pFS->dVZ += zz*dVradialDiff;
//                qDebug() << m_qsFilterName
//                         << " Ts=" << sPriPt.dTs
//                         << " reg=" << dVradial
//                         << " dif=" << dVradialDiff
//                         << " sum=" << dVradial+dVradialDiff
//                         << " r=" << dCorrCoef;
            }
            else {
                                qDebug() << m_qsFilterName << " noRadialCorr! "
                                         << " Ts=" << sPriPt.dTs
                                         << " reg=" << dVradial
                                         << " dif=" << dVradialDiff
                                         << " sum=" << dVradial+dVradialDiff
                                         << " r=" << dCorrCoef;
            }
            return true;
        }
    }
    //    m_pFS->dVX+=m_pFS->dAX*dDeltaT;
    //    m_pFS->dVY+=m_pFS->dAY*dDeltaT;
    //    m_pFS->dVZ+=m_pFS->dAZ*dDeltaT;
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
