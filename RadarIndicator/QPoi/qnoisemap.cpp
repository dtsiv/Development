#include "qnoisemap.h"
#include <QtGlobal>
#include <QDebug>

#define MAX_MAP_EXTENT 10000

//======================================================================================================
//
//======================================================================================================
QNoiseMap::QNoiseMap(int iNFFT, int iFilteredN, qint64 iNoiseMapsGUID) :
    m_uNStrobs(0)
  , m_uArrSize(0)
  , m_pRe(NULL)
  , m_pIm(NULL)
  , m_pM2(NULL)
  , m_dM1min(1.0e99)
  , m_dM1max(-1.0e99)
  , m_uNFFT(iNFFT)
  , m_uFilteredN(iFilteredN)
  , m_iNoiseMapsGUID(iNoiseMapsGUID) {
    if (m_uNFFT > MAX_MAP_EXTENT || m_uFilteredN > MAX_MAP_EXTENT) {
        qDebug() << "QNoiseMap: m_uPeriodsCount > MAX_MAP_EXTENT || m_uFilteredN > MAX_MAP_EXTENT";
        return;
    }
    m_uArrSize = m_uNFFT*m_uFilteredN*sizeof(double);
    m_baSumReY.resize(m_uArrSize);
    m_baSumImY.resize(m_uArrSize);
    m_baSumM2.resize(m_uArrSize);
    m_pRe = (double *)m_baSumReY.data();
    m_pIm = (double *)m_baSumImY.data();
    m_pM2 = (double *)m_baSumM2.data();
    // make cumulants zero
    for (int iDelay=0; iDelay<m_uFilteredN; iDelay++) {
        for (int kDoppler=0; kDoppler<m_uNFFT; kDoppler++) {
            int idx_base=kDoppler*m_uFilteredN+iDelay;
            m_pRe[idx_base]=0.0e0;
            m_pIm[idx_base]=0.0e0;
            m_pM2[idx_base]=0.0e0;
        }
    }
}
//======================================================================================================
//
//======================================================================================================
QNoiseMap::~QNoiseMap() {
    // no cleanup needed at the moment
}
//======================================================================================================
//
//======================================================================================================
bool QNoiseMap::appendStrobe(QByteArray &baDopplerRepresentation) {
    if (!m_pRe || !m_pIm || !m_pM2) {
        qDebug() << "QNoiseMap::appendStrobe(): !m_pRe || !m_pIm || !m_pM2";
        return false;
    }
    if (baDopplerRepresentation.size() != 2*m_uArrSize) {
        qDebug() << "QNoiseMap::appendStrobe(): size() != 2*m_uArrSize";
        return false;
    }
    double *pDopplerData = (double *)baDopplerRepresentation.data();
    for (int iDelay=0; iDelay<m_uFilteredN; iDelay++) {
        for (int kDoppler=0; kDoppler<m_uNFFT; kDoppler++) {
            double dRe,dIm;
            int idx_base=kDoppler*m_uFilteredN+iDelay;
            dRe=pDopplerData[2*idx_base];
            dIm=pDopplerData[2*idx_base+1];

            // calculate signal in Doppler representation
            m_pRe[idx_base]+=dRe;
            m_pIm[idx_base]+=dIm;
            m_pM2[idx_base]+=dRe*dRe+dIm*dIm;
        }
    }
    m_uNStrobs++;
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QNoiseMap::getMap(qint64 &iNoiseMapsGUID, QByteArray &baNoiseMap) {
    // assign return value
    iNoiseMapsGUID = -1;
    // assign min max
    m_dM1min =  1.0e99;
    m_dM1max = -1.0e99;
    // if no strobes were added - return false
    if (m_uNStrobs < 2) {
        baNoiseMap.clear();
        return false;
    }
    // sample estimate of variance
    baNoiseMap.clear();
    baNoiseMap.resize(m_uArrSize);
    double *pMapData = (double *)baNoiseMap.data();
    for (int iDelay=0; iDelay<m_uFilteredN; iDelay++) {
        for (int kDoppler=0; kDoppler<m_uNFFT; kDoppler++) {
            int idx_base=kDoppler*m_uFilteredN+iDelay;
            // average Re(Y)
            double dRe = m_pRe[idx_base] / m_uNStrobs;
            // average Im(Y)
            double dIm = m_pIm[idx_base] / m_uNStrobs;
            // average |Y|^2
            double dM2 = m_pM2[idx_base] / m_uNStrobs;
            // modulus squared of average: |<Y>|^2
            double dAvrM2 = dRe*dRe + dIm*dIm;
            double dVar = dM2 - dAvrM2;
            dVar = dVar * m_uNStrobs / (m_uNStrobs - 1);
            if (dVar < 1.0e-8) {
                qDebug() << "QNoiseMap::getMap(): dM2: " << dM2 << " dAvrM2 = " << dAvrM2;
                qDebug() << "QNoiseMap::getMap(): dVar < 1.0e-8: " << dVar << " m_uNStrobs = " << m_uNStrobs;
                return false;
            }
            pMapData[idx_base] = sqrt(dVar);
            m_dM1min = ((pMapData[idx_base]<m_dM1min)?pMapData[idx_base]:m_dM1min);
            m_dM1max = ((pMapData[idx_base]>m_dM1max)?pMapData[idx_base]:m_dM1max);
        }
    }
    // assign return values
    iNoiseMapsGUID = m_iNoiseMapsGUID;
    printMinMax();
    return true;
}
//======================================================================================================
//
//======================================================================================================
qint64 QNoiseMap::getGUID() {
    return m_iNoiseMapsGUID;
}
//======================================================================================================
//
//======================================================================================================
void QNoiseMap::printMinMax() {
    qDebug() << "QNoiseMap: m_dM1min = " << m_dM1min << " m_dM1max = " << m_dM1max;
}
