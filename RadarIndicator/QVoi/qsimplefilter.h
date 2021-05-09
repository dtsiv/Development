#ifndef QSIMPLEFILTER_H
#define QSIMPLEFILTER_H

#include "qtracefilter.h"

class QSimpleFilter : public QTraceFilter {
public:
    struct sPrimaryPt : public QTraceFilter::sPrimaryPt {
        double dX;       // EUN-coordinate, meters
        double dY;       // EUN-coordinate, meters
        double dZ;       // EUN-coordinate, meters
        double dCosE,dSinE,dCosB,dSinB;
        sPrimaryPt(QCoreTraceFilter::sPrimaryPt &pPriPt);
        virtual ~sPrimaryPt(){}
    };

protected:
    struct sFilterState {
        double dT;       // Filter-current time, seconds
        double dX;       // EUN-coordinate, meters
        double dY;       // EUN-coordinate, meters
        double dZ;       // EUN-coordinate, meters
        double dVX;      // EUN-coordinate, mps
        double dVY;      // EUN-coordinate, mps
        double dVZ;      // EUN-coordinate, mps
        double dAX;      // EUN-coordinate, mps2
        double dAY;      // EUN-coordinate, mps2
        double dAZ;      // EUN-coordinate, mps2
        explicit sFilterState(const QSimpleFilter::sPrimaryPt *pPriPt) {
            dT = pPriPt->dTs;
            dX = pPriPt->dX; dY = pPriPt->dY; dZ = pPriPt->dZ;
            dVX = 0.0e0; dVY = 0.0e0; dVZ = 0.0e0;
            dAX = 0.0e0; dAY = 0.0e0; dAZ = 0.0e0;
        }
    };

public:
    QSimpleFilter(const QCoreTraceFilter &other);
    virtual ~QSimpleFilter();

    virtual bool eatPrimaryPoint(int iPtIdx);
    virtual double calcProximity(const QCoreTraceFilter::sPrimaryPt *pPrimaryPoint) const;
    virtual bool isInsideSpaceStrobe(const struct sPrimaryPt *pPriPt);
    bool filterStep(const struct sPrimaryPt &pp);
    bool linearRegression(const struct sPrimaryPt *pPriPt);
    virtual QCoreTraceFilter::sFilterState getState(double dTime, bool bGetCurrentState = false) const;
    virtual bool isStale(double dTime);

private:
    struct sFilterState *m_pFS;

public:
    // virtual void print();
};

#endif // QSIMPLEFILTER_H
