#include "qpoi.h"
#include "qexceptiondialog.h"

#include "nr.h"
using namespace std;

#define SKIP_ZERO_DOPPLER          0         // when noise map is used - no need to skip doppler channels
#define SKIP_ZERO_DELAY            0

static int iCnt=0;

//======================================================================================================
//
//======================================================================================================
bool QPoi::detectTargets(QByteArray &baSamplesDP, QByteArray &baStrTargets, int &nTargets) {
    bool retVal=false;

    iCnt++;

    QByteArray baDopplerRepresentation = dopplerRepresentation(baSamplesDP);
    if (baDopplerRepresentation.isEmpty()) {
        qDebug() << "baDopplerRepresentation.isEmpty()";
        return false;
    }
    if (baDopplerRepresentation.size()!=2*m_iFilteredN*m_NFFT*sizeof(double)) {
        qDebug() << "baDopplerRepresentation.size()!=2*m_iFilteredN*m_NFFT*sizeof(double)";
        return false;
    }
    double *pDopplerData=(double*)baDopplerRepresentation.data();


    //================================================================================================================================
    //  ok, at this point the Doppler representation (Re,Im)=(pDopplerData[2*(k*m_iFilteredN+i)],pDopplerData[2*(k*m_iFilteredN+i)+1])
    //  where k = 0,...,(m_NFFT-1) is Doppler frequency index, currently 20191016 this is 0,...,1023
    //  and   i = 0, ..., (m_iFilteredN-1) is delay index, currently 20191016 this is 0,...,72
    //================================================================================================================================

    // initialize number of candidates nc
    int nc=0;

    // 20200502: if (m_iFilteredN!=m_NT_-m_Ntau+1) {  ---- changed to: m_iFilteredN!=m_NT_
    // 20210216 changed to: m_NT_+m_Ntau-1
    if (m_iFilteredN!=m_NT_+m_Ntau-1) {
         // before 20200502: qDebug() << "QPoi::detectTargets(): m_iFilteredN!=m_NT_-m_Ntau+1";
        // 20210216 changed to: m_NT_+m_Ntau-1
        qDebug() << "QPoi::detectTargets(): m_iFilteredN!=m_NT_+m_Ntau-1";
        return false;
    }
    if (m_NFFT <= 2*SKIP_ZERO_DOPPLER) {
        qDebug() << "negative m_NFFT-SKIP_ZERO_DOPPLER";
        return false;
    }
    // rescale the signal using noise map
    if (m_bUseNoiseMap) {
        if (m_baNoiseMap.size() != m_iFilteredN*m_NFFT*sizeof(double)) {
            qDebug() << "m_baNoiseMap.size() != m_iFilteredN*m_NFFT*sizeof(double)";
            return false;
        }
        double *pNoiseMap = (double *)m_baNoiseMap.data();
        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) {
            for (int kDoppler=0; kDoppler<m_NFFT; kDoppler++) {
                int idx_base=kDoppler*m_iFilteredN+iDelay;
                double dRe=pDopplerData[2*idx_base];
                double dIm=pDopplerData[2*idx_base+1];
                double dNoiseAmpl = pNoiseMap[idx_base];
                if (dNoiseAmpl < 1.0e-8) {
                        qDebug() << "dNoiseAmpl < 1.0e-8";
                        return false;
                }
                pDopplerData[2*idx_base]   = dRe / dNoiseAmpl;
                pDopplerData[2*idx_base+1] = dIm / dNoiseAmpl;
            }
        }
    }
    //---------------------------------------------------------------- debug: normalize inside Doppler channels
    //    if (m_iFilteredN<1) return false;
    //    for (int kDoppler=0; kDoppler<m_NFFT; kDoppler++) {
    //        double dAvrM2=0.0e0;
    //        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) {
    //            int idx_base=kDoppler*m_iFilteredN+iDelay;
    //            double dRe=pDopplerData[2*idx_base];
    //            double dIm=pDopplerData[2*idx_base+1];
    //            dAvrM2+=dRe*dRe+dIm*dIm;
    //        }
    //        double dSamplingSigma = dAvrM2/(m_iFilteredN);
    //        dSamplingSigma=sqrt(dSamplingSigma);
    //        for (int iDelay=0; iDelay<m_iFilteredN; iDelay++) {
    //            int idx_base=kDoppler*m_iFilteredN+iDelay;
    //            pDopplerData[2*idx_base]/=dSamplingSigma;
    //            pDopplerData[2*idx_base+1]/=dSamplingSigma;
    //        }
    //    }
    //-----------------------------------------------------------------------------------------------------------

    // candidates detection
    QList<int> kc,lc;    // doppler channel, delay for candidate
    QList<double> y2mc;  // signal energy for candidate
    QList<double> SNRc;  // candidate SNR
    for (int iDelay=SKIP_ZERO_DELAY; iDelay<m_iFilteredN; iDelay++) {
        for (int kDoppler1=0; kDoppler1<m_NFFT-2*SKIP_ZERO_DOPPLER; kDoppler1++) {
            int kDoppler = m_NFFT/2+kDoppler1;
            if (kDoppler >= m_NFFT-SKIP_ZERO_DOPPLER) kDoppler=kDoppler-m_NFFT+2*SKIP_ZERO_DOPPLER;
            int idx=kDoppler*m_iFilteredN+iDelay;
            Vec_DP dSignal(2);
            dSignal[0]=pDopplerData[2*idx];
            dSignal[1]=pDopplerData[2*idx+1];
            Vec_DP dNoise((2*NOISE_AVERAGING_N-2)*2);
            int idxNoise=0;
            for (int iShift=-NOISE_AVERAGING_N; iShift<=NOISE_AVERAGING_N; iShift++) {
                if (iShift==-1 || iShift==0 || iShift==1) continue;
                int k1=kDoppler+iShift;
                if (k1<0) k1+=m_NFFT;
                if (k1>=m_NFFT) k1-=m_NFFT;
                int idxWindow=k1*m_iFilteredN+iDelay;
                if (idxNoise>dNoise.size()-2) {
                    throw RmoException("QPoi: idxNoise>dNoise.size()-2");
                }
                dNoise[idxNoise++]=pDopplerData[2*idxWindow];
                dNoise[idxNoise++]=pDopplerData[2*idxWindow+1];
            }
            int n_noise=dNoise.size();
            int n_signal=dSignal.size();
            if (n_signal!=2 || n_noise!=idxNoise) {
                throw RmoException("QPoi: n_signal!=2 || n_noise!=idxNoise");
            }
            DP ave_noise,ave_signal;
            DP var_noise,var_signal;
            avevar_poi(dNoise,ave_noise,var_noise);      // var_noise  = SUM_i [(Re Noise_i)^2  + (Im Noise_i)^2 ] / (2*(2*NOISE_AVERAGING_N-2))
            avevar_poi(dSignal,ave_signal,var_signal);   // var_signal =       [(Re Signal_i)^2 + (Im Signal_i)^2] / 2
            DP dFrac,dProb;
            dFrac=var_signal/var_noise; // signal-to-noise
            // Weinberg 2017 parameter
            double dTau = dFrac/(0.5*n_noise);
            // false alarm probability
            dProb = exp(-(0.5*n_noise)*log(1+dTau));
            //==========================================================================================
            // switch from full Fisher test
            // to simplified Winberg(2017) formula
            //==========================================================================================
            // void QPoi::ftest_poi(Vec_I_DP &data_noise, Vec_I_DP &data_signal, DP &f, DP &prob) {
            // DP var_noise,var_signal,ave_noise,ave_signal;
            //
            // int n_noise=data_noise.size();
            // int n_signal=data_signal.size();
            // if (n_signal!=2) qFatal("n_signal!=2");
            // avevar_poi(data_noise,ave_noise,var_noise);
            // avevar_poi(data_signal,ave_signal,var_signal);
            // f=var_signal/var_noise; // signal-to-noise
            // prob = NR::betai(0.5*n_noise,0.5*n_signal,n_noise/(n_noise+n_signal*f));
            //==========================================================================================
            // ftest_poi(dNoise,dSignal,dFrac,dProb);
            // dFracMax=(dFracMax>dFrac)?dFracMax:dFrac;
            // dProbMin=(dProbMin<dProb)?dProbMin:dProb;
            //==========================================================================================
            if (dProb < m_dFalseAlarmProb) {
                nc++;
                kc.append(kDoppler);
                lc.append(iDelay + m_iBlank);
                y2mc.append(2*var_signal);
                SNRc.append(dFrac);
            }
        }
    }
    if (!nc) {
        // qDebug() << "No candidates!";
        return retVal;
    }

    // multiple targets resolution
    int ntg=0;
    QList<int> tgs;
    QList<double> ktg,ltg;
    QList<double> y2mtg;
    QList<double> SNRtg;
    QList<double> y2mtg_sum;
    QList<int> lmtg,kmtg;

    //-------------------------------------------------------Debug!!!
    //        if (m_uStrobeNoForDebugging == 6933) { // debug output of candidates
    //            QFile qfDebugCandidates("candidates.dat");
    //            if (qfDebugCandidates.open(QIODevice::WriteOnly)) {
    //                qDebug() << "Opened " << QFileInfo(qfDebugCandidates).absoluteFilePath();
    //                QTextStream tsDebugCandidates(&qfDebugCandidates);
    //                for (int ic=0; ic<nc; ic++) {  // loop over all candidates - resolution elements (l,k) with threshold reached
    //                    int kc_=kc.at(ic);             // resolution element along Doppler dimension for candidate ic
    //                    int lc_=lc.at(ic);             // resolution element along delay for candidate ic
    //                    double dPow = y2mc.at(ic);     // signal pwr @ candidate
    //                    tsDebugCandidates << lc_ << "\t" << kc_ << "\t"
    //                                      << qSetFieldWidth(15) << right << qSetRealNumberPrecision(6)
    //                                      << dPow
    //                                      << endl;
    //                }
    //            }
    //            else {
    //                qDebug() << "Could not open " << QFileInfo(qfDebugCandidates).absoluteFilePath();
    //            }
    //        }
    //----------------------------------------------------------------
#if 0
COMPLEX ALGORITHM FOT MULTIPLE TARGETS....
    for (int ic=0; ic<nc; ic++) {  // loop over all candidates - resolution elements (l,k) with threshold reached
        bool bFlag=false;          // found candidates attributed to running target
        for (int itg=0; itg<ntg; itg++) {  // loop over existing targets itg
            int ktg_=ktg.at(itg);          // target mass center (Doppler)
            int ltg_=ltg.at(itg);          // target mass center (delay)
            int kc_=kc.at(ic);             // resolution element along Doppler dimension for candidate ic
            int lc_=lc.at(ic);             // resolution element along delay for candidate ic
            // test delay difference
            if (abs(lc_-ltg_)>m_uMaxTgSize) continue;     // if candidate delay is too far from target, then skip
            // test Doppler channel difference
            double kdiff=kc_-ktg_;
            if (kdiff>m_NFFT/2) kdiff-=m_NFFT;
            if (kdiff<-m_NFFT/2) kdiff+=m_NFFT;
            if (abs(kdiff)>1) continue;             // if Doppler channel difference is greater than 1, then skip
            //----------debug!!!
            // if (abs(kdiff)>2) continue;
            //------------------
            // candidate associated with itg
            bFlag=true;
            int tgs_=tgs.at(itg)+1;                             // candidate belongs to target
            double y2mtg_sum_=y2mtg_sum[itg]+y2mc.at(ic);       // previous value of target power
//-----------------------------------------------------------------------------------------
// 20210214: before today target center was mass center with all candidates having 'equal mass'
//            // recalculate k-center
//            kc_=ktg_+kdiff;
//            ktg[itg]=(ktg.at(itg)*tgs.at(itg)+kc_)/tgs_;
//            if (ktg.at(itg)>m_NFFT) ktg[itg]-=m_NFFT;
//            if (ktg.at(itg)<0) ktg[itg]+=m_NFFT;
//            // recalculate l-center
//            ltg[itg]=(ltg.at(itg)*tgs.at(itg)+lc_)/tgs_;
//-----------------------------------------------------------------------------------------
// 20210214: since today: target center is mass center of all candidates with their signal power as 'masses'
            // recalculate k-center
            kc_=ktg_+kdiff;
            ktg[itg]=(ktg.at(itg)*y2mtg_sum[itg]+kc_*y2mc.at(ic))/y2mtg_sum_;
            if (ktg.at(itg)>m_NFFT) ktg[itg]-=m_NFFT;
            if (ktg.at(itg)<0) ktg[itg]+=m_NFFT;
            // recalculate l-center
            ltg[itg]=(ltg.at(itg)*y2mtg_sum[itg]+lc_*y2mc.at(ic))/y2mtg_sum_;
            // update number of candidates in this target
            tgs[itg]=tgs_;                                      // update number of candidates for target
            // update sum of candidates'signal power for this target
            y2mtg_sum[itg]=y2mtg_sum_;                          // update cumulative power for target
            // update target maximum M2
            if (y2mtg.at(itg)<y2mc.at(ic)) {
                y2mtg[itg]=y2mc.at(ic);
                SNRtg[itg]=SNRc.at(ic);
                lmtg[itg]=lc.at(ic);
                kmtg[itg]=kc.at(ic);
            }
        }
        if (bFlag) continue; // candidate belongs to existing target, hence continue
        // create new target from current candidate
        ltg.append(lc.at(ic));
        ktg.append(kc.at(ic));
        tgs.append(1);
        y2mtg.append(y2mc.at(ic));
        SNRtg.append(SNRc.at(ic));
        y2mtg_sum.append(y2mc.at(ic));
        lmtg.append(lc.at(ic));
        kmtg.append(kc.at(ic));
        ntg=ntg+1;
    }
... END OF COMPLEX ALGORITHM FOT MULTIPLE TARGETS
#endif

//---- SIMPLE ALGORITHM FOR ONE TARGET -------------------------------------------------------
//    ntg = 0;
//    if (nc) {
//        int ic=0;
//        ltg.append(100000);
//        ktg.append(100000);
//        tgs.append(m_uTargSzThresh+1);
//        y2mtg.append(0);
//        SNRtg.append(0);
//        y2mtg_sum.append(y2mc.at(ic));
//        lmtg.append(lc.at(ic));
//        kmtg.append(kc.at(ic));
//    }
//    int itg = 0;
//    for (int ic=0; ic<nc; ic++) {      // loop over all candidates - resolution elements (l,k) with threshold reached
//        int kc_=kc.at(ic);             // resolution element along Doppler dimension for candidate ic
//        int lc_=lc.at(ic);             // resolution element along delay for candidate ic
//        if (kc_ == 120
//         || kc_ == 121
//         || kc_ == 392
//         || kc_ == 263
//         || kc_ == 135
//         || kc_ == 136) continue;
//        ntg = 1;
//        double y2m_curr = y2mc.at(ic);      // signal modulus squared
//        if (y2m_curr > y2mtg.at(itg)) {
//            ltg[itg]=lc_;
//            ktg[itg]=kc_;
//            y2mtg[itg]=y2m_curr;
//            SNRtg[itg]=SNRc.at(ic);
//            y2mtg_sum[itg]=y2m_curr;
//            lmtg[itg]=lc_;
//            kmtg[itg]=kc_;
//        }
//    }
//------ debug
//    if (ntg) {
//        qDebug() << m_uStrobeNoForDebugging << " k=" << ktg[itg];
//    }
//-------------
//---- END OF SIMPLE ALGORITHM FOR ONE TARGET ------------------------------------------------

    //---- TARGET SEPARATION ALGORITHM FOR CHIRP SOUNDING PULSE ----------------------------------
    for (int ic=0; ic<nc; ic++) {  // loop over all candidates - resolution elements (l,k) with threshold reached
        bool bFlag=false;          // Flag means that this candidate belongs to existing target
        int kc_=kc.at(ic);             // resolution element along Doppler dimension for candidate ic
        // debug: exclude rabbish frequencies
        if (m_qmmSkipDoppler.contains(m_iSigType, kc_)) continue;
        //        if (//--- signal type 5
        //            (kc_ ==   0 && m_iSigType == 5)
        //         || (kc_ ==  63 && m_iSigType == 5)
        //         || (kc_ == 120 && m_iSigType == 5)
        //         || (kc_ == 121 && m_iSigType == 5)
        //         || (kc_ == 149 && m_iSigType == 5)
        //         || (kc_ == 150 && m_iSigType == 5)
        //         || (kc_ == 391 && m_iSigType == 5)
        //         || (kc_ == 392 && m_iSigType == 5)
        //          //--- signal type 6
        //         || (kc_ ==   0 && m_iSigType == 6)
        //         || (kc_ ==  66 && m_iSigType == 6)
        //         || (kc_ == 120 && m_iSigType == 6)
        //         || (kc_ == 121 && m_iSigType == 6)
        //         || (kc_ == 135 && m_iSigType == 6)
        //         || (kc_ == 136 && m_iSigType == 6)
        //         || (kc_ == 150 && m_iSigType == 6)
        //         || (kc_ == 151 && m_iSigType == 6) ) continue;
        for (int itg=0; itg<ntg; itg++) {  // loop over existing targets itg
            double ktg_=ktg.at(itg);          // target mass center (Doppler)
            // int ltg_=ltg.at(itg);       // target mass center (delay)
            // int lc_=lc.at(ic);             // resolution element along delay for candidate ic
            // test delay difference
            // if (abs(lc_-ltg_)>m_uMaxTgSize) continue;     // if candidate delay is too far from target, then skip
            // test Doppler channel difference
            double kdiff=kc_-ktg_;
            if (kdiff>m_NFFT/2) kdiff-=m_NFFT;
            if (kdiff<-m_NFFT/2) kdiff+=m_NFFT;
            //----------debug!!!
            // if (abs(kdiff)>1) continue;             // if Doppler channel difference is greater than 1, then skip
            //----------debug!!!
            if (abs(kdiff)>2) continue;                // if Doppler channel difference is greater than 2, then skip
            //------------------
            // candidate associated with itg
            bFlag=true;
            int tgs_=tgs.at(itg)+1;                             // candidate belongs to target
            double y2mtg_sum_=y2mtg_sum[itg]+y2mc.at(ic);       // previous value of target power
            // recalculate k-center
            // kc_=ktg_+kdiff;
            // ktg[itg]=(ktg.at(itg)*y2mtg_sum[itg]+kc_*y2mc.at(ic))/y2mtg_sum_;
            // if (ktg.at(itg)>m_NFFT) ktg[itg]-=m_NFFT;
            // if (ktg.at(itg)<0) ktg[itg]+=m_NFFT;
            // recalculate l-center
            //...use ArgMax instead... ltg[itg]=(ltg.at(itg)*y2mtg_sum[itg]+lc_*y2mc.at(ic))/y2mtg_sum_;
            // update number of candidates in this target
            tgs[itg]=tgs_;                                      // update number of candidates for target
            // update sum of candidates'signal power for this target
            y2mtg_sum[itg]=y2mtg_sum_;                          // update cumulative power for target
            // update target maximum M2
            if (y2mtg.at(itg)<y2mc.at(ic)) {
                y2mtg[itg]=y2mc.at(ic);
                SNRtg[itg]=SNRc.at(ic);
                lmtg[itg]=lc.at(ic);
                kmtg[itg]=kc.at(ic);
                ltg[itg]=lc.at(ic);
                ktg[itg]=kc.at(ic);
            }
        }
        if (bFlag) continue; // candidate belongs to existing target, hence continue
        // create new target from current candidate
        ltg.append(lc.at(ic));
        ktg.append(kc.at(ic));
        tgs.append(1);
        y2mtg.append(y2mc.at(ic));
        SNRtg.append(SNRc.at(ic));
        y2mtg_sum.append(y2mc.at(ic));
        lmtg.append(lc.at(ic));
        kmtg.append(kc.at(ic));
        ntg=ntg+1;
    }
    //---- END OF TARGET SEPARATION ALGORITHM FOR CHIRP SOUNDING PULSE ---------------------------

    // translate into physical dimensions for target formular
    bool bTargetsListed=false;
    baStrTargets.clear();   // return array of struct sTarget
    nTargets=0;             // return number of valid targets
    for (int itg=0; itg<ntg; itg++) {
        if (tgs.at(itg)>m_uTargSzThresh) {
            bTargetsListed=true;
            baStrTargets.resize(baStrTargets.size()+sizeof(struct sTarget));
            struct sTarget *pStrTarget = (struct sTarget *)baStrTargets.data()+nTargets;
            nTargets++;

            // assign target structure
            double kDoppler=ktg.at(itg);
            double iDelay=ltg.at(itg);
            double dTgVel;
            if (kDoppler >= m_NFFT/2) {
                dTgVel=m_dVelCoef*(kDoppler-m_NFFT);
            }
            else {
                dTgVel=m_dVelCoef*kDoppler;
            }
            double dTgDist = iDelay*m_dDistCoef;
            //double dTgPower = dY2M;
            pStrTarget->uCandNum=tgs.at(itg);
            pStrTarget->qpf_wei=QPointF(dTgDist,dTgVel);
            pStrTarget->y2mc_sum=y2mtg_sum[itg];
            pStrTarget->qp_rep=QPoint(lmtg[itg],kmtg[itg]);
            pStrTarget->y2mc_rep=y2mtg[itg];
            pStrTarget->SNRc_rep=SNRtg[itg];
            // qDebug() << "Tg y2mc_rep: " << y2mtg[itg] << " l: " << lmtg[itg] << " k: " << kmtg[itg];
        }
    }
    // qDebug() << "At exit bTargetsListed = " << bTargetsListed;
    // qDebug() << QString("%1 %2 %3 %4").arg(nStrobNo).arg(nc).arg(ntg).arg(nTargets);
    return bTargetsListed;
}

