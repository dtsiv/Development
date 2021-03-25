#ifndef QVOI_H
#define QVOI_H

#include <QObject>
#include <QVector>
#include "qproppages.h"

#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "nr.h"
using namespace std;

#define QVOI_PROP_TAB_CAPTION    "VOI"
#define QVOI_MEAS_NOISE_VAR      "measurementNoise"

class QCoreTraceFilter;

class QVoi:public QObject {
    Q_OBJECT
public:
    explicit QVoi(QObject *parent = 0);
    ~QVoi();

    int processPrimaryPoint(
            double dTsExact, // exact time of strobe exection (seconds)
            double dR,       // distance (meters)
            double dElRad,   // elevation (radians)
            double dAzRad,   // azimuth (radians)
            double dV_D,     // Doppler velocity (m/s)
            double dVDWin);  // Doppler velocity window (m/s)

    Q_INVOKABLE void addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);

private:
    QCoreTraceFilter &m_coreInstance;
};
#endif // QVOI_H
