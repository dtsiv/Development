#ifndef QPULSESAMPLINGDLG_H
#define QPULSESAMPLINGDLG_H

#include <QtCore>
#include <QMetaObject>

#include "qproppages.h"
#include "qinisettings.h"

#include "qchrprotosnc.h"
#include "qchrprotoacm.h"

class QIndicatorWindow;

// dialog for selecting time sampling of transmitted signal
class QPulseSamplingDlg : public QDialog {
    Q_OBJECT

public:
    QPulseSamplingDlg(QIndicatorWindow *pOwner, CHR::STROBE_HEADER *pStructStrobeHeader = 0, QWidget *parent = 0);
    ~QPulseSamplingDlg();

public slots:
    // void onAccepted();
    void onLegacyToggled(int iState);
    void updateTSmplg();
    void chooseSmplgFile();
    void clearSmplgFile();

signals:

    // public-visible interface controls
public:
    QGroupBox     *m_pGrBoxStrobe;
    QGroupBox     *m_pGrBoxSig;
    QLineEdit     *m_plePlsSmplFileName;
    QPushButton   *m_ppbChoose;
    QPushButton   *m_ppbClrSel;
    QPushButton   *m_ppbCancel;
    QPushButton   *m_ppbAccept;
    QCheckBox     *m_pcbLegacy;
    QCheckBox     *m_pcbForAll;
    QLineEdit     *m_pleNumPulsesInStrobe;
    QLineEdit     *m_plePulseDuration;
	QLineEdit     *m_plePulseCount;
	QLineEdit     *m_plePulseTypeInStrobe;
	QLineEdit     *m_plePulseTypeInSignalFile;
	QLineEdit     *m_plePulseLenInSignalFile;

private:
    QIndicatorWindow *m_pOwner;
    CHR::STROBE_HEADER m_structStrobeHeader;
};


#endif // QPULSESAMPLINGDLG_H
