#include "qcoretracefilter.h"
#include "qtracefilter.h"
#include "kalextended.h"
#include "qsimplefilter.h"

bool QCoreTraceFilter::bInit=false;
QList<QTraceFilter *> QCoreTraceFilter::m_qlFilters;
QList<struct QCoreTraceFilter::sPrimaryPt *> QCoreTraceFilter::m_qlAllPriPts;

//========================================================================
//
//========================================================================
QCoreTraceFilter::sPrimaryPt::sPrimaryPt()
        : pFlt(NULL) {
}
//========================================================================
//
//========================================================================
QCoreTraceFilter::sPrimaryPt::sPrimaryPt(const sPrimaryPt &other)
        : pFlt(NULL) {
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
QCoreTraceFilter::QCoreTraceFilter(const QCoreTraceFilter &other) {
    m_dMeasNoise=other.m_dMeasNoise;
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
    T::sPrimaryPt *pp=new T::sPrimaryPt(primaryPoint);
    m_qlAllPriPts.append(pp);
    // get primary point proximity to each filter state
    if (m_qlFilters.size()) {
        QList<int> qlFltIndxsLocal;
        QList<double> qlDist;
        for (int i=0; i<m_qlFilters.size(); i++) {
            qlFltIndxsLocal<<i;
            qlDist<<m_qlFilters.at(i)->calcProximity(pp);
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
        // eat the primary point starting from closest filter
        QListIterator<int> i(qlFltIndxsLocal);
        while (i.hasNext()) {
            if (m_qlFilters.at(i.next())->eatPrimaryPoint(iPtIdx)) return iPtIdx;
        }
    }
    // If neither filter accepts it - create new (empty) filter for this primary point
    T *pNewFilter = new T(*this);
    m_qlFilters.append(pNewFilter);
    pNewFilter->eatPrimaryPoint(iPtIdx); // the empty filter accepts it for sure
    return iPtIdx;
}
//========================================================================
//
//========================================================================
void QCoreTraceFilter::cleanup() {
    while (m_qlFilters.count()) delete m_qlFilters.takeLast();
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
