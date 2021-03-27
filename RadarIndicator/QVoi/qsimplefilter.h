#ifndef QSIMPLEFILTER_H
#define QSIMPLEFILTER_H

#include "qtracefilter.h"

class QSimpleFilter : public QTraceFilter {

protected:
    struct sFilterState {
        double dT;       // Filter-current time, seconds
        double dX;       // EUN-coordinate, meters
        double dY;       // EUN-coordinate, meters
        double dZ;       // EUN-coordinate, meters
        double dVX;      // EUN-coordinate, mps
        double dVY;      // EUN-coordinate, mps
        double dVZ;      // EUN-coordinate, mps
    };

    struct sLocalCoordSystem {
        double dX;       // EUN-coordinate, meters
        double dY;       // EUN-coordinate, meters
        double dZ;       // EUN-coordinate, meters
                         // n_R vector: n_R=\vec{R_L}/R_L
        double dn_RX;    // n_R vector X-coordinate
        double dn_RY;    // n_R vector Y-coordinate
        double dn_RZ;    // n_R vector Z-coordinate
    };

public:
    struct sPrimaryPt : public QTraceFilter::sPrimaryPt {
        double dX;       // EUN-coordinate, meters
        double dY;       // EUN-coordinate, meters
        double dZ;       // EUN-coordinate, meters
        double dCosE,dSinE,dCosB,dSinB;
        sPrimaryPt(QCoreTraceFilter::sPrimaryPt &pPriPt);
        virtual ~sPrimaryPt(){}
    };

    QSimpleFilter(const QCoreTraceFilter &other);
    virtual ~QSimpleFilter();

    virtual bool eatPrimaryPoint(int iPtIdx);
    virtual double calcProximity(const QCoreTraceFilter::sPrimaryPt &primaryPoint) const;
    bool filterStep(const struct sPrimaryPt &pp);
    // dummy state
    double m_dR;
    double m_dV_D;

private:
    struct sFilterState m_sFS;

public:
    // virtual void print();
};

#endif // QSIMPLEFILTER_H
