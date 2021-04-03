#include "qformular.h"
#include "qtargetsmap.h"

const quint64 QFormular::m_uFormularLifetime = 30000;
const quint64 QFormular::m_uFormularCutoffPix = 10;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QFormular::QFormular(struct sMouseStillPos *pMouseStill,
                     QObject *parent /* = 0 */ )
               : QObject(parent)
               , m_pTargetMarker(NULL)
               , m_bStale(false)
               , m_qpOffset(QPoint(30,30)) {
    Q_ASSERT(pMouseStill != 0);
    m_mouseStill = *pMouseStill;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QFormular::isStale() {
    if (m_bStale) return true;
    return (m_mouseStill.last.msecsTo(QTime::currentTime()) > m_uFormularLifetime);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QFormular::setStale() {
    // ensure this formular is no more used
    m_bStale = true;
    // m_pTargetMarker: object was destroyed
    m_pTargetMarker = NULL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QFormular::selectTarg(const QList<QTargetMarker*> &qlTargets, QTransform &t) {
    QList<double> qlDists;
    QList<double> qlIndxs;
    // if currently no targets detected - then skip
    if (!qlTargets.size()) return false;
    // calculate all dists (in pixels) from targets to (m_dMouseDPhys,m_dMouseVPhys)
    for (int i=0; i<qlTargets.size(); i++) {
        QPointF qpTar = qlTargets.at(i)->tar();
        QPoint qpTarPix = (qpTar*t).toPoint();
        qpTarPix -= m_mouseStill.pos;
        qlDists << qpTarPix.manhattanLength();
        qlIndxs << i;
    }
    // sort the distances
    for (int i=0; i<qlTargets.size()-1; i++) {
        for (int j=i+1; j<qlTargets.size(); j++) {
            if (qlDists.at(i) > qlDists.at(j)) {
                qlDists.swap(i,j);
                qlIndxs.swap(i,j);
            }
        }
    }
    // smallest distance is too many pixels - skip
    for (int i=0; i<qlDists.size(); i++) {
        if (qlDists.at(i) > m_uFormularCutoffPix) return false;
        QTargetMarker *pTargetMarker = qlTargets.at(qlIndxs.at(i));
        if (pTargetMarker->hasFormular()) continue;
        pTargetMarker->setFormular(this);
        m_pTargetMarker = pTargetMarker;
        return true;
    }
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QFormular::contains(QPoint qp) {
    return m_qrFormularRect.contains(qp);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QFormular::drawFormular(QPainter &painter, QTransform &t) {
    // if formular is stale then m_pTargetMarker no longer exists
    if (m_bStale) return;
    // if no target marker was found close to mouse pointer - then skip
    if (!m_pTargetMarker) return;

    // save painter state
    painter.save();

    // Position of target in pixels (view coordinates)
    QPoint qpTarPix = (m_pTargetMarker->tar() * t).toPoint();
    // target outside widget area - skip
    QRect qrBounding=painter.window();
    if (!qrBounding.contains(qpTarPix)) {
        painter.restore(); return;
    }

    // Draw the formular
    // Background color
    QColor qcFormularBg(Qt::green);
    qcFormularBg.setAlpha(192);
    QBrush qbFormularBrush(qcFormularBg);
    // Border color
    QBrush qbFormularBorder(Qt::darkGray,Qt::SolidPattern);
    int iFormularThickness=2;
    QPen qpFormularPen(qbFormularBorder,iFormularThickness,Qt::SolidLine,Qt::RoundCap);
    // font metrics
    int iLegendSize = 14;
    QFont font=QApplication::font();
    font.setWeight(QFont::Normal);
    font.setPointSize(iLegendSize);
    painter.setFont(font);
    QFontMetrics fmTickLabel(font,painter.device());
    // formular rectangle
    QString qsMesg = m_pTargetMarker->mesgString();
    QRect qrOuter(0,0,100,100);
    int iFlags=0;
    QRect qrTxt = fmTickLabel.boundingRect(qrOuter,iFlags,qsMesg);
    QRectF qrFormuRect = QRectF(0.0, 0.0, qrTxt.width()+6, qrTxt.height());
    qrFormuRect.moveTo(m_qpOffset + qpTarPix);
    if (qrFormuRect.top() < 0.) qrFormuRect.translate(0.,-qrFormuRect.top());
    if (qrFormuRect.left() < 0.) qrFormuRect.translate(-qrFormuRect.left(),0.);
    if (qrFormuRect.bottom() > qrBounding.height()) qrFormuRect.translate(QPointF(0.,qrBounding.height()-qrFormuRect.bottom()));
    if (qrFormuRect.right() > qrBounding.width()) qrFormuRect.translate(QPointF(qrBounding.width()-qrFormuRect.right(),0.));
    QPointF pt0(qrFormuRect.left(),qrFormuRect.top());
    double d0 = _hypot(qpTarPix.x()-pt0.x(),qpTarPix.y()-pt0.y());
    QPointF pt1(qrFormuRect.right(),qrFormuRect.top());
    double d1 = _hypot(qpTarPix.x()-pt1.x(),qpTarPix.y()-pt1.y());
    QPointF pt2(qrFormuRect.right(),qrFormuRect.bottom());
    double d2 = _hypot(qpTarPix.x()-pt2.x(),qpTarPix.y()-pt2.y());
    QPointF pt3(qrFormuRect.left(),qrFormuRect.bottom());
    double d3 = _hypot(qpTarPix.x()-pt3.x(),qpTarPix.y()-pt3.y());
    if (d1 < d0) {
        d0 = d1; pt0 = pt1;
    }
    if (d2 < d0) {
        d0 = d2; pt0 = pt2;
    }
    if (d3 < d0) {
        d0 = d3; pt0 = pt3;
    }
    painter.drawLine(qpTarPix,pt0);
    // QPointF qpTxt(qrFormuRect.left(),qrFormuRect.top());
    // qpTxt += QPointF(3.0,qrTxt.height()-3.0);
    // start actual grawing
    painter.setPen(qpFormularPen);
    painter.setBrush(qbFormularBrush);
    painter.drawRect(qrFormuRect);
    m_qrFormularRect.setRect(qrFormuRect.x(),qrFormuRect.y(),qrFormuRect.width(),qrFormuRect.height());
    QPointF qpTopLeft = qrFormuRect.topLeft();
    qpTopLeft.rx()+=3.0e0;
    qpTopLeft.ry()-=3.0e0;
    qrFormuRect.moveTopLeft(qpTopLeft);
    QBrush qbFormularTextBrush(Qt::white,Qt::SolidPattern);
    QPen qpFormularTextPen(qbFormularTextBrush,iFormularThickness,Qt::SolidLine,Qt::RoundCap);
    painter.setPen(qpFormularTextPen);
    painter.drawText(qrFormuRect,Qt::AlignLeft,qsMesg);
    // restore painter state
    painter.restore();
}
