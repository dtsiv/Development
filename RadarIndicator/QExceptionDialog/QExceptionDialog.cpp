#include "qexceptiondialog.h"

#include <QDialog>
#include <QIcon>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
void showExceptionDialog(QString exceptionWhat, QWidget *parent /* = 0 */) {
    QExceptionDialog w(exceptionWhat, parent);
    w.exec();
}
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
QExceptionDialog::QExceptionDialog(QString exceptionWhat, QWidget *parent) :
       QDialog(parent),
       m_pgsPlane(0) {
        // Set QExceptionDialog window flags
    Qt::WindowFlags flags = windowFlags() | Qt::WindowStaysOnTopHint
    #ifdef __linux
        | Qt::X11BypassWindowManagerHint
    #endif
        | Qt::Dialog;
    setWindowFlags(flags);
    resize(769,600);
    setWindowTitle("Ooops... This is embarassing!");
    // This dialog is a separate window. It needs its icon
    QPixmap qpmSpear(":/Resources/spear.ico");
    setWindowIcon(QIcon(qpmSpear));
    QPixmap qpmPlane(":/Resources/plane_small.png");
    m_pgsPlane = new QGraphicsScene(qpmPlane.rect());
    m_pgsPlane->addPixmap(qpmPlane);
    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setSpacing(6);
    pVLayout->setMargin(11);
    QGraphicsView *pGraphicsView = new QGraphicsView(m_pgsPlane);
    pGraphicsView->setMinimumSize(750,446);
    pGraphicsView->setFrameShape(QFrame::NoFrame);
    pGraphicsView->setFrameShadow(QFrame::Plain);
    pGraphicsView->setLineWidth(0);
    pGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pVLayout->addWidget(pGraphicsView);
    QPlainTextEdit *pteWhat = new QPlainTextEdit(exceptionWhat);
    QFont qfMesg;
    qfMesg.setPointSize(14);
    pteWhat->setFont(qfMesg);
    pteWhat->setAcceptDrops(false);
    pteWhat->setFrameShape(QFrame::StyledPanel);
    pteWhat->setFrameShadow(QFrame::Sunken);
    pteWhat->setReadOnly(true);
    pteWhat->setBackgroundVisible(false);

    pVLayout->addWidget(pteWhat);
    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->addStretch();
    QPushButton *ppbDismiss = new QPushButton("Dismiss");
    QObject::connect(ppbDismiss,SIGNAL(clicked()),SLOT(accept()));
    pHLayout->addWidget(ppbDismiss);
    pVLayout->addLayout(pHLayout);
    setLayout(pVLayout);

    QDesktopWidget *desktop = QApplication::desktop();
    QRect qrScrGeo = desktop->screenGeometry(desktop->primaryScreen());
    QRect rectDialog = this->frameGeometry();
    move((qrScrGeo.width()-rectDialog.width())/2,(qrScrGeo.height()-rectDialog.height())/2);
}
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
QExceptionDialog::~QExceptionDialog() {
    delete m_pgsPlane;
}
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
RmoException::RmoException(QString str) _GLIBCXX_USE_NOEXCEPT {
    m_qsMsg = QString("RmoException: ")+str;
}

// copy constructor
//RmoException::RmoException(const RmoException& _Right) _GLIBCXX_USE_NOEXCEPT
//		: m_qsMsg(_Right.m_qsMsg) { } // construct by copying _Right

// assignment operator
//RmoException& RmoException::operator=(const RmoException& _Right) _GLIBCXX_USE_NOEXCEPT {
//    // assign _Right
//	m_qsMsg = _Right.m_qsMsg;
//	return (*this);
//}

// virtual keyword in base class destructor is important to invoke
// child class destructor for pointer or reference variable
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
RmoException::~RmoException() _GLIBCXX_USE_NOEXCEPT {}

//---------------------------------------------------------------------------------
// it is important to specify const modifier to enable polymorphic what()
//---------------------------------------------------------------------------------
const char * RmoException::what() const _GLIBCXX_USE_NOEXCEPT {
    m_baLocalEnc=m_qsMsg.toLocal8Bit().data();
    return m_baLocalEnc.data();
}
