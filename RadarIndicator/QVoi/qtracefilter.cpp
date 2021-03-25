#include "qtracefilter.h"
#include <QtCore>

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "nr.h"
using namespace std;

#include "cmatrix"
typedef techsoft::matrix<double> Matrix;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTraceFilter::QTraceFilter(const QCoreTraceFilter &other)
        : QCoreTraceFilter(other)
        , pPriPtNULL(NULL)
        , m_bTrackingOn(false) {
//    static bool bInit=false;
//    if (!bInit) {
//        qDebug() << "Init: QTraceFilter::QTraceFilter(const QCoreTraceFilter &other)";
//        bInit=true;
//        this->spearman();
//        qDebug() << "Init: spearman ok";
//    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTraceFilter::~QTraceFilter() {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QTraceFilter::sPrimaryPt::sPrimaryPt(QCoreTraceFilter::sPrimaryPt &other)
        : QCoreTraceFilter::sPrimaryPt(other) {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QTraceFilter::spearman() {
    const int NDAT=20,NMON=12;
    int i,j;
    char temp[17];
    string txt,txt2,city[NDAT],mon[NMON];
    DP d,probd,probrs,rs,zd;
    Vec_DP data1(NDAT),data2(NDAT);
    Mat_DP rays(NDAT,NMON);
    ifstream fp("C:/PROGRAMS/RadarIndicator/NRSources/table2.dat");
    ofstream fout("C:/PROGRAMS/RadarIndicator/NRSources/output.txt");

    if (fp.fail()) {
        NR::nrerror("Data file table2.dat not found");
    }
    getline(fp,txt);
    getline(fp,txt2);
    fp >> txt;
    for (i=0;i<NMON;i++) fp >> mon[i];
    getline(fp,txt);
    getline(fp,txt);
    for (i=0;i<NDAT;i++) {
        fp.get(temp,sizeof(temp));
        city[i]=temp;
        for (j=0;j<NMON;j++) fp >> rays[i][j];
        getline(fp,txt);
    }
    fp.close();
    fout << endl << setw(16) << " ";
    for (i=0;i<12;i++) fout << setw(5) << mon[i];
    fout << endl << endl;
    for (i=0;i<NDAT;i++) {
        fout << city[i];
        for (j=0;j<12;j++) {
            fout << setw(5) << int(0.5+rays[i][j]);
        }
        fout << endl;
    }
    fout << "press return to continue ..." << endl;
    // cin.get();
    // Check temperature correlations between different months
    fout << endl << "Are sunny summer places also sunny winter places?";
    fout << endl << "Check correlation of sampled U.S. solar radiation";
    fout << endl << "(july with other months)" << endl << endl;
    fout << "month" << setw(10) << "d" << setw(15) << "st. dev.";
    fout << setw(12) << "probd" << setw(16) << "spearman-r";
    fout << setw(11) << "probrs" << endl << endl;
    for (i=0;i<NDAT;i++) data1[i]=rays[i][0];
    for (j=0;j<12;j++) {
        for (i=0;i<NDAT;i++) data2[i]=rays[i][j];
        NR::spear(data1,data2,d,zd,probd,rs,probrs);
        fout << fixed << setprecision(2);
        fout << setw(4) << mon[j] << setw(13) << d;
        fout << fixed << setprecision(6);
        fout << setw(13) << zd << setw(13) << probd << setw(14) << rs;
        fout << setw(13) << probrs << endl;
    }
    return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QTraceFilter::inversion() {

    qDebug() << "QTraceFilter::inversion()";

        int j,k,l,m,n;
        string dummy;
        ifstream fp("C:/PROGRAMS/NumericalRecipes/CPP_211/data/matrx1.dat");

        ofstream fout("C:/PROGRAMS/RadarIndicator/NRSources/aaa.txt");

        if (fp.fail())
          NR::nrerror("Data file matrx1.dat not found");
        fout << fixed << setprecision(6);
        getline(fp,dummy);
        while (!fp.eof()) {
          getline(fp,dummy);
          fp >> n >> m;
          fp.get();
          getline(fp,dummy);
          Mat_DP a(n,n),u(n,n),b(n,m),t(n,m);
          for (k=0;k<n;k++)
            for (l=0;l<n;l++) fp >> a[k][l];
          fp.get();
          getline(fp,dummy);
          for (l=0;l<m;l++)
            for (k=0;k<n;k++) fp >> b[k][l];
          fp.get();
          getline(fp,dummy);
          // save matrices for later testing of results
          Mat_DP ai=a;
          Mat_DP x=b;
          // invert matrix
          NR::gaussj(ai,x);
          fout << endl << "Inverse of matrix a : " << endl;
          for (k=0;k<n;k++) {
            for (l=0;l<n;l++) fout << setw(12) << ai[k][l];
            fout << endl;
          }
          // check inverse
          fout << endl << "a times a-inverse:" << endl;
          for (k=0;k<n;k++) {
            for (l=0;l<n;l++) {
              u[k][l]=0.0;
              for (j=0;j<n;j++)
                u[k][l] += (a[k][j]*ai[j][l]);
            }
            for (l=0;l<n;l++) fout << setw(12) << u[k][l];
            fout << endl;
          }
          // check vector solutions
          fout << endl << "Check the following for equality:" << endl;
          fout << setw(21) << "original" << setw(15) << "matrix*sol'n" << endl;
          for (l=0;l<m;l++) {
            fout << "vector " << l << ": " << endl;
            for (k=0;k<n;k++) {
              t[k][l]=0.0;
              for (j=0;j<n;j++)
                t[k][l] += (a[k][j]*x[j][l]);
              fout << "        " << setw(13) << b[k][l];
              fout << setw(13) << t[k][l] << endl;
            }
          }
          fout << "***********************************" << endl;
          fout << "press RETURN for next problem:" << endl;
          // cin.get();
        }
        fp.close();
        return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double QTraceFilter::twoSidedStudentSignificance(double dThreshold, int iDegreesOfFreedomN) {
    return 1.0e0-NR::betai(0.5*iDegreesOfFreedomN,0.5,
                     iDegreesOfFreedomN/(iDegreesOfFreedomN+dThreshold*dThreshold));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//void QTraceFilter::print() {
//    QListIterator<int> i(m_qlPriPts);
//    while (i.hasNext()) {
//        int iIdx=i.next();
//        qDebug() << "QTraceFilter::print(): iIdx=" << iIdx;
//        sPrimaryPt *pPriPt = dynamic_cast<struct QTraceFilter::sPrimaryPt*>(m_qlAllPriPts.value(iIdx,pPriPtNULL));
//        if (pPriPt) {
//            QTraceFilter *pFlt=dynamic_cast<QTraceFilter *>(pPriPt->pFlt);
//            if (!pFlt) {
//                qDebug() << "QTraceFilter::print() - dynamic cast failed #2";
//                return; // not valid list index or failed dynamic_cast
//            }
//            qDebug() << (pFlt==this)
//                     << " R=" << pPriPt->dR
//                     << " ElDeg=" << pPriPt->dElRad
//                     << " AzDeg=" << pPriPt->dAzRad;
//        }
//        else {
//            qDebug() << "QTraceFilter::print(): m_qlAllPriPts.size()=" << m_qlAllPriPts.size() << " iIdx=" << iIdx;
//        }
//    }
//    // ttest();
//}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//void NR::ttest(Vec_I_DP &data1, Vec_I_DP &data2, DP &t, DP &prob) {
//	DP var1,var2,svar,df,ave1,ave2;
//
//	int n1=data1.size();
//	int n2=data2.size();
//	avevar(data1,ave1,var1);
//	avevar(data2,ave2,var2);
//	df=n1+n2-2;
//	svar=((n1-1)*var1+(n2-1)*var2)/df;
//	t=(ave1-ave2)/sqrt(svar*(1.0/n1+1.0/n2));
//	prob=betai(0.5*df,0.5,df/(df+t*t));
//}
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//void QTraceFilter::ttest() {
//    ofstream fout("C:/PROGRAMS/aaa.txt");
//    int iMax=4,Nmax=10;
//    double dPmin=0.5,dPStep=0.1;
//    // Generate gaussian distributed data
//    fout << setw(6) << "iDeg";
//    for (int i=0; i<=iMax; i++) {
//        fout << setw(9) << setprecision(2)
//             << dPmin+i*dPStep;
//    }
//    fout << endl;
//    for (int N=1;N<=Nmax;N++) {
//        fout << setw(6) << N;
//        for (int i=0; i<=iMax; i++) {
//            double dProb=dPmin+i*dPStep;
//            double dThreshold=-1.0;
//            for (double dThres=0.5; dThres<10; dThres+=0.01) {
//                if (twoSidedStudentSignificance(dThres,N)>dProb) {
//                    dThreshold=dThres;
//                    break;
//                }
//            }
//            fout << setw(9) << setprecision(4)
//                 << dThreshold;
//        }
//        fout << endl;
//    }
//    return;
//}
