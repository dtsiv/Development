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
    virtual double calcProximity(const QCoreTraceFilter::sPrimaryPt &primaryPoint) const = 0;

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

protected:
    Matrix m_mH;
    bool m_bTrackingOn;

public:
    // virtual void print();
};

#endif // QTRACEFILTER_H
