#ifndef KALEXTENDED_H
#define KALEXTENDED_H

#include "qtracefilter.h"

class KalExtended : public QTraceFilter
{
public:
    KalExtended(const QCoreTraceFilter &other);
    ~KalExtended();
    virtual bool eatPrimaryPoint(int iPtIdx);
    virtual double calcProximity(const QCoreTraceFilter::sPrimaryPt &primaryPoint) const;
    // dummy state
    double m_dR;
};

#endif // KALEXTENDED_H
