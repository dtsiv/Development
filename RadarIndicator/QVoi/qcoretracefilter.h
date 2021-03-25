#ifndef QCORETRACEFILTER_H
#define QCORETRACEFILTER_H

#include <QtCore>

class QVoi;
class QTraceFilter;

class QCoreTraceFilter {

public:
    struct sPrimaryPt {  // base structure for measurements
        double dR;       // distance, meters
        double dElRad;   // elevation, radians
        double dAzRad;   // azimuth, radians
        double dV_D;     // Doppler velocity, m/s
        double dVDWin;   // Doppler velocity window, m/s
        double dTs;      // measurement time, seconds
        QTraceFilter *pFlt;
        sPrimaryPt();
        sPrimaryPt(const sPrimaryPt &other);
        virtual ~sPrimaryPt(){}
    };
    // process primary point using a specific trace filter
    int addPrimaryPoint(struct sPrimaryPt &primaryPoint);

private:
    QCoreTraceFilter(){}
    template <typename T>
    int addPrimaryPoint(struct sPrimaryPt &primaryPoint);

protected:
    QCoreTraceFilter(const QCoreTraceFilter &other);
    ~QCoreTraceFilter(){}

    // private methods&fields accessible from QVoi only
private:
    // assignment operator (currently unused)
    QCoreTraceFilter& operator=(QCoreTraceFilter &);
    // get Singleton instance (cannot delete ever)
    static QCoreTraceFilter& getInstance();
    void cleanup();

    // private boolean flag of initialization
    static bool bInit;

    // simulation parameters accessible for descendants
protected:
    double m_dMeasNoise;
    static QList<struct sPrimaryPt *>m_qlAllPriPts;

    // private members - restrict access to them from individual filters
private:
    static QList<QTraceFilter *>m_qlFilters;
    friend class QVoi;

public:
    // void printAll();
};

#endif // QCORETRACEFILTER_H
