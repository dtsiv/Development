#ifndef QTRACEFILTER_H
#define QTRACEFILTER_H

#include "qcoretracefilter.h"
// #include <math.h>  // already in <QtCore>
#include "cmatrix"
typedef techsoft::matrix<double> Matrix;

const double dEPS=1.0e-8;
class QTraceFilter : public QCoreTraceFilter {
public:
    struct sPrimaryPt : public QCoreTraceFilter::sPrimaryPt {
        sPrimaryPt(QCoreTraceFilter::sPrimaryPt &primaryPointOther);
        virtual ~sPrimaryPt(){}
    };
    virtual double calcProximity(const QCoreTraceFilter::sPrimaryPt *pPrimaryPoint) const = 0;

protected:
    QList <int>m_qlPriPts; // indices within m_qlAllPriPts, which are related to this QTraceFilter
    QList <int>m_qlCluster;// cluster for trajectory binding
    struct QCoreTraceFilter::sPrimaryPt * const & pPriPtNULL;

public:
    QTraceFilter(const QCoreTraceFilter &other);
    // virtual destructor ensures that descendant's destructor is called
    virtual ~QTraceFilter();
    void inversion();
    void spearman();

    // void ttest();
    double twoSidedStudentSignificance(double dThreshold, int iDegreesOfFreedomN);
    virtual bool eatPrimaryPoint(int iPtIdx) = 0;
    virtual QCoreTraceFilter::sFilterState getState(double dTime, bool bGetCurrentState = false) const;
    virtual bool isStale(double dTime);
    virtual bool isTrackingOn() { return m_bTrackingOn; }
    virtual quint32 filterId() { return m_uFilterId; }

protected:
    Matrix m_mH;
    bool m_bTrackingOn;
    QString m_qsFilterName;
    quint32 m_uFilterId;

public:
    // virtual void print();
private:
    static int m_iNextFilterNumber;
};

#endif // QTRACEFILTER_H
