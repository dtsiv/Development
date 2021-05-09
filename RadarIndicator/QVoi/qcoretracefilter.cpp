#include "qcoretracefilter.h"
#include "qtracefilter.h"
#include "kalextended.h"
#include "qsimplefilter.h"
#include "qexceptiondialog.h"

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "nr.h"
using namespace std;

bool QCoreTraceFilter::bInit=false;
QMap<quint32,QTraceFilter *> QCoreTraceFilter::m_qmFilters;
QList<struct QCoreTraceFilter::sPrimaryPt *> QCoreTraceFilter::m_qlAllPriPts;
double QCoreTraceFilter::m_dCorrSignif = 0.8e0;
QMap<int,double> QCoreTraceFilter::m_qmCorrThresh;
double QCoreTraceFilter::m_dMeasNoise = -1.0e0;
int QCoreTraceFilter::m_iMaxClusterSz = 6;
double QCoreTraceFilter::m_dStaleTimeout = 3.0e0;
double QCoreTraceFilter::m_dSpaceStrobeCutoff = 300.0e0;
double QCoreTraceFilter::m_dVD_MaxRelOffset = 0.1e0;
double QCoreTraceFilter::m_dStrobeGrowthVelocity = 0.0e0;
double QCoreTraceFilter::m_dFilterAlpha = 0.2e0;

//========================================================================
//
//========================================================================
QCoreTraceFilter::sPrimaryPt::sPrimaryPt()
        : pFlt(NULL)
        , uFilterIndex(0) {
}
//========================================================================
//
//========================================================================
QCoreTraceFilter::sPrimaryPt::sPrimaryPt(const sPrimaryPt &other)
        : pFlt(NULL)
        , uFilterIndex(0) {
    dR     = other.dR;
    dElRad = other.dElRad;
    dAzRad = other.dAzRad;
    dV_D   = other.dV_D;
    dTs    = other.dTs;
    dVDWin = other.dVDWin;
}
//========================================================================
//
//========================================================================
QCoreTraceFilter::QCoreTraceFilter([[maybe_unused]]const QCoreTraceFilter &other) {
}
//========================================================================
//
//========================================================================
QCoreTraceFilter& QCoreTraceFilter::getInstance() {
    if (!bInit) {
        bInit=true;
        // perform init
    }
    static QCoreTraceFilter instance;
    return instance;
}
//========================================================================
//
//========================================================================
int QCoreTraceFilter::addPrimaryPoint(struct sPrimaryPt &primaryPoint) {
    // select a specific type of primary points
    if (true /* SIMPLE FILTER SELECTED*/) {
        return addPrimaryPoint<QSimpleFilter>(primaryPoint);
    }
    else {
        return addPrimaryPoint<KalExtended>(primaryPoint);
    }
    return -1; // unreachable
}
//========================================================================
//
//========================================================================
template <typename T>
int QCoreTraceFilter::addPrimaryPoint(struct sPrimaryPt &primaryPoint) {
    // immediately add to list and obtain the return index iPtIdx
    int iPtIdx = m_qlAllPriPts.size();
    double dTs=primaryPoint.dTs;
    T::sPrimaryPt *pp=new T::sPrimaryPt(primaryPoint);
    m_qlAllPriPts.append(pp);
    // get primary point proximity to each filter state
    if (m_qmFilters.size()) {
        QList<int> qlFltIndxsLocal;
        QList<double> qlDist;
        QList<quint32> qlKeys=m_qmFilters.keys();
        for (int i=0; i<qlKeys.size(); i++) {
            quint32 uFilterIndex=qlKeys.at(i);
            QTraceFilter *pFilter=m_qmFilters.value(uFilterIndex,NULL);
            // stale filters are always omitted
            if (!pFilter || pFilter->isStale(dTs)) continue;
            qlFltIndxsLocal<<uFilterIndex;
            qlDist<<pFilter->calcProximity(pp);
        }
        // sort qlDist ascending together with qlFltIndxsLocal
        for (int i=0; i<qlFltIndxsLocal.size()-1; i++) {
            for (int j=i+1; j<qlFltIndxsLocal.size(); j++) {
                if (qlDist.at(i)>qlDist.at(j)) {
                    qlDist.swap(i,j);
                    qlFltIndxsLocal.swap(i,j);
                }
            }
        }
        // eat the primary point starting from closest active filter
        QListIterator<int> i(qlFltIndxsLocal);
        while (i.hasNext()) {
            quint32 uFilterIndex=i.next();
            QTraceFilter *pFilter=m_qmFilters.value(uFilterIndex,NULL);
            if (pFilter && pFilter->isTrackingOn()) {
                if (pFilter->eatPrimaryPoint(iPtIdx)) {
                    primaryPoint.uFilterIndex=pp->uFilterIndex=uFilterIndex;
                    return iPtIdx;
                }
            }
        }
        i.toFront();
        // eat the primary point starting from closest cluster
        while (i.hasNext()) {
            quint32 uFilterIndex=i.next();
            QTraceFilter *pFilter=m_qmFilters.value(uFilterIndex,NULL);
            if (pFilter && !pFilter->isTrackingOn()) {
                if (pFilter->eatPrimaryPoint(iPtIdx)) {
                    primaryPoint.uFilterIndex=pp->uFilterIndex=uFilterIndex;
                    return iPtIdx;
                }
            }
        }
    }
    // If neither filter accepts it - create new (empty) filter for this primary point
    T *pNewFilter = new T(*this);
    quint32 uFilterIndex=pNewFilter->filterId();
    m_qmFilters.insert(uFilterIndex,pNewFilter);
    pNewFilter->eatPrimaryPoint(iPtIdx); // the empty filter accepts it for sure
    primaryPoint.uFilterIndex=pp->uFilterIndex=uFilterIndex;
    return iPtIdx;
}
//========================================================================
//
//========================================================================
void QCoreTraceFilter::cleanup() {
    while (m_qmFilters.count()) delete m_qmFilters.take(m_qmFilters.lastKey());
}
//========================================================================
//
//========================================================================
//void QCoreTraceFilter::printAll() {
//    QListIterator<QTraceFilter*> i(m_qlFilters);
//    while (i.hasNext()) {
//        qDebug() << "printing...";
//        i.next()->print();
//    }
//}
//========================================================================
//
//========================================================================
double QCoreTraceFilter::twoSidedStudentProbability(double dThreshold, int iDegreesOfFreedomN) {
    // this is probability that Student's t_N is within interval [-dThreshold,+dThreshold]
    // Note: rs*sqrt[(N-2)/(1-rs)/(1+rs)] has Student's distribution t_{N-2}
    return 1.0e0-NR::betai(0.5*iDegreesOfFreedomN,0.5,
                     iDegreesOfFreedomN/(iDegreesOfFreedomN+dThreshold*dThreshold));
}
//========================================================================
//
//========================================================================
void QCoreTraceFilter::initCorrThresh() {
    int iTotIter=0, iTotIterMax=1.0e3;
    double dAccuracy=0.01;
    if (m_iMaxClusterSz < 3 || m_iMaxClusterSz > 100 || m_dCorrSignif <= 0.5e0 || m_dCorrSignif >= 1.0e0) {
        qDebug() << "QCoreTraceFilter::initCorrThresh(): bad initialization";
        return;
    }
    // qDebug() << "Student's prob = " << m_dCorrSignif;
    // for (int i=3; i<=m_iMaxClusterSz; i++) {
    for (int i=3; i<=m_iMaxClusterSz; i++) {
        // Two-sided cumulative Student's distribution thresholds
        double dThresholdMin=0.67e0;
        double dThresholdMax=dThresholdMin;
        double dThreshold=dThresholdMin;
        int iDF=i-2;
        double dP1;
        while((dP1=twoSidedStudentProbability(dThresholdMax, iDF))<=m_dCorrSignif) {
                    //            qDebug() << "searching max: "
                    //                     << " dP1=" << dP1
                    //                     << " iDF=" << iDF
                    //                     << " dThresholdMax=" << dThresholdMax
                    //                     << " m_dCorrSignif=" << m_dCorrSignif;
            dThresholdMax*=2.0e0;
            if (++iTotIter>iTotIterMax) {
                qDebug() << "QCoreTraceFilter::initCorrThresh(): bad dThreshold initialization #1";
                return;
            }
        }
        // P_Student(dThresholdMin) < m_dCorrSignif;  P_Student(dThresholdMax) > m_dCorrSignif
        while(dThresholdMax-dThresholdMin>dAccuracy) {
            if (++iTotIter>iTotIterMax) {
                qDebug() << "QCoreTraceFilter::initCorrThresh(): bad dThreshold initialization #2";
                return;
            }
            dThreshold=(dThresholdMax+dThresholdMin)*0.5;
            double dProb=twoSidedStudentProbability(dThreshold, iDF);
            if (dProb>m_dCorrSignif) {
                dThresholdMax=dThreshold;
            }
            else {
                dThresholdMin=dThreshold;
            }
        }
        double dCorrThresh=iDF+dThreshold*dThreshold; dCorrThresh=dThreshold/sqrt(dCorrThresh);
        m_qmCorrThresh.insert(i,dCorrThresh);
        //  qDebug() << "Student's N=" << iDF << " threshStudent=" << dThreshold << " dCorrThresh=" << dCorrThresh;
    }
}
