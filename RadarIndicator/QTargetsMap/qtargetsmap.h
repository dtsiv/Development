#ifndef QTARGETSMAP_H
#define QTARGETSMAP_H

#include <QtGui>
#include <QtWidgets>
#include <QtSql>
#include <QMetaObject>
#include <QOpenGLWidget>

#include "qproppages.h"
#include "qformular.h"
#include "qtargetmarker.h"
#include "mapwidget.h"
#include "qvoi.h"

#define QTARGETSMAP_PROP_TAB_CAPTION    "Indicator grid"
#define QTARGETSMAP_DOC_CAPTION         "Targets map"

#define QTARGETSMAP_SCALE_D             "PixelsPerDistanceM"
#define QTARGETSMAP_SCALE_V             "PixelsPerVelMpS"
#define QTARGETSMAP_VIEW_D0             "DistanceOriginM"
#define QTARGETSMAP_VIEW_V0             "VelOriginMpS"
#define QTARGETSMAP_ADAPT_GRD_STEP      "GridStepAdapt"
#define QTARGETSMAP_GRD_STEP_D          "GridStepDistanceM"
#define QTARGETSMAP_GRD_STEP_V          "GridStepVelMpS"
#define QTARGETSMAP_TIMER_MSEC          "timerMsecs"
#define SETTINGS_KEY_FORMULARCONTENT    "FormularContent"
#define SETTINGS_KEY_SKIPDOPPLER        "SkipDoppler"

#define QTARGETSMAP_MINTICKS            5     // minimum number of major ticks along one axis
#define QTARGETSMAP_MAXTICKS            40     // maximum number of major ticks along one axis
#define QTARGETSMAP_TICKS_OVERFLOW      200    // number of major ticks that cause error message
#define QTARGETSMAP_MAXSCALED           80     // pixels per m
#define QTARGETSMAP_MAXSCALEV           200     // pixels per m/s
#define QTARGETSMAP_MINSCALED           0.1    // pixels per m
#define QTARGETSMAP_MINSCALEV           0.25   // pixels per m/s
#define QTARGETSMAP_MAXTIMERMSEC        1000   // maximum timer interval

#define QTARGETSMAP_FORMULAR_ITEM_STROBENO           0
#define QTARGETSMAP_FORMULAR_ITEM_TARMAXDB           1
#define QTARGETSMAP_FORMULAR_ITEM_TIMESEC            2
#define QTARGETSMAP_FORMULAR_ITEM_AZDEGREE           3
#define QTARGETSMAP_FORMULAR_ITEM_ELDEGREE           4
#define QTARGETSMAP_FORMULAR_ITEM_TARSIZE            5
#define QTARGETSMAP_FORMULAR_ITEM_TARSUMDB           6
#define QTARGETSMAP_FORMULAR_ITEM_IDELAY             7
#define QTARGETSMAP_FORMULAR_ITEM_TARDISTKM          8
#define QTARGETSMAP_FORMULAR_ITEM_KDOPPLER           9
#define QTARGETSMAP_FORMULAR_ITEM_TARVELMPS         10
#define QTARGETSMAP_FORMULAR_ITEM_SIGTYPE           11
#define QTARGETSMAP_FORMULAR_ITEM_TARPROBFA         12
#define QTARGETSMAP_FORMULAR_ITEM_FLTIDX            13

class QIndicatorWindow;

class QTargetsMap : public QObject {
	Q_OBJECT

public:
    struct sFormularItem {
        quint32 uRefCode;
        QString qsTitle;
        bool bEnabled;
    };

public:
    QTargetsMap(QIndicatorWindow *pOwner = 0);
	~QTargetsMap();

    Q_INVOKABLE void addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);
    void zoomMap(MapWidget *pMapWidget, bool bZoomAlongD, bool bZoomAlongV, bool bZoomIn = true);
    void zoomMap(MapWidget *pMapWidget, int iX, int iY, bool bZoomIn = true);

private slots:
    void onExceptionDialogClosed();
    void onFormularTimeout();

public:
    MapWidget *getMapInstance();
    void addTargetMarker(const struct sVoiPrimaryPointInfo &sPriPtInfo);
    void addTargetMarker(const struct sVoiFilterInfo &pFltInfo);
    void clearMarkers();
    void refresh() {emit doUpdate();}
    bool getFirstFormular(QPoint &qpPos);
    bool getFirstMarker(QPoint &qpPos);
    void convertSkipDopplerList(QMultiMap<int,int> &qmmInOut, QString &qsInOut, bool bEncode = false);

signals:
    void doUpdate();

private:
    void mapPaintEvent(MapWidget *pMapWidget, QPaintEvent *qpeEvent);
    bool drawGrid(MapWidget *pMapWidget, QPainter &painter);
    void shiftView(bool bVertical, bool bPositive);
    void shiftView(QPoint qpShift);
    void shiftFormular(QPoint qpShift);

    void resetScale();

    struct GridSafeParams {
        double dScaleD; // pixels per m
        double dScaleV; // pixels per m/s
        double dViewD0; // localtion of widget (0,0) along physical D axis (m)
        double dViewV0; // localtion of widget (0,0) along physical V axis (m/s)
        double dDGridStep; // grid step (m) along distance axis
        double dVGridStep; // grid step (m/s) along velocity axis
    } *m_pSafeParams;

    // scales over horizontal and vertical direction
    double m_dScaleD; // pixels per m
    double m_dMaxScaleD; // maximum scale over distance
    double m_dMinScaleD; // minimum scale over distance
    double m_dScaleV; // pixels per m/s
    double m_dMaxScaleV; // maximum scale over velocity
    double m_dMinScaleV; // minimum scale over velocity
    // the only reference values are coordinates of widget (0,0) along dimensional axes
    double m_dViewD0; // along distance axis (m)
    double m_dViewV0; // along velocity axis (m/s)
    // grid steps, dimensional
    double m_dVGridStep;  // m/s
    double m_dDGridStep;  // m
    QTransform m_transform; // from physical coordinates to widget coordinates
    QString m_qsLastError;
    bool m_bAdaptiveGridStep;

public:
    // last position & time
    QFormular::sMouseStillPos *m_pMouseStill;
    // time delay to show formular
    static const quint64 m_iFormularDelay;
    quint32 m_uTimerMSecs;
    QFormular* m_pMovingFormular;
    QList<sFormularItem> m_qlFormularItems;
    QString m_qsSkipDoppler;

private:
    QTimer m_qtFormular;
    quint32 m_uFilterIdxHighlighted;
    QList<QFormular*> m_qlFormulars;
    QList<QTargetMarker*> m_qlTargets;
    QMap<quint32,quint32> m_qmFltCurrMarker;
    QIndicatorWindow *m_pOwner;
    friend class MapWidget;
};



#endif // QTARGETSMAP_H
