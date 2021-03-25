#include "qpoi.h"

//======================================================================================================
//
//======================================================================================================
bool QPoi::matchedFilter(QByteArray &baSamplesDP, int iDelay, int jPeriod, double &dRe, double &dIm) {
    if (baSamplesDP.size()!=m_iBeamCountsNum*2*sizeof(double)) return false;
    // pointer to array of complex pairs (double,double). This is INPUT array of size m_Np*m_NT_
    double *pData = (double*) baSamplesDP.data();
    // after matched filter the received signal array length is m_iFilteredN samples
    // no need for arrays: only one (Re,Im) output pair.... QByteArray baDataFiltered(2*m_Np*m_iFilteredN*sizeof(double),0);
    // no need for arrays: only one (Re,Im) output pair.... double *pDataFiltered=(double *)baDataFiltered.data();
    if (m_baSignalSamplData.size() != 2*m_Ntau*sizeof(qint16)) return false;
    // Copy of thre transmitted signal (complex-conjugate, probably)
    qint16 *pPulseCopy=(qint16 *)m_baSignalSamplData.data();
    // loop over periods
    // for (int jPeriod=0; jPeriod<m_Np; jPeriod++) {
        // loop over one period (recorded fragment). Shift index iDelay runs trough 0...m_NT_+m_Ntau-2 inclusive
        // for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) { // index within one period
            // now these are return output parameters: double dRe=0,dIm=0; // running cumulants
            int itau_min = qMax(0,m_Ntau-iDelay-1);
            int itau_max = qMin(m_Ntau-1,m_NT_-iDelay+m_Ntau-2);
            for (int itau=itau_min; itau<=itau_max; itau++) { // summation over pulse duration:
                                                         // itau=itau_min,itau_min+1,...,itau_max
                int idx_in; // index of input array pData: idx_in=2*(jPeriod*m_NT_),...,2*(jPeriod*m_NT_+m_NT_-1)
                idx_in=2*(jPeriod*m_NT_ + (iDelay-m_Ntau+1+itau) ); // jPeriod*m_NT_ - offset of period beginning;
                                                              // iDelay - offset of filter output: iDelay=0,1,...,(m_iFilteredN-1)
                                                              // (iDelay-m_Ntau+1+itau) = max(0,iDelay-m_Ntau+1), ... , min(iDelay, m_NT_-1)
                // complex (qint16,qint16) sample of pulse copy (m_cvPulseCopy)
                if (itau > m_Ntau-1) return false;
                qint16 iPuCoRe = pPulseCopy[2*itau];
                qint16 iPuCoIm = pPulseCopy[2*itau+1];
                if (idx_in-2*jPeriod*m_NT_ > 2*(m_NT_-1)) return false;
                //------------------------ debug: (Recvd_sig)*(Copy_complexConjugate)
//                dRe+=iPuCoRe*pData[idx_in    ] + iPuCoIm*pData[idx_in + 1]; // the first array index is 2*(jPeriod*m_NT_)
//                dIm+=iPuCoRe*pData[idx_in + 1] - iPuCoIm*pData[idx_in    ]; // the last array index is 2*(jPeriod*m_NT_+m_NT_-1)+1 = 2*(jPeriod*m_NT_+m_NT_)-1
                //------------------------------------------------------------------
                //------------------------ debug: (Recvd_sig)*(Copy_noConjugation)
                dRe+=iPuCoRe*pData[idx_in    ] - iPuCoIm*pData[idx_in + 1]; // the first array index is 2*(jPeriod*m_NT_)
                dIm+=iPuCoRe*pData[idx_in + 1] + iPuCoIm*pData[idx_in    ]; // the last array index is 2*(jPeriod*m_NT_+m_NT_-1)+1 = 2*(jPeriod*m_NT_+m_NT_)-1
                //------------------------------------------------------------------
                // Thus within one period we span (2*m_NT_) pData array elements: 2*(jPeriod*m_NT_)+0, ..., 2*(jPeriod*m_NT_+m_NT_)-1
            }
            // idx - index of output array pDataFiltered: idx=2*(jPeriod*m_iFilteredN),...,2*(jPeriod*m_iFilteredN+m_iFilteredN-1)
            // no need for arrays: only one (Re,Im) output pair.... int idx=2*(jPeriod*m_iFilteredN+ iDelay); // jPeriod*m_iFilteredN - offset of period beginning; iDelay - offset of filter output: iDelay=0,1,...,(m_iFilteredN-1)
            // Thus within one period we produce 2*(m_iFilteredN) pDataFiltered array elements of type double
            // no need for arrays: only one (Re,Im) output pair.... pDataFiltered[idx]=dRe; pDataFiltered[idx+1]=dIm;
        // no loops: scalar product for fixed jPeriod and iDelay }
    // no loops: scalar product for fixed jPeriod and iDelay }
    // return array of complex pairs (double,double) with indexing: (Re) 2*(jPeriod*m_iFilteredN+ iDelay), (Im) 2*(jPeriod*m_iFilteredN+ iDelay)+1
    return true;
}
