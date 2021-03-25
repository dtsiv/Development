#include "qproppages.h"
#include "qinisettings.h"

#include <QPixmap>
#include <QVBoxLayout>
#include <QMetaObject>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QColorSelectionButton::QColorSelectionButton(const QColor &c, QWidget *parent /*=0*/) : 
  QPushButton(parent)
        , m_qcColorSelection(c)
        , m_pcdColorDlg(NULL) {
	setContentsMargins(2,2,2,2);
	QObject::connect(this,SIGNAL(clicked()),SLOT(onClicked()));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QColorSelectionButton::~QColorSelectionButton() {
	if (m_pcdColorDlg) delete m_pcdColorDlg;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QColor& QColorSelectionButton::getSelection() {
	return m_qcColorSelection;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QColorSelectionButton::paintEvent(QPaintEvent *pe) {
	this->QPushButton::paintEvent(pe);
	QPainter painter(this);
	painter.fillRect(contentsRect(), m_qcColorSelection);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QColorSelectionButton::onClicked() {
	m_pcdColorDlg = new QColorDialog (m_qcColorSelection);
	m_pcdColorDlg->setWindowIcon(QIcon(QPixmap(":/Resources/qtdemo.ico")));
    m_pcdColorDlg->setOption(QColorDialog::ShowAlphaChannel);
	m_pcdColorDlg->open(this, SLOT(onColorSelected(QColor)));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QColorSelectionButton::onColorSelected(const QColor &color) {
	m_qcColorSelection=color;
	update();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QPropPages::QPropPages(QObject *pOwner, QWidget *parent /* =0 */)
 : QDialog(parent,Qt::Dialog)
         , m_pOwner(pOwner)
         , m_pleDBFileName(NULL)
         , m_pleRegFileName(NULL)
         , m_ppbAccept(NULL)
         , m_ppbCancel(NULL)
         , m_ppbParse(NULL)
         , m_pleFCarrier(NULL)
         , m_pleTSampl(NULL)
         , m_pcbParseWholeDir(NULL)
         , m_pcbAdaptiveGrid(NULL)
         , m_ppbarParseProgress(NULL)
         , m_pleTimerMSecs(NULL)
         , m_pleNoiseMapFName(NULL)
         , m_pcbUseNoiseMap(NULL)
         , m_ppbarGenerateNoiseMap(NULL)
         , m_ppbGenerateNoiseMap(NULL)
         , m_pcbbRejector(NULL)
         , m_pleFalseAlarmP(NULL)
         , m_pleDetectionThreshold(NULL)
         , m_pleMaxTgSize(NULL)
         , m_plbParsingMsg(NULL)
         , m_plbDBValidationErr(NULL)
         , m_pleMeasNoise(NULL)
         , m_ppbChoose(NULL)
         , m_bMayAccept(true) {

	 resize(591,326);
     setWindowTitle("Properties");
     setWindowIcon(QIcon(QPixmap(":/Resources/settings.ico")));
     QVBoxLayout *pVLayout = new QVBoxLayout;
     m_pTabWidget=new QTabWidget;
     m_pTabWidget->setStyleSheet(
       "QTabBar::tab:selected { background: lightgray; } ");

     pVLayout->addWidget(m_pTabWidget);
     QMetaObject::invokeMethod(m_pOwner,"fillTabs",Q_ARG(QObject *,this),Q_ARG(QObject *,m_pTabWidget));
     m_ppbCancel = new QPushButton("Cancel");
     m_ppbAccept = new QPushButton("Accept");
     m_ppbAccept->setDefault(true);
     QHBoxLayout *pHLayout = new QHBoxLayout;
     pHLayout->addStretch();
     pHLayout->addWidget(m_ppbCancel);
     pHLayout->addSpacing(20);
     pHLayout->addWidget(m_ppbAccept);
     pVLayout->addLayout(pHLayout);
     setLayout(pVLayout);
     QObject::connect(m_ppbAccept,SIGNAL(clicked(bool)),SLOT(onAccepted()));
     QObject::connect(m_ppbCancel,SIGNAL(clicked(bool)),SLOT(close()));
     QObject::connect(this,SIGNAL(doParse()),m_pOwner,SLOT(onParseDataFile()));
     QIniSettings &iniSettings = QIniSettings::getInstance();
     QIniSettings::STATUS_CODES scStat;
     bool bOk=false;
     iniSettings.setDefault(QPROPPAGES_ACTIVE_TAB,-1);
     int iActiveTab = iniSettings.value(QPROPPAGES_ACTIVE_TAB,scStat).toInt(&bOk);
     if (bOk && iActiveTab >= 0 && iActiveTab < m_pTabWidget->count()) m_pTabWidget->setCurrentIndex(iActiveTab);

     // extend m_pleDBFileName's SIGNAL editingFinished() to this object
     QObject::connect(m_pleDBFileName,SIGNAL(editingFinished()),SIGNAL(DBFileNameEditingFinished()));

     // connect QPropPages::signals and QIndicatorWindow::slots
     QObject::connect(this,SIGNAL(updateParseProgressBar(double)),m_pOwner,SLOT(onUpdateParseProgressBar(double)));
     QObject::connect(this,SIGNAL(setParsingMsg(QString)),m_pOwner,SLOT(onParsingMsgSet(QString)));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QPropPages::~QPropPages() {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QPropPages::onAccepted() {
    if (!m_bMayAccept) return; // prevent dialog from being Accepted

    QMetaObject::invokeMethod(m_pOwner,"propChanged",Q_ARG(QObject *,this));
    if (m_pTabWidget) {
        QIniSettings &iniSettings = QIniSettings::getInstance();
        iniSettings.setValue(QPROPPAGES_ACTIVE_TAB,m_pTabWidget->currentIndex());
    }
    done(QDialog::Accepted);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QPropPages::onDBValidationFailed() {
    emit DBValidationFailed();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QPropPages::closeEvent(QCloseEvent *event) {

    // always can close!
    event->accept();

    //if (!m_bMayAccept) { // consume the event and prevent window close
    //    qDebug() << "Blocking close()";
    //    event->ignore();
    //}
    //else {
    //    qDebug() << "Permitting close()";
    //    event->accept();
    //}
}
