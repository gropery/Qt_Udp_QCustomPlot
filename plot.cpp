#include "plot.h"
#include "ui_plot.h"

Plot::Plot(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Plot),
    //曲线颜色初始化 14种
    line_colors{
      /* For channel data (gruvbox palette) */
      /* Light */
      QColor ("#fb4934"),
      QColor ("#b8bb26"),
      QColor ("#fabd2f"),
      QColor ("#83a598"),
      QColor ("#d3869b"),
      QColor ("#8ec07c"),
      QColor ("#fe8019"),
      /* Light */
      QColor ("#cc241d"),
      QColor ("#98971a"),
      QColor ("#d79921"),
      QColor ("#458588"),
      QColor ("#b16286"),
      QColor ("#689d6a"),
      QColor ("#d65d0e"),
    },
    //界面GUI配色初始化 4种
    gui_colors {
      QColor (48,  47,  47,  255), /**<  0: qdark ui dark/background color */
      QColor (80,  80,  80,  255), /**<  1: qdark ui medium/grid color */
      QColor (170, 170, 170, 255), /**<  2: qdark ui light/text color */
      QColor (48,  47,  47,  200)  /**<  3: qdark ui dark/background color w/transparency */
    },
    //参数初始化
    isPlotting(false),
    isTrackAixs(true),
    isYAutoScale(false),
    dataPointNumber (0),
    channelNumber(0)
{
    ui->setupUi(this);
    setWindowTitle("Plot");

    //初始化Plot相关配置
    setupPlot();

    //连接槽函数-实现状态栏显示鼠标的xy坐标
    connect (ui->plot, SIGNAL (mouseMove (QMouseEvent*)), this, SLOT (slot_plot_mouseMove(QMouseEvent*)));
    //连接槽函数-实现plot中曲线可选择
    connect (ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(slot_plot_selectionChangedByUser()));
    //连接槽函数-实现双击legend可修改名称
    connect (ui->plot, SIGNAL(legendDoubleClick (QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)), this, SLOT(slot_plot_legendDoubleClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));
    //连接槽函数-实现定时replot，将数据填充和图标刷新分开
    connect (&timerUpdatePlot, SIGNAL (timeout()), this, SLOT (slot_timerUpdatePlotr_timeout()));
    //连接槽函数-实现坐标轴变化值同步至spinBox和scrollBar
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_QCPXAxis_rangeChanged(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_QCPYAxis_rangeChanged(QCPRange)));
}

Plot::~Plot()
{
    delete ui;
}

//------------------------------------------------------------------
//初始化Plot相关配置
void Plot::setupPlot()
{
    //清除图标中所有项目
    ui->plot->clearItems();

    //设置背景颜色
    ui->plot->setBackground (gui_colors[0]);

    //用于高性能模式，去除抗锯齿， (see QCustomPlot real time example)
    ui->plot->setNotAntialiasedElements (QCP::aeAll);
    QFont font;
    font.setStyleStrategy (QFont::NoAntialias);
    ui->plot->legend->setFont (font);

    //See QCustomPlot examples / styled demo
    /* X Axis: Style */
    ui->plot->xAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible (true);
    ui->plot->xAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->xAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->xAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->xAxis->setTickLabelFont (font);
    // 范围
    ui->plot->xAxis->setRange (dataPointNumber - ui->spinBoxXPoints->value(), dataPointNumber);
    ui->spinBoxXCurPos->setEnabled(false);
    // 设置坐标轴名称
    ui->plot->xAxis->setLabel("X");

    /* Y Axis */
    ui->plot->yAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridVisible (true);
    ui->plot->yAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->yAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->yAxis->setTickLabelFont (font);
    // 范围
    ui->plot->yAxis->setRange (ui->spinBoxYMin->value(), ui->spinBoxYMax->value());
    // 设置坐标轴名称
    ui->plot->yAxis->setLabel("Y");

    // 图表交互
    ui->plot->setInteraction (QCP::iRangeDrag, true);           //可单击拖拽
    ui->plot->setInteraction (QCP::iRangeZoom, true);           //可滚轮缩放
    ui->plot->setInteraction (QCP::iSelectPlottables, true);    //图表内容可选择
    ui->plot->setInteraction (QCP::iSelectLegend, true);        //图例可选
    ui->plot->axisRect()->setRangeDrag (Qt::Horizontal|Qt::Vertical);  //水平拖拽
    ui->plot->axisRect()->setRangeZoom (Qt::Vertical);        //水平缩放

    // 图例设置
    QFont legendFont;
    legendFont.setPointSize (9);
    ui->plot->legend->setVisible (true);
    ui->plot->legend->setFont (legendFont);
    ui->plot->legend->setBrush (gui_colors[3]);
    ui->plot->legend->setBorderPen (gui_colors[2]);
    // 图例位置，右上角
    ui->plot->axisRect()->insetLayout()->setInsetAlignment (0, Qt::AlignTop|Qt::AlignRight);
}

//------------------------------------------------------------------
// plot数据填写，与数据刷新分开
// 数据宽度8bit
void Plot::onNewDataArrived(QByteArray baRecvData)
{
    static int num = 0;
    static int channelIndex = 0;

    if (isPlotting){
        num = baRecvData.size();

        for (int i = 0; i < num; i++){
            if(ui->plot->plottableCount() <= channelIndex){
                /* Add new channel data */
                ui->plot->addGraph();
                ui->plot->graph()->setPen (line_colors[channelNumber % CUSTOM_LINE_COLORS]);
                ui->plot->graph()->setName (QString("Channel %1").arg(channelNumber));
                if(ui->plot->legend->item(channelNumber)){
                    ui->plot->legend->item (channelNumber)->setTextColor (line_colors[channelNumber % CUSTOM_LINE_COLORS]);
                }
                ui->listWidgetChannels->addItem(ui->plot->graph()->name());
                ui->listWidgetChannels->item(channelIndex)->setForeground(QBrush(line_colors[channelNumber % CUSTOM_LINE_COLORS]));
                channelNumber++;
            }

            /* Add data to Graph */
            ui->plot->graph(channelIndex)->addData (dataPointNumber, baRecvData[channelIndex]);
            /* Increment data number and channel */
            channelIndex++;
        }

        dataPointNumber++;
        channelIndex = 0;
      }
}

//------------------------------------------------------------------
//状态栏显示鼠标的xy坐标
void Plot::slot_plot_mouseMove(QMouseEvent *event)
{
    int xPos = int(ui->plot->xAxis->pixelToCoord(event->x()));
    int yPos = int(ui->plot->yAxis->pixelToCoord(event->y()));
    QString coordinates = tr("X: %1 Y: %2").arg(xPos).arg(yPos);
    ui->statusBar->showMessage(coordinates);
}

//------------------------------------------------------------------
//plot中曲线和图例可选择
void Plot::slot_plot_selectionChangedByUser (void)
{
    /* synchronize selection of graphs with selection of corresponding legend items */
     for (int i = 0; i < ui->plot->graphCount(); i++)
       {
         QCPGraph *graph = ui->plot->graph(i);
         QCPPlottableLegendItem *item = ui->plot->legend->itemWithPlottable (graph);
         if (item->selected())
           {
             item->setSelected (true);
   //          graph->set (true);
           }
         else
           {
             item->setSelected (false);
     //        graph->setSelected (false);
           }
       }
}

//------------------------------------------------------------------
// 双击legend中项目，重命名其曲线名称
void Plot::slot_plot_legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED (legend)
    Q_UNUSED(event)
    /* Only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0) */
    if (item)
      {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText (this, "Change channel name", "New name:", QLineEdit::Normal, plItem->plottable()->name(), &ok, Qt::Popup);
        if (ok)
          {
            plItem->plottable()->setName(newName);
            for(int i=0; i<ui->plot->graphCount(); i++)
            {
                ui->listWidgetChannels->item(i)->setText(ui->plot->graph(i)->name());
            }
            ui->plot->replot(QCustomPlot::rpQueuedReplot);
          }
      }
}

//------------------------------------------------------------------
// 定时replot图表，将数据填写和刷新图表分开
void Plot::slot_timerUpdatePlotr_timeout()
{
    if(isTrackAixs){
        ui->plot->xAxis->setRange (dataPointNumber - ui->spinBoxXPoints->value(), dataPointNumber);
    }

    if(isYAutoScale)
        on_pushButtonYAutoScale_clicked();

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 同步X轴的范围变化至spinBox和scrollBar
void Plot::slot_QCPXAxis_rangeChanged(QCPRange xNewRange)
{
    ui->spinBoxXCurPos->setValue(xNewRange.lower);
    ui->spinBoxXPoints->setValue(xNewRange.upper - xNewRange.lower);

    ui->horizontalScrollBar->setRange(0, dataPointNumber);
    ui->horizontalScrollBar->setPageStep(xNewRange.upper - xNewRange.lower);
    ui->horizontalScrollBar->setValue(xNewRange.lower);
}

//------------------------------------------------------------------
// 同步Y轴的范围变化至spinBox
void Plot::slot_QCPYAxis_rangeChanged(QCPRange yNewRange)
{
    ui->spinBoxYMax->setValue(yNewRange.upper);
    ui->spinBoxYMin->setValue(yNewRange.lower);
}

//------------------------------------------------------------------
// 是否跟踪X轴滚动显示
void Plot::on_checkBoxXTrackAixs_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        isTrackAixs = true;
        ui->spinBoxXCurPos->setEnabled(false);
    }
    else{
        isTrackAixs = false;
        ui->spinBoxXCurPos->setEnabled(true);
    }
}

//------------------------------------------------------------------
// 是否自适应Y轴数值
void Plot::on_checkBoxYAutoScale_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        isYAutoScale = true;
        ui->spinBoxYMin->setEnabled(false);
        ui->spinBoxYMax->setEnabled(false);
    }
    else{
        isYAutoScale = false;
        ui->spinBoxYMin->setEnabled(true);
        ui->spinBoxYMax->setEnabled(true);
    }
}

//------------------------------------------------------------------
// 自适应Y轴数值1次
void Plot::on_pushButtonYAutoScale_clicked()
{
    ui->plot->yAxis->rescale(true);
    int newYMax = int(ui->plot->yAxis->range().upper*1.1);
    int newYMin = int(ui->plot->yAxis->range().lower*1.1);
    ui->plot->yAxis->setRange(newYMin,newYMax);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXCurPos值变化同步至x轴最小值
void Plot::on_spinBoxXCurPos_valueChanged(int arg1)
{
    double newXMin = arg1;
    double newXNumber = ui->plot->xAxis->range().upper - ui->plot->xAxis->range().lower;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXPoints值变化同步至x轴窗口显示点数
void Plot::on_spinBoxXPoints_valueChanged(int arg1)
{
    double newXMin = ui->plot->xAxis->range().lower;
    double newXNumber = arg1;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYMin值变化同步至Y轴最小值
void Plot::on_spinBoxYMin_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeLower (arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYMax值变化同步至Y轴最大值
void Plot::on_spinBoxYMax_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeUpper (arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXTicks值变化同步至X轴tick
void Plot::on_spinBoxXTicks_valueChanged(int arg1)
{
    ui->plot->xAxis->ticker()->setTickCount(arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYTicks值变化同步至Y轴tick
void Plot::on_spinBoxYTicks_valueChanged(int arg1)
{
    ui->plot->yAxis->ticker()->setTickCount(arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 滚轮缩放、鼠标拖拽、XY轴分别设定
void Plot::on_radioButtonRangeZoomX_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Horizontal);
}

void Plot::on_radioButtonRangeZoomY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Vertical);
}

void Plot::on_radioButtonRangeZoomXY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Horizontal|Qt::Vertical);
}

void Plot::on_radioButtonRangeDragX_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Horizontal);
}

void Plot::on_radioButtonRangeDragY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Vertical);
}

void Plot::on_radioButtonRangeDragXY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Horizontal|Qt::Vertical);
}

//------------------------------------------------------------------
// 同步水平滚动轴变化值到X轴最小值
void Plot::on_horizontalScrollBar_valueChanged(int value)
{
    double newXMin = value;
    double newXNumber = ui->plot->xAxis->range().upper - ui->plot->xAxis->range().lower;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 显示图例
void Plot::on_checkBoxShowLegend_stateChanged(int arg1)
{
    if(arg1){
        ui->plot->legend->setVisible(true);
    }else{
        ui->plot->legend->setVisible(false);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 显示所有曲线（包括被隐藏曲线）
void Plot::on_pushButtonShowAllCurve_clicked()
{
    for(int i=0; i<ui->plot->graphCount(); i++)
    {
        ui->plot->graph(i)->setVisible(true);
        ui->listWidgetChannels->item(i)->setBackground(Qt::NoBrush);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 清除所有曲线（清除历史数据）
void Plot::on_pushButtonClearAllCurve_clicked()
{
    //pPlot1->graph(i)->data().data()->clear(); // 仅仅清除曲线的数据
    //pPlot1->clearGraphs();                    // 清除图表的所有数据和设置，需要重新设置才能重新绘图
    ui->plot->clearPlottables();                // 清除图表中所有曲线，需要重新添加曲线才能绘图
    ui->listWidgetChannels->clear();
    channelNumber = 0;
    dataPointNumber = 0;
    setupPlot();
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 开始接收数据并启动定时刷新定时器
void Plot::on_pushButtonStartPlot_clicked()
{
    if(ui->pushButtonStartPlot->isChecked()){
        //启动绘图
        timerUpdatePlot.start(20);
        isPlotting = true;
        ui->pushButtonStartPlot->setText("停止绘图");
    }
    else{
        //停止绘图
        timerUpdatePlot.stop();
        isPlotting = false;
        ui->pushButtonStartPlot->setText("开始绘图");
    }
}

//------------------------------------------------------------------
// 在exe目录下保存波形截图
void Plot::on_pushButtonSavePng_clicked()
{
    ui->plot->savePng (QString::number(dataPointNumber) + ".png", 1920, 1080, 2, 50);
}

//------------------------------------------------------------------
// 双击列表框中曲线隐藏/显示plot曲线（已隐藏曲线背景为黑色）
void Plot::on_listWidgetChannels_itemDoubleClicked(QListWidgetItem *item)
{
    int graphIdx = ui->listWidgetChannels->currentRow();

    if(ui->plot->graph(graphIdx)->visible()){
        ui->plot->graph(graphIdx)->setVisible(false);
        item->setBackground(Qt::black);
    }
    else{
        ui->plot->graph(graphIdx)->setVisible(true);
        item->setBackground(Qt::NoBrush);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 单击选中列表框项目，更新对应曲线信息
void Plot::on_listWidgetChannels_currentRowChanged(int currentRow)
{
    //曲线可见
    if(ui->plot->graph(currentRow)->visible())
        ui->checkBoxCurveVisible->setCheckState(Qt::Checked);
    else
        ui->checkBoxCurveVisible->setCheckState(Qt::Unchecked);

    //曲线加粗
    if(ui->plot->graph(currentRow)->pen().width() == 3)
        ui->checkBoxCurveBold->setCheckState(Qt::Checked);
    else
        ui->checkBoxCurveBold->setCheckState(Qt::Unchecked);

    //颜色设置
    //获取当前颜色
    QColor curColor = ui->plot->graph(currentRow)->pen().color();
    //设置选择框颜色
    ui->pushButtonCurveColor->setStyleSheet(QString("border:0px solid;background-color: %1;").arg(curColor.name()));

    //线形设置
    QCPGraph::LineStyle lineStyle = ui->plot->graph(currentRow)->lineStyle();
    ui->comboBoxCurveLineStyle->setCurrentIndex(int(lineStyle));

    //散点样式
    QCPScatterStyle::ScatterShape scatterShape = ui->plot->graph(currentRow)->scatterStyle().shape();
    ui->comboBoxCurveScatterStyle->setCurrentIndex(int(scatterShape));
}

//------------------------------------------------------------------
// 曲线可见/隐藏
void Plot::on_checkBoxCurveVisible_stateChanged(int arg1)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    if(arg1 == Qt::Checked){
        ui->plot->graph(graphIdx)->setVisible(true);
        ui->listWidgetChannels->item(graphIdx)->setBackground(Qt::NoBrush);
    }
    else{
        ui->plot->graph(graphIdx)->setVisible(false);
        ui->listWidgetChannels->item(graphIdx)->setBackground(Qt::black);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 曲线加粗
void Plot::on_checkBoxCurveBold_stateChanged(int arg1)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    // 预先读取曲线的颜色
    QPen pen = ui->plot->graph(graphIdx)->pen();

    if(arg1 == Qt::Checked)
        pen.setWidth(3);
    else
        pen.setWidth(1);

    ui->plot->graph(graphIdx)->setPen(pen);

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}
//------------------------------------------------------------------
// 曲线颜色更改
void Plot::on_pushButtonCurveColor_clicked()
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    // 获取当前颜色
    QColor curColor = ui->plot->graph(graphIdx)->pen().color();// 由curve曲线获得颜色
    // 以当前颜色打开调色板，父对象，标题，颜色对话框设置项（显示Alpha透明度通道）
    QColor color = QColorDialog::getColor(curColor, this,
                                     tr("颜色对话框"),
                                     QColorDialog::ShowAlphaChannel);
    // 判断返回的颜色是否合法。若点击x关闭颜色对话框，会返回QColor(Invalid)无效值，直接使用会导致变为黑色。
    if(color.isValid()){
        // 设置选择框颜色
        ui->pushButtonCurveColor->setStyleSheet(QString("border:0px solid;background-color: %1;").arg(color.name()));
        // 设置曲线颜色
        QPen pen = ui->plot->graph(graphIdx)->pen();
        pen.setBrush(color);
        ui->plot->graph(graphIdx)->setPen(pen);
        // 设置legend文本颜色
        ui->plot->legend->item (graphIdx)->setTextColor(color);
        // 设置listwidget文本颜色
        ui->listWidgetChannels->item(graphIdx)->setForeground(QBrush(color));
    }

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 曲线样式选择（点、线、左、右、中、积）
void Plot::on_comboBoxCurveLineStyle_currentIndexChanged(int index)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    ui->plot->graph(graphIdx)->setLineStyle((QCPGraph::LineStyle)index);

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// 散点样式选择（空心圆、实心圆、正三角、倒三角）
void Plot::on_comboBoxCurveScatterStyle_currentIndexChanged(int index)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    if(index <= 10){
        ui->plot->graph(graphIdx)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)index, 5)); // 散点样式
    }else{ // 后面的散点图形略复杂，太小会看不清
        ui->plot->graph(graphIdx)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)index, 8)); // 散点样式
    }

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}
