#include "qpoi.h"
#include "qexceptiondialog.h"

#define PELENG_INV_SIZE 1000

// int iCntMax=0;

const char *QPoi::m_pWeightingType[] = {
    "Raised cosine p=0.65"
};

void QPoi::initPeleng() {
    double dCLight = 2.99e8; //meters per second
    double dLambda = dCLight/m_dCarrierF/1.0e6; // m_dCarrierF MHz
    m_dAzLOverD = dLambda/m_dAntennaSzAz;
    m_dElLOverD = dLambda/m_dAntennaSzEl;

    if (m_iWeighting == QPOI_WEIGHTING_RCOSINE_P065 && m_dBeamDelta0 == 0.487e0) {
        m_dDeltaBound = 1.3e0;
        m_dPelengBound = raised_cos_charact(m_dDeltaBound);
        initPelengInv(&QPoi::raised_cos_charact);
    }
    else {
        m_dDeltaBound = 1.0e0;
        m_dPelengBound = raised_cos_charact(m_dDeltaBound);
        initPelengInv(&QPoi::raised_cos_charact);
    }
}
//--------------------------------------------------------------------------------------------------
//  dAzimuth, dElevation - radians
//--------------------------------------------------------------------------------------------------
bool QPoi::getAngles(double dBeamAmplRe[QPOI_NUMBER_OF_BEAMS],
                     double dBeamAmplIm[QPOI_NUMBER_OF_BEAMS],
                     double &dAzimuth, double &dElevation) {
    double dSumRe = dBeamAmplRe[0] + dBeamAmplRe[1] + dBeamAmplRe[2] + dBeamAmplRe[3];
    double dSumIm = dBeamAmplIm[0] + dBeamAmplIm[1] + dBeamAmplIm[2] + dBeamAmplIm[3];
    double dSum2 = dSumRe*dSumRe + dSumIm*dSumIm;

    double dSumAzRe = dBeamAmplRe[0] + dBeamAmplRe[2] - dBeamAmplRe[1] - dBeamAmplRe[3];
    double dSumAzIm = dBeamAmplIm[0] + dBeamAmplIm[2] - dBeamAmplIm[1] - dBeamAmplIm[3];

    double dSumElRe = dBeamAmplRe[2] + dBeamAmplRe[3] - dBeamAmplRe[0] - dBeamAmplRe[1];
    double dSumElIm = dBeamAmplIm[2] + dBeamAmplIm[3] - dBeamAmplIm[0] - dBeamAmplIm[1];

    double dRatioAz = dSumAzRe*dSumRe+dSumAzIm*dSumIm; dRatioAz/=dSum2;
    double dRatioEl = dSumElRe*dSumRe+dSumElIm*dSumIm; dRatioEl/=dSum2;

    bool bOk;
    double dOffsetAz = getRelAngOffset(dRatioAz, &bOk);
    if (!bOk) return false;
    double dOffsetEl = getRelAngOffset(dRatioEl, &bOk);
    if (!bOk) return false;

    dAzimuth = m_dAzLOverD * dOffsetAz;
    dElevation = m_dElLOverD * dOffsetEl;
    return true;
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
double QPoi::dsinc(double x) {
    double dEPS = 1.0e-8;
    if (x < dEPS && x > -dEPS) return 1.0e0;
    double retval=sin(x);
    retval=retval/x;
    return retval;
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
double QPoi::raised_cos_spec(double Delta, double p /* = 0.65 */ ) {
    double dPI = 3.14159265e0;
    double retVal=p*dsinc(dPI*Delta);
    double dArg=Delta+0.5e0;
    dArg*=dPI;
    retVal+=(1.0e0-p)*dsinc(dArg)/2.0e0;
    dArg=Delta-0.5e0;
    dArg*=dPI;
    retVal+=(1.0e0-p)*dsinc(dArg)/2.0e0;
    return retVal;
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
double QPoi::raised_cos_charact(double Delta) {
    double dEPS = 1.0e-8;
    double dHuge = 1.0e+8;
    double Ip,Im;

    Ip=raised_cos_spec(Delta+m_dBeamDelta0);
    Im=raised_cos_spec(Delta-m_dBeamDelta0);
    double dSum = Im+Ip;
    if (abs(dSum) < dEPS) return dHuge;
    double dRetVal = (Im-Ip)/dSum;
    return dRetVal;
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
double QPoi::dichotomy(double dFuncValue, double dLeft, double dRight, double (QPoi::*pFunc)(double), bool *pbOk /* = NULL */) {
    if (pbOk) *pbOk = false;
    double dEPS = m_dDeltaBound/PELENG_INV_SIZE/10;
    double dFLeft = (this->*pFunc)(dLeft);
    double dFRight = (this->*pFunc)(dRight);
    if (dFLeft > dFuncValue || dFRight < dFuncValue) {
        return 0.0e0;
    }
    double dMid = 0.5e0*(dRight + dLeft);
    int iCnt=0;
    while (dRight - dLeft > dEPS) {
        double dFMid = (this->*pFunc)(dMid);
        if (dFMid > dFuncValue) {
            dRight = dMid;
        }
        else {
            dLeft = dMid;
        }
        dMid = 0.5e0*(dRight + dLeft);
        if (iCnt++>30) return 0.0e0;
    }
    // iCntMax=(iCntMax>iCnt)?iCntMax:iCnt;
    if (pbOk) *pbOk = true;
    return dMid;
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
void QPoi::initPelengInv(double (QPoi::*pFunc)(double)) {
    m_pPelengInv = new double[PELENG_INV_SIZE];
    if (m_pPelengInv == NULL) {
        qDebug() << "QPoi::initPelengInv() m_pPelengInv == NULL";
        throw RmoException("QPoi::initPelengInv() m_pPelengInv == NULL");
    }
    m_dPelengIncr = 2*m_dPelengBound/PELENG_INV_SIZE;
    if (m_dPelengIncr < 1.0e-8) {
        qDebug() << "QPoi::initPelengInv() m_dPelengIncr < 1.0e-8";
        throw RmoException("QPoi::initPelengInv() m_dPelengIncr < 1.0e-8");
    }
    double dDeltaIncr = 2*m_dDeltaBound/PELENG_INV_SIZE;

    // Fill m_pPelengInv[] array
    QFile qfPelengInv("pelenginv.dat");
    qfPelengInv.open(QIODevice::WriteOnly);
    QTextStream tsPelengInv(&qfPelengInv);
    for (int i=0; i<PELENG_INV_SIZE; i++) {
        // qDebug() << "initPelengInv i=" << i;
        double dPelengLower = -m_dPelengBound + m_dPelengIncr*i;
        double dPelengUpper = dPelengLower + m_dPelengIncr;
        double dPelengMid = 0.5*(dPelengLower + dPelengUpper);
        double dDeltaLower = -m_dDeltaBound + dDeltaIncr*i;
        double dDeltaUpper = dDeltaLower + dDeltaIncr;
        bool bOk;
        double dPelengInv;
        dPelengInv = dichotomy(dPelengMid, dDeltaLower, dDeltaUpper, pFunc, &bOk);
        if (!bOk) {
            dPelengInv = dichotomy(dPelengMid, -m_dDeltaBound, m_dDeltaBound, pFunc, &bOk);
            if (!bOk) {
                qDebug() << "QPoi::initPelengInv() failed";
                throw RmoException("QPoi::initPelengInv() failed");
            }
        }
        m_pPelengInv[i] = dPelengInv;
        tsPelengInv << dPelengMid << "\t" << dPelengInv << endl;
    }
    // tsPelengInv << "iCntMax = " << iCntMax << endl;
    qfPelengInv.close();
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
double QPoi::getRelAngOffset(double dPelengRatio, bool *pbOk /* = NULL */) {
    if (pbOk) *pbOk=false;
    if (dPelengRatio >= m_dPelengBound || dPelengRatio < -m_dPelengBound) return 0.0e0;
    int idx = qFloor((dPelengRatio + m_dPelengBound) / m_dPelengIncr);
    if (idx >= PELENG_INV_SIZE || idx < 0) return 0.0e0;
    if (pbOk) *pbOk=true;
    return m_pPelengInv[idx];
}
