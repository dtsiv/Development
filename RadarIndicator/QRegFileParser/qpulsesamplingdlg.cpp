#include "qpulsesamplingdlg.h"
#include "qindicatorwindow.h"
#include "qexceptiondialog.h"

#include <windows.h>
#include <stdlib.h>

//====================================================================
//
//====================================================================
QPulseSamplingDlg::QPulseSamplingDlg(QIndicatorWindow *pOwner, CHR::STROBE_HEADER *pStructStrobeHeader /* = 0 */, QWidget *parent /* = 0 */ )
            : QDialog(parent,Qt::Dialog)
        , m_pOwner(pOwner)
        , m_pGrBoxStrobe(NULL)
        , m_pGrBoxSig(NULL)
        , m_plePlsSmplFileName(NULL)
        , m_pleNumPulsesInStrobe(NULL)
        , m_plePulseDuration(NULL)
        , m_plePulseCount(NULL)
        , m_plePulseTypeInStrobe(NULL)
        , m_plePulseTypeInSignalFile(NULL)
        , m_plePulseLenInSignalFile(NULL)
        , m_ppbChoose(NULL)
        , m_ppbClrSel(NULL)
        , m_ppbCancel(NULL)
        , m_ppbAccept(NULL)
        , m_pcbLegacy(NULL)
        , m_pcbForAll(NULL) {

    setWindowIcon(QIcon(QPixmap(":/Resources/pulse.ico")));
    setWindowTitle("Select file with time sampling of transmitted signal");
    if (pStructStrobeHeader) {
        m_structStrobeHeader = *pStructStrobeHeader;
    }

    // dialog widget layout
    QVBoxLayout *pVLayout = new QVBoxLayout;

    // parameters table layout for strobe
    QGridLayout *pGLayout01=new QGridLayout;
    pGLayout01->addWidget(new QLabel("Number of pulses in strobe"),0,0);
    m_pleNumPulsesInStrobe = new QLineEdit(QString::number(m_structStrobeHeader.pCount,10));
    m_pleNumPulsesInStrobe->setMaximumWidth(PROP_LINE_WIDTH);
    m_pleNumPulsesInStrobe->setReadOnly(true);
    pGLayout01->addWidget(m_pleNumPulsesInStrobe,0,1);

    pGLayout01->addWidget(new QLabel("Pulse duration (samples)"),0,3);
    m_plePulseDuration = new QLineEdit(QString::number(m_structStrobeHeader.pDuration,10));
    m_plePulseDuration->setMaximumWidth(PROP_LINE_WIDTH);
    m_plePulseDuration->setReadOnly(true);
    pGLayout01->addWidget(m_plePulseDuration,0,4);

    pGLayout01->addWidget(new QLabel("Samples per period"),1,0);
    m_pleNumPulsesInStrobe = new QLineEdit(QString::number(m_structStrobeHeader.pPeriod,10));
    m_pleNumPulsesInStrobe->setMaximumWidth(PROP_LINE_WIDTH);
    m_pleNumPulsesInStrobe->setReadOnly(true);
    pGLayout01->addWidget(m_pleNumPulsesInStrobe,1,1);

    pGLayout01->addWidget(new QLabel("Samples recorded"),1,3);
    m_plePulseDuration = new QLineEdit(QString::number(m_structStrobeHeader.distance,10));
    m_plePulseDuration->setMaximumWidth(PROP_LINE_WIDTH);
    m_plePulseDuration->setReadOnly(true);
    pGLayout01->addWidget(m_plePulseDuration,1,4);

    pGLayout01->addWidget(new QLabel("Modulation type"),2,0);
    m_plePulseTypeInStrobe = new QLineEdit(QString::number(m_structStrobeHeader.signalID,10));
    m_plePulseTypeInStrobe->setMaximumWidth(PROP_LINE_WIDTH);
    m_plePulseTypeInStrobe->setReadOnly(true);
    pGLayout01->addWidget(m_plePulseTypeInStrobe,2,1);

    pGLayout01->setColumnStretch(2,100);
    pGLayout01->setColumnStretch(5,100);

    // arrange group box "New strobe parameters"
    m_pGrBoxStrobe = new QGroupBox("New strobe parameters");
    m_pGrBoxStrobe->setLayout(pGLayout01);
    pVLayout->addWidget(m_pGrBoxStrobe);

    // group box layout for strobe
    QVBoxLayout *pVLayout02 = new QVBoxLayout;

    // file lineeid & buttons layout
    QHBoxLayout *pHLayout02 = new QHBoxLayout;
    m_plePlsSmplFileName = new QLineEdit;
    QObject::connect(m_plePlsSmplFileName,SIGNAL(editingFinished()),this,SLOT(updateTSmplg()));
    m_ppbChoose=new QPushButton(QIcon(":/Resources/open.ico"),"Choose");
    QObject::connect(m_ppbChoose,SIGNAL(clicked()),this,SLOT(chooseSmplgFile()));
    m_ppbClrSel=new QPushButton(QIcon(":/Resources/broom.ico"),"Clear");
    QObject::connect(m_ppbClrSel,SIGNAL(clicked()),this,SLOT(clearSmplgFile()));
    pHLayout02->addWidget(m_plePlsSmplFileName);
    pHLayout02->addWidget(m_ppbChoose);
    pHLayout02->addWidget(m_ppbClrSel);
    pVLayout02->addLayout(pHLayout02);

    // parameters table layout for signal
    QGridLayout *pGLayout02=new QGridLayout;
    pGLayout02->addWidget(new QLabel("Transmitted siganl type"),3,0);
    m_plePulseTypeInSignalFile = new QLineEdit(QString::number(0,10));
    m_plePulseTypeInSignalFile->setMaximumWidth(PROP_LINE_WIDTH);
    m_plePulseTypeInSignalFile->setReadOnly(true);
    pGLayout02->addWidget(m_plePulseTypeInSignalFile,3,1);

    pGLayout02->addWidget(new QLabel("Transmitted siganl lenth"),3,3);
    m_plePulseLenInSignalFile = new QLineEdit(QString::number(0,10));
    m_plePulseLenInSignalFile->setMaximumWidth(PROP_LINE_WIDTH);
    m_plePulseLenInSignalFile->setReadOnly(true);
    pGLayout02->addWidget(m_plePulseLenInSignalFile,3,4);

    pGLayout02->setColumnStretch(2,100);
    pGLayout02->setColumnStretch(5,100);

    pVLayout02->addLayout(pGLayout02);

    pVLayout02->addWidget(new QLabel("place signal plot here"));

    // arrange group box "New strobe parameters"
    m_pGrBoxSig = new QGroupBox("Time sampling of the transmitted signal");
    m_pGrBoxSig->setLayout(pVLayout02);
    pVLayout->addWidget(m_pGrBoxSig);

    m_pcbLegacy=new QCheckBox("Legacy");
    m_pcbLegacy->setEnabled(false);
    m_pcbForAll=new QCheckBox("For all");
    m_pcbForAll->setEnabled(false);
    m_ppbCancel=new QPushButton("Cancel");
    m_ppbAccept=new QPushButton("Accept");
    QObject::connect(m_pcbLegacy,SIGNAL(stateChanged(int)),this,SLOT(onLegacyToggled(int)));
    QObject::connect(m_ppbCancel,SIGNAL(clicked()),this,SLOT(reject()));
    QObject::connect(m_ppbAccept,SIGNAL(clicked()),this,SLOT(accept()));
    QObject::connect(m_plePlsSmplFileName,SIGNAL(editingFinished()),this,SLOT(updateTSmplg()));
    m_ppbAccept->setDefault(true);
    QHBoxLayout *pHLayout03 = new QHBoxLayout;
    pHLayout03->addWidget(m_pcbLegacy);
    pHLayout03->addWidget(m_pcbForAll);
    pHLayout03->addWidget(m_ppbCancel);
    pHLayout03->addWidget(m_ppbAccept);
    pVLayout->addLayout(pHLayout03);
    // set widget layout
    setLayout(pVLayout);
    // set active focus
    QTimer::singleShot(0,m_ppbChoose,SLOT(setFocus()));
}

//====================================================================
//
//====================================================================
QPulseSamplingDlg::~QPulseSamplingDlg() {
}

//====================================================================
//
//====================================================================
void QPulseSamplingDlg::onLegacyToggled(int iState) {
    if (iState==Qt::Unchecked) {
        m_pGrBoxStrobe->setEnabled(true);
        m_pGrBoxSig->setEnabled(true);
        return;
    }
    m_pGrBoxStrobe->setEnabled(false);
    m_pGrBoxSig->setEnabled(false);
}

//====================================================================
//
//====================================================================
void QPulseSamplingDlg::updateTSmplg() {
    m_ppbAccept->setEnabled(false);
    // QPoi *pPoi = m_pOwner->m_pPoi;
    // if (!pPoi) return; // redundant check
    QRegFileParser *pRegFileParser = m_pOwner->m_pRegFileParser;
    if (!pRegFileParser) return; // redundant check
    QString qsSigFileName = m_plePlsSmplFileName->text();
    QFile qfSigFile(qsSigFileName);
    // qDebug() << "parse file " << qsSigFileName;
    if (!qfSigFile.exists()) return;
    QByteArray baSigFileData,baSignalFileText; // dummy
    int iSignalID;
    int iSignalLen;
    // qDebug() << "pRegFileParser->parseSignalFile()";
    if (!pRegFileParser->parseSignalFile(qsSigFileName,baSigFileData,
                                         iSignalID,iSignalLen,
                                         baSignalFileText)) {
        qDebug() << "pRegFileParser->parseSignalFile: " << qsSigFileName;
        return;
    }
    m_plePulseTypeInSignalFile->setText(QString::number(iSignalID));
    m_plePulseLenInSignalFile->setText(QString::number(iSignalLen));
    if (   iSignalID == m_structStrobeHeader.signalID
        && iSignalLen == m_structStrobeHeader.pDuration) {
        m_ppbAccept->setEnabled(true);
        // qDebug() << "pRegFileParser->parseSignalFile() ok";
    }
}

//====================================================================
//
//====================================================================
void QPulseSamplingDlg::clearSmplgFile() {
    m_plePlsSmplFileName->clear();
}

//====================================================================
//
//====================================================================
void QPulseSamplingDlg::chooseSmplgFile() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Time sample file (*.txt)"));
    // if time sampling file name was set - copy it here
    QString qsCurrFile = m_plePlsSmplFileName->text();
    if (!qsCurrFile.isEmpty()) {
        QFileInfo fiCurrFile(qsCurrFile);
        if (fiCurrFile.absoluteDir().exists()) {
            dialog.setDirectory(fiCurrFile.absoluteDir());
        }
        else { // otherwise set current directory in dialog
            dialog.setDirectory(QDir::current());
        }
    }
    else { // otherwise set current directory in dialog
        QSqlModel *pSqlModel = m_pOwner->m_pSqlModel;
        if (pSqlModel) { // redundant check
            QFileInfo fiCurrentDB(m_pOwner->m_pSqlModel->getDBFileName());
            if (fiCurrentDB.exists() && fiCurrentDB.isFile()) {
                dialog.setDirectory(fiCurrentDB.absoluteDir());
            }
            else {
                dialog.setDirectory(QDir::current());
            }
        }
    }
    if (dialog.exec()) {
        QStringList qsSelection=dialog.selectedFiles();
        if (qsSelection.size() != 1) return;
        QString qsSelFilePath=qsSelection.at(0);
        QFileInfo fiSelFilePath(qsSelFilePath);
        if (fiSelFilePath.isFile() && fiSelFilePath.isReadable())	{
            QDir qdCur = QDir::current();
            QDir qdSelFilePath = fiSelFilePath.absoluteDir();
            if (qdCur.rootPath() == qdSelFilePath.rootPath()) { // same Windows drives
                m_plePlsSmplFileName->setText(qdCur.relativeFilePath(fiSelFilePath.absoluteFilePath()));
            }
            else { // different Windows drives
                m_plePlsSmplFileName->setText(fiSelFilePath.absoluteFilePath());
            }
            updateTSmplg();
        }
        else {
            m_plePlsSmplFileName->setText("error");
        }
    }
}
