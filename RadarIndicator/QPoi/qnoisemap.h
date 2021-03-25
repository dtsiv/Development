#ifndef QNOISEMAP_H
#define QNOISEMAP_H

#include <QObject>

class QNoiseMap {
public:
    QNoiseMap(int iNFFT, int iFilteredN, qint64 iNoiseMapsGUID);
    ~QNoiseMap();
    bool appendStrobe(QByteArray &baDopplerRepresentation);
    bool getMap(qint64 &iNoiseMapsGUID, QByteArray &baNoiseMap);
    qint64 getGUID();
    void printMinMax();

private:
    QByteArray m_baSumReY;
    QByteArray m_baSumImY;
    QByteArray m_baSumM2;
    quint32 m_uNStrobs;
    quint32 m_uNFFT;
    quint64 m_uArrSize;
    quint32 m_uFilteredN;
    qint64 m_iNoiseMapsGUID;
    double m_dM1min;
    double m_dM1max;
    double *m_pRe;
    double *m_pIm;
    double *m_pM2;
};

#endif // QNOISEMAP_H
