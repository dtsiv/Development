#include "qpoi.h"
#include "qexceptiondialog.h"

#include <QTextStream>
#include "nr.h"
using namespace std;

//======================================================================================================
//
//======================================================================================================
QByteArray QPoi::dopplerRepresentation(QByteArray &baSamplesDP) {
    if (m_iBeamCountsNum != m_Np*m_NT_
            || m_NT_<m_Ntau
            || m_NT<m_NT_
            // 20210216: changed  from || m_iFilteredN != m_NT_ ... to
            || m_iFilteredN != m_NT_+m_Ntau-1
            || m_iSigType != m_iSigID_fromStrobeHeader
            || m_iSignalLen != m_Ntau
            || m_baSignalSamplData.size() != 2*m_Ntau*sizeof(qint16)
            || baSamplesDP.size()!=m_iBeamCountsNum*2*sizeof(double)) {
        throw RmoException("QPoi::dopplerRepresentation(): QPoi internal parameters error");
        return QByteArray();
    }
    // array for signal after matched filter
    QByteArray baDataFiltered(2*m_Np*m_iFilteredN*sizeof(double),0);
    // after matched filter the received signal array length is m_iFilteredN samples for every period
    double *pDataFiltered=(double *)baDataFiltered.data();

    //----------------- debug!!!
    //    if (m_uStrobeNoForDebugging==6813) {
    //        for (int itau=0; itau<m_Ntau; itau++) {
    //            int idx_in;
    //            int k_detection = 100;
    //            idx_in=2*(k_detection*m_NT_ + itau); //
    //            pData[idx_in    ] = pPulseCopy[2*itau];
    //            pData[idx_in + 1] = pPulseCopy[2*itau+1];
    //        }
    //    }
    //---------------------------

    // loop over periods
    for (int jPeriod=0; jPeriod<m_Np; jPeriod++) {
        // loop over one period (recorded fragment). Shift index iDelay runs trough 0...m_NT_+m_Ntau-2 inclusive
        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) { // index within one period
            double dRe=0,dIm=0; // running cumulants
            if (!matchedFilter(baSamplesDP, iDelay, jPeriod, dRe, dIm)) {
                throw RmoException("QPoi::dopplerRepresentation(): matchedFilter() failed");
                return QByteArray();
            }
            int idx; // index of output array pDataFiltered: idx=2*(k*m_NT_),...,2*(k*m_NT_+m_iFilteredN-1)
            idx=2*(jPeriod*m_iFilteredN+ iDelay); // k*m_iFilteredN - offset of period beginning; i - offset of filter output: i=0,1,...,(m_iFilteredN-1)
            // Thus within one period we produce 2*(m_iFilteredN) pDataFiltered array elements of type double
            pDataFiltered[idx]=dRe; pDataFiltered[idx+1]=dIm;
        }
    }

    if (m_iRejectorType == QPOI_REJECTOR_NARROW_BAND) {
        // interference rejector: cancellation of zero-frequency interference
        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) { // index within one period
            double dSumRe=0.0e0, dSumIm=0.0e0;
            for (int jPeriod=0; jPeriod<m_Np; jPeriod++) {
                // index of output array pDataFiltered: idx=2*(jPeriod*m_iFilteredN),...,2*(jPeriod*m_iFilteredN+m_iFilteredN-1)
                int idx=2*(jPeriod*m_iFilteredN+  iDelay); // jPeriod*m_iFilteredN - offset of period beginning; iDelay - offset of filter output: i=0,1,...,(m_iFilteredN-1)
                dSumRe+=pDataFiltered[idx];
                dSumIm+=pDataFiltered[idx+1];
            }
            dSumRe/=m_Np;
            dSumIm/=m_Np;
            for (int jPeriod=0; jPeriod<m_Np; jPeriod++) {
                int idx=2*(jPeriod*m_iFilteredN+  iDelay); // jPeriod*m_iFilteredN - offset of period beginning; iDelay - offset of filter output: i=0,1,...,(m_iFilteredN-1)
                pDataFiltered[idx]  -=dSumRe;
                pDataFiltered[idx+1]-=dSumIm;
            }
        }
    }

    if (m_iRejectorType == QPOI_REJECTOR_CHE_PE_KA) {
        // interference rejector: inter-period cancellation (+-)
        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) { // index within one period
            for (int jPeriod=0; jPeriod<m_Np-1; jPeriod++) { // here we must use (m_Np-1), not Np!!!
                double dSumRe=0.0e0, dSumIm=0.0e0;
                // Term Signal(jPeriod+1), negative sign
                int idx=2*((jPeriod+1)*m_iFilteredN+ iDelay); // jPeriod*m_iFilteredN - offset of period beginning; iDelay - offset of filter output: iDelay=0,1,...,(m_iFilteredN-1)
                dSumRe-=pDataFiltered[idx];
                dSumIm-=pDataFiltered[idx+1];
                // Term Signal(jPeriod), positive sign
                idx=2*(jPeriod*m_iFilteredN+ iDelay); // jPeriod*m_iFilteredN - offset of period beginning; iDelay - offset of filter output: iDelay=0,1,...,(m_iFilteredN-1)
                dSumRe+=pDataFiltered[idx];
                dSumIm+=pDataFiltered[idx+1];
                pDataFiltered[idx]=dSumRe;
                pDataFiltered[idx+1]=dSumIm;
            }
            int idx=2*((m_Np-1)*m_iFilteredN+ iDelay); // final element: jPeriod=(m_Np-1) - just put zero for simplicity
            pDataFiltered[idx]=0.0e0;
            pDataFiltered[idx+1]=0.0e0;
        }
    }

    // Hamming window for weighting
    QByteArray baHammingWin=getHammingWindow();
    if (baHammingWin.isEmpty()) {
        throw RmoException("QPoi::dopplerRepresentation: getHammingWindow() failed");
        return QByteArray();
    }
    double *pHammingWin=(double *)baHammingWin.data();

    // Fourier transform
    QByteArray retVal(2*m_NFFT*m_iFilteredN*sizeof(double),0);
    double *pRetData=(double *)retVal.data();
    Vec_DP inData(m_NFFT*2),vecZeroes(m_NFFT*2);
    for (int kDummy=0; kDummy<m_NFFT*2; kDummy++) vecZeroes[kDummy]=0.0e0;

    // loop over echo delay
    for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) {
        inData=vecZeroes;
        // if (i==10) tsStdOut << i << "\t" << QString::number(inData[2*m_Np]) << "\t" << QString::number(inData[2*m_Np+1]) << endl;
        for (int jPeriod=0; jPeriod<m_Np; jPeriod++) {
            int idx=2*(jPeriod*m_iFilteredN+iDelay);
            inData[2*jPeriod]=pHammingWin[jPeriod]*pDataFiltered[idx];
            inData[2*jPeriod+1]=pHammingWin[jPeriod]*pDataFiltered[idx+1];
        }
        int isign=1;
        NR::four1(inData,isign);
        for (int kDoppler=0; kDoppler<m_NFFT; kDoppler++) {
            int idxW=2*(kDoppler*m_iFilteredN+iDelay);
            pRetData[idxW]=inData[2*kDoppler]/m_NFFT;
            pRetData[idxW+1]=inData[2*kDoppler+1]/m_NFFT;
        }
    }
    return retVal;
}
//======================================================================================================
//
//======================================================================================================
bool QPoi::getPointDopplerRep(int iDelay, int kDoppler,
        QByteArray pbaBeamDataDP[QPOI_NUMBER_OF_BEAMS],
        double dBeamAmplRe[QPOI_NUMBER_OF_BEAMS],
        double dBeamAmplIm[QPOI_NUMBER_OF_BEAMS]) {
    if (m_iBeamCountsNum != m_Np*m_NT_
            || m_NT_<m_Ntau
            || m_NT<m_NT_
            // 20210216: changed  from || m_iFilteredN != m_NT_ ... to
            || m_iFilteredN != m_NT_+m_Ntau-1
            || m_iSigType != m_iSigID_fromStrobeHeader
            || m_iSignalLen != m_Ntau
            || m_baSignalSamplData.size() != 2*m_Ntau*sizeof(qint16)) {
        throw RmoException("QPoi::getPointDopplerRep(): QPoi internal parameters error");
        return false;
    }

    // check kDoppler against interval 0...m_NFFT-1
    if (kDoppler<0 || kDoppler>m_NFFT-1) {
        throw RmoException("QPoi::getPointDopplerRep(): kDoppler<0 || kDoppler>m_NFFT-1");
        return false;
    }
    // check iDelay against interval 0...m_iFilteredN-1; that is: 0...m_NT_+m_Ntau-2
    if (iDelay<0 || iDelay>m_NT_+m_Ntau-2) {
        throw RmoException("QPoi::getPointDopplerRep(): iDelay<0 || iDelay>m_NT_+m_Ntau-2");
        return false;
    }
    // check size of input arrays
    for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) {
        int iDataSize = pbaBeamDataDP[iBeam].size();
        if (iDataSize!=m_iBeamCountsNum*2*sizeof(double)) {
            throw RmoException("QPoi::getPointDopplerRep(): iDataSize!=iBeamCountsNum*2*sizeof(double)");
            return false;
        }
    }
    // matched filter applied for all beams
    QByteArray baDataFiltered(2*m_Np*QPOI_NUMBER_OF_BEAMS*sizeof(double),(char)0);
    double *pDataFiltered = (double *)baDataFiltered.data();
    for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) { //  loop over beams
        for (int jPeriod=0; jPeriod<m_Np; jPeriod++) { // loop over periods
            double dRe=0, dIm=0; // scalar product with m_baSignalSamplData
            if (!matchedFilter(pbaBeamDataDP[iBeam],iDelay,jPeriod,dRe,dIm)) {
                throw RmoException("QPoi::getPointDopplerRep(): matchedFilter failed");
                return false;
            }
            int idx=2*(jPeriod*QPOI_NUMBER_OF_BEAMS + iBeam); // index of output array pDataFiltered: idx=2*(jPeriod*4),...,2*(jPeriod*4+3)
            // Thus within one period we produce 2*(QPOI_NUMBER_OF_BEAMS) pDataFiltered array elements of type double
            pDataFiltered[idx]=dRe;
            pDataFiltered[idx+1]=dIm;
        }
    }

    // Hamming window with size m_NFFT
    QByteArray baHammingWin=getHammingWindow();
    if (baHammingWin.isEmpty()) {
        throw RmoException("QPoi::PtDopplerRep(): getHammingWindow() failed");
        return false;
    }
    double *pHammingWin=(double *)baHammingWin.data();

    // Output parameters: Complex amplitude for 4 beams at resolution element (iDelay, kDoppler)
    for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) {
        dBeamAmplRe[iBeam] = 0.0e0;
        dBeamAmplIm[iBeam] = 0.0e0;
    }
    // Doppler representation for all beams
    Vec_DP inData(m_NFFT*2),vecZeroes(m_NFFT*2);
    for (int kDummy=0; kDummy<m_NFFT*2; kDummy++) vecZeroes[kDummy]=0.0e0;
    for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) { //  loop over beams
        inData=vecZeroes;
        for (int jPeriod=0; jPeriod<m_Np; jPeriod++) { // loop over periods
            int idx=2*(jPeriod*QPOI_NUMBER_OF_BEAMS+iBeam);
            inData[2*jPeriod]=pHammingWin[jPeriod]*pDataFiltered[idx];
            inData[2*jPeriod+1]=pHammingWin[jPeriod]*pDataFiltered[idx+1];
        }
        int isign=1;
        NR::four1(inData,isign);
        // we only use one sample within Fourier spectrum
        dBeamAmplRe[iBeam] = inData[2*kDoppler]/m_NFFT;
        dBeamAmplIm[iBeam] = inData[2*kDoppler+1]/m_NFFT;
    }
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QPoi::updateNFFT() {
    if (this->m_Np <= 0) return false;
    // number of points in FFT
    int iNFFT;
    for (int i=0; i<31; i++) {
        iNFFT = 1<<i;
        if (this->m_Np <= iNFFT) break;
        iNFFT=0;
    }
    if (iNFFT < this->m_Np || iNFFT > 2*this->m_Np) return false;
    this->m_NFFT = iNFFT;

    // phy scales
    updatePhyScales();

    return true;
}
//======================================================================================================
//
//======================================================================================================
QByteArray QPoi::getHammingWindow() {
    if (m_NFFT < m_Np || m_NFFT > 2*m_Np) return QByteArray();
    // Hamming window with size m_NFFT
    QByteArray baHammingWin(m_NFFT*sizeof(double),0);
    double *pHammingWin=(double *)baHammingWin.data();
    double dHamA0=25.0/46;
    double dHamA1=21.0/46;
    double dHamW=2.0*3.14159265/(m_Np-1);
    for (int j=0; j<m_Np; j++) {
        pHammingWin[j]=dHamA0-dHamA1*cos(dHamW*j);
    }
    for (int j=m_Np; j<m_NFFT; j++) {
        pHammingWin[j]=0.0e0;
    }
    return baHammingWin;
}
