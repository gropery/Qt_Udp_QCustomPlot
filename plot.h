#ifndef PLOT_H
#define PLOT_H

#include <QMainWindow>
#include <QTimer>
#include "qcustomplot.h"

#define CUSTOM_LINE_COLORS   14     //曲线颜色
#define GCP_CUSTOM_LINE_COLORS 4    //界面GUI配色

namespace Ui {
class Plot;
}

class Plot : public QMainWindow
{
    Q_OBJECT

public:
    explicit Plot(QWidget *parent = nullptr);
    ~Plot();

private slots:
    // plot设置
    void slot_plot_mouseMove(QMouseEvent*);
    void slot_plot_selectionChangedByUser();
    void slot_plot_legendDoubleClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*);
    void slot_timerUpdatePlotr_timeout();

    // 坐标轴设置
    void slot_QCPXAxis_rangeChanged(QCPRange xNewRange);
    void slot_QCPYAxis_rangeChanged(QCPRange yNewRange);
    void on_checkBoxXTrackAixs_stateChanged(int arg1);
    void on_checkBoxYAutoScale_stateChanged(int arg1);
    void on_pushButtonYAutoScale_clicked();
    void on_spinBoxXCurPos_valueChanged(int arg1);
    void on_spinBoxXPoints_valueChanged(int arg1);
    void on_spinBoxYMin_valueChanged(int arg1);
    void on_spinBoxYMax_valueChanged(int arg1);
    void on_spinBoxXTicks_valueChanged(int arg1);
    void on_spinBoxYTicks_valueChanged(int arg1);
    void on_horizontalScrollBar_valueChanged(int value);

    // 缩放拖拽设置
    void on_radioButtonRangeZoomX_toggled(bool checked);
    void on_radioButtonRangeZoomY_toggled(bool checked);
    void on_radioButtonRangeZoomXY_toggled(bool checked);
    void on_radioButtonRangeDragX_toggled(bool checked);
    void on_radioButtonRangeDragY_toggled(bool checked);
    void on_radioButtonRangeDragXY_toggled(bool checked);

    // 显示控制
    void on_checkBoxShowLegend_stateChanged(int arg1);
    void on_pushButtonShowAllCurve_clicked();
    void on_pushButtonClearAllCurve_clicked();
    void on_pushButtonStartPlot_clicked();
    void on_pushButtonSavePng_clicked();

    // 曲线列表参数设置
    void on_listWidgetChannels_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidgetChannels_currentRowChanged(int currentRow);
    void on_checkBoxCurveVisible_stateChanged(int arg1);
    void on_checkBoxCurveBold_stateChanged(int arg1);
    void on_pushButtonCurveColor_clicked();
    void on_comboBoxCurveLineStyle_currentIndexChanged(int index);
    void on_comboBoxCurveScatterStyle_currentIndexChanged(int index);

private:
    Ui::Plot *ui;

    /* Line colors */
    QColor line_colors[CUSTOM_LINE_COLORS];
    QColor gui_colors[GCP_CUSTOM_LINE_COLORS];

    /* Main info */
    bool isPlotting;             //判断是否处于绘图进行中
    bool isTrackAixs;            //判断是否跟踪X轴滚动显示
    bool isYAutoScale;           //判断是否自适应Y轴数值
    qint64 dataPointNumber;      //X轴总计数据点
    int channelNumber;           //通道数


    QTimer timerUpdatePlot;     //定时刷新plot，与数据填写分开

    void setupPlot();           //初始化Plot相关配置

public:
    void onNewDataArrived(QByteArray baRecvData);  //plot数据填写，与数据刷新分开
};

#endif // PLOT_H
