#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QPainter>
#include <QPlainTextEdit>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QtNetwork>
#include "plot.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define FIND_HEADER1      1
#define FIND_HEADER2      2
#define CHECK_FUNWORD     3
#define CHECK_DATALEN     4
#define TAKE_DATA         5
#define CHECK_CRC         6

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonShowLocalIp_clicked();        //显示本地所有IP地址
    void on_pushButtonBindPort_clicked();           //绑定指定端口
    void slot_udpSocket_readyRead();                //UDP 数据接收函数
    void slot_udpSocket_stateChanged(QAbstractSocket::SocketState socketState); //UDP 状态变化

    void on_pushButtonClearRec_clicked();           //清除接收
    void on_pushButtonClearSend_clicked();          //清除发送
    void slot_timerUpdateLabel_timeout();           //统计数据更新定时器槽函数

    void on_checkBoxHexRec_stateChanged(int arg1);      //16进制接收
    void on_checkBoxHexSend_stateChanged(int arg1);     //16进制发送

    void on_pushButtonSingleSend_clicked();                 //tab1 单次发送按钮槽函数
    void on_checkBoxSingleSend_stateChanged(int arg1);      //tab1 定时发送选框槽函数(开启单发定时器)
    void slot_pushButtonMultiSend_n_clicked();              //tab2 数字标号单次发送按钮槽函数
    void on_checkBoxMultiSend_stateChanged(int arg1);       //tab2 定时发送选框槽函数(开启多发定时器)
    void slot_timerMultiSend_timeout();                     //tab2 多发定时器槽函数

    void on_checkBoxFrameData_stateChanged(int arg1);       //tab3 显示Frame数据
    void on_checkBoxPlotData_stateChanged(int arg1);        //tab3 显示Plot数据
    void on_pushButtonClearFramePlotData_clicked();         //清除Frame和Plot窗口内容

    void slot_timerWaveGene_timeout();                      //tab4 仿真波形定时器
    void on_checkBoxWaveGeneStart_stateChanged(int arg1);   //tab4 仿真波形开始

    void on_actionPlotShow_triggered();                     //显示Plot窗口

private:
    void changeEncodeStrAndHex(QPlainTextEdit *plainTextEdit,int arg1); //切换字符编码与16进制
    void processRecvProtocol(QByteArray *baRecvData);                   //对串口接收数据进行协议处理

private:
    Ui::MainWindow *ui;

    //-----------------------
    // UDP
    QUdpSocket *udpSocket;
    QHostAddress peerUdpAddr;
    quint16 peerUdpPort;

    //-----------------------
    //发送接收数量，速率计算
    long curSendNum=0;
    long curRecvNum=0;
    long lastSendNum=0;
    long lastRecvNum=0;
    long sendRate=0;
    long recvRate=0;
    //接收帧数量、速率、CRC错误
    long curRecvFrameNum=0;
    long lastRecvFrameNum=0;
    long recvFrameRate=0;
    long recvFrameCrcErrNum=0;
    long recvFrameMissNum=0;

    //-----------------------
    // 定时发送-定时器
    QTimer *timerSingleSend;    //定时调用on_pushButtonSingleSend_clicked()
    QTimer *timerMultiSend;     //定时轮训发送选中的数据
    // Label数据更新-定时器
    QTimer *timerUpdateLabel;
    // 仿真波形生成-定时器
    QTimer *timerWaveGene;

    //-----------------------
    QByteArray baRecvDataBuf;   //接收数据流暂存
    int STATE=FIND_HEADER1;     //接受数据处理状态机
    char funWord=0;             //协议-功能字
    char dataLen=0;             //协议-有效数据长度
    char dataLenCnt=0;          //协议-有效数据长度计数
    char crc=0;                 //协议-校验

    bool showFramData=false;    //是否显示帧数据
    bool showPlotData=false;    //是否显示有效数据

    //-----------------------
    // 波形绘图窗口
    Plot *plot = NULL;// 必须初始化为空，否则后面NEW判断的时候会异常结束

    //-----------------------
    //CSV file to save data
    QFile *m_csvFile = nullptr;
    QTextStream *m_csvFileTextStream = nullptr;
    void openCsvFile(void);
    void closeCsvFile(void);
    void saveCsvFile(QByteArray baRecvData);
};
#endif // MAINWINDOW_H
