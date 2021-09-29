#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Qt Udp Plot");

    connect(ui->pushButtonMultiSend_1,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_2,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_3,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_4,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_5,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_6,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_7,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_8,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_9,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));
    connect(ui->pushButtonMultiSend_10,SIGNAL(clicked(bool)),this,SLOT(slot_pushButtonMultiSend_n_clicked()));

    // 单次发送选项卡-定时发送-定时器
    timerSingleSend = new QTimer;
    timerSingleSend->setInterval(1000);// 设置默认定时时长1000ms
    connect(timerSingleSend, &QTimer::timeout, this, [=](){
        on_pushButtonSingleSend_clicked();
    });

    // 多次发送选项卡-定时发送-定时器
    timerMultiSend = new QTimer;
    timerMultiSend->setInterval(1000);// 设置默认定时时长1000ms
    connect(timerMultiSend, &QTimer::timeout, this, [=](){
        slot_timerMultiSend_timeout();
    });

    // Label数据更新-定时器
    timerUpdateLabel = new QTimer;
    timerUpdateLabel->start(1000);
    connect(timerUpdateLabel, &QTimer::timeout, this, [=](){
        slot_timerUpdateLabel_timeout();
    });

    // 模拟波形串口输出定时器
    timerWaveGene = new QTimer;
    timerWaveGene->setInterval(20);
    connect(timerWaveGene, &QTimer::timeout, this, [=](){
        slot_timerWaveGene_timeout();
    });

    // 新建UDP socket
    udpSocket = new QUdpSocket(this);
    connect(udpSocket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(slot_udpSocket_stateChanged(QAbstractSocket::SocketState)));
    slot_udpSocket_stateChanged(udpSocket->state());    //更新Socket状态
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(slot_udpSocket_readyRead()));

    // 新建波形显示界面
    plot = new Plot;
    plot->show();

    qDebug()<<"start..."<<endl;
}

// 显示本地所有IPv4地址
void MainWindow::on_pushButtonShowLocalIp_clicked()
{
    QList<QNetworkInterface> list=QNetworkInterface::allInterfaces();
    for(int i=0;i<list.count();i++)
    {
        QNetworkInterface aInterface=list.at(i);
        if (!aInterface.isValid())
           continue;

        ui->plainTextEditRec->appendPlainText("设备名称："+aInterface.humanReadableName());
        ui->plainTextEditRec->appendPlainText("硬件地址："+aInterface.hardwareAddress());
        QList<QNetworkAddressEntry> entryList=aInterface.addressEntries();
        for(int j=0;j<entryList.count();j++)
        {
            QNetworkAddressEntry aEntry=entryList.at(j);
            if(!(aEntry.ip().protocol() == QAbstractSocket::IPv4Protocol))  //只打印IPv4
                continue;

            ui->plainTextEditRec->appendPlainText("   IP 地址："+aEntry.ip().toString());
            ui->plainTextEditRec->appendPlainText("   子网掩码："+aEntry.netmask().toString());
            ui->plainTextEditRec->appendPlainText("   广播地址："+aEntry.broadcast().toString()+"\n");
        }
        ui->plainTextEditRec->appendPlainText("\n");
    }
}

// UDP绑定端口
void MainWindow::on_pushButtonBindPort_clicked()
{
    if(ui->pushButtonBindPort->isChecked()){
        quint16 port=ui->spinBoxLocalPort->value(); //本机UDP端口

        if (udpSocket->bind(port)){
            ui->pushButtonBindPort->setChecked(true);
            ui->spinBoxLocalPort->setEnabled(false);
            ui->pushButtonBindPort->setText("解除绑定");

            if(ui->actionSaveCsv->isChecked()){
                //以当前日期时间戳创建CSV文件
                openCsvFile();
            }
            //串口使用过程中锁定保存按钮不可用
            ui->actionSaveCsv->setEnabled(false);
        }
    }
    else{
        udpSocket->abort();
        ui->pushButtonBindPort->setChecked(false);
        ui->spinBoxLocalPort->setEnabled(true);
        ui->pushButtonBindPort->setText("绑定端口");

        if(ui->actionSaveCsv->isChecked()){
            //关闭文件
            closeCsvFile();
        }
        ui->actionSaveCsv->setEnabled(true);
    }
}

//UDP 状态变化
void MainWindow::slot_udpSocket_stateChanged(QAbstractSocket::SocketState socketState)
{
    switch(socketState)
    {
        case QAbstractSocket::UnconnectedState:
            ui->labelSocketState->setText("scoket状态：UnconnectedState");
            break;
        case QAbstractSocket::HostLookupState:
            ui->labelSocketState->setText("scoket状态：HostLookupState");
            break;
        case QAbstractSocket::ConnectingState:
            ui->labelSocketState->setText("scoket状态：ConnectingState");
            break;
        case QAbstractSocket::ConnectedState:
            ui->labelSocketState->setText("scoket状态：ConnectedState");
            break;
        case QAbstractSocket::BoundState:
            ui->labelSocketState->setText("scoket状态：BoundState");
            break;
        case QAbstractSocket::ClosingState:
            ui->labelSocketState->setText("scoket状态：ClosingState");
            break;
        case QAbstractSocket::ListeningState:
            ui->labelSocketState->setText("scoket状态：ListeningState");
    }
}

MainWindow::~MainWindow()
{
    closeCsvFile();
    udpSocket->abort();
    delete udpSocket;
    delete plot;
    delete ui;
}


//清除接收窗口
void MainWindow::on_pushButtonClearRec_clicked()
{
    //(1)清空plainTextEditRec内容
    ui->plainTextEditRec->clear();
    //(2)清空字节计数
    curSendNum = 0;
    curRecvNum = 0;
    lastSendNum = 0;
    lastRecvNum = 0;
    curRecvFrameNum = 0;
    lastRecvFrameNum = 0;
    recvFrameCrcErrNum = 0;
    recvFrameMissNum = 0;
}

//清除发送窗口
void MainWindow::on_pushButtonClearSend_clicked()
{
    //(1)清空plainTextEditSend内容
    ui->plainTextEditSend->clear();
    //(2)清空发送字节计数
    curSendNum = 0;
    lastSendNum = 0;
}

//定时器槽函数timeout，1s 数据更新
void MainWindow::slot_timerUpdateLabel_timeout(void)
{
    //当前总计数-上次总结存暂存
    sendRate = curSendNum - lastSendNum;
    recvRate = curRecvNum - lastRecvNum;
    recvFrameRate = curRecvFrameNum - lastRecvFrameNum;
    //设置label
    ui->labelSendRate->setText(tr("Byte/s: %1").arg(sendRate));
    ui->labelRecvRate->setText(tr("Byte/s: %1").arg(recvRate));
    ui->labelRecvFrameRate->setText(tr("FPS: %1").arg(recvFrameRate));
    //暂存当前总计数
    lastSendNum = curSendNum;
    lastRecvNum = curRecvNum;
    lastRecvFrameNum = curRecvFrameNum;
    //更新label
    ui->labelSendNum->setText(tr("S: %1").arg(curSendNum));
    ui->labelRecvNum->setText(tr("R: %1").arg(curRecvNum));
    ui->labelRecvFrameNum->setText(tr("FNum: %1").arg(curRecvFrameNum));

    ui->labelRecvFrameErrNum->setText(tr("FErr: %1").arg(recvFrameCrcErrNum));
    ui->labelRecvFrameMissNum->setText(tr("FMiss: %1").arg(recvFrameMissNum));
}

//16进制发送选框，切换发送框内容为编码字符或16进制字节
void MainWindow::on_checkBoxHexSend_stateChanged(int arg1)
{
    changeEncodeStrAndHex(ui->plainTextEditSend, arg1);
}

//16进制接收选框，切换接收框内容为编码字符或16进制字节
void MainWindow::on_checkBoxHexRec_stateChanged(int arg1)
{
    changeEncodeStrAndHex(ui->plainTextEditRec, arg1);
}

//切换内容为编码字符或16进制字节
void MainWindow::changeEncodeStrAndHex(QPlainTextEdit *plainTextEdit, int arg1)
{
    QString strRecvData = plainTextEdit->toPlainText();
    QByteArray ba;
    QString str;

    // 获取多选框状态，未选为0，选中为2
    if(arg1 == Qt::Unchecked){
        // 多选框未勾选，接收区先前接收的16进制数据转换为asc2字符串格式
        //ba = QByteArray::fromHex(strRecvData1.toUtf8());               //Unicode(UTF8)编码，
        //str = QString(ba);//QString::fromUtf8(ba);                     //这里bytearray转为string时也可直接赋值，或者QString构造，因为QString存储的UTF-16编码
        ba = QByteArray::fromHex(strRecvData.toLocal8Bit());             //ANSI(GB2132)编码
        str =QString::fromLocal8Bit(ba);                                 //这里bytearray转为string一定要指定编码
    }
    else{
        // 多选框勾选，接收区先前接收asc2字符串转换为16进制显示
        //QByteArray ba = strRecvData1.toUtf8().toHex(' ').toUpper();    // Unicode(UTF8)编码(中国：E4 B8 AD E5 9B BD)
        ba = strRecvData.toLocal8Bit().toHex(' ').toUpper();             // ANSI(GB2132)编码(中国:D6 D0 B9 FA)
        str = QString(ba).append(' ');                                   //这里由16进制的bytearray转为string所有编码统一，所以可以直接构造，并没有歧义
    }

    // 文本控件清屏，显示新文本
    plainTextEdit->clear();
    plainTextEdit->insertPlainText(str);
    // 移动光标到文本结尾
    plainTextEdit->moveCursor(QTextCursor::End);
}

//tab1 单次发送按钮槽函数
void MainWindow::on_pushButtonSingleSend_clicked()
{
    QString strSendData = ui->plainTextEditSend->toPlainText();
    QByteArray ba;
    if(ui->checkBoxHexSend->checkState() == Qt::Unchecked){
        // 字符串形式发送
        //ba = strSendData.toUtf8();                            // Unicode编码输出
        ba = strSendData.toLocal8Bit();                         // GB2312编码输出,用以兼容大多数单片机
    }else{
        // 16进制发送
        //ba = QByteArray::fromHex(strSendData.toUtf8());       // Unicode编码输出
        ba = QByteArray::fromHex(strSendData.toLocal8Bit());    // GB2312编码输出
    }

    QString     targetIP=ui->comboBoxTargetIP->currentText(); //目标IP
    QHostAddress    targetAddr(targetIP);
    quint16     targetPort=ui->spinBoxTargetPort->value();    //目标port

    // 如发送成功，会返回发送的字节长度。失败，返回-1。
    qint64 ret = udpSocket->writeDatagram(ba,targetAddr,targetPort);      //发出数据报

    if(ret > 0)
        curSendNum += ret;
}

//tab1 定时发送选框槽函数(开启单发定时器)
void MainWindow::on_checkBoxSingleSend_stateChanged(int arg1)
{
    int TimerInterval = ui->lineEditSingleSend->text().toInt();

    if(arg1 == Qt::Unchecked){
        timerSingleSend->stop();
        ui->lineEditSingleSend->setEnabled(true);
    }else{
        // 对输入的值做限幅，小于20ms会弹出对话框提示
        if(TimerInterval >= 20){
            timerSingleSend->start(TimerInterval);    // 设置定时时长
            ui->lineEditSingleSend->setEnabled(false);
        }else{
            ui->checkBoxSingleSend->setCheckState(Qt::Unchecked);
            QMessageBox::critical(this, "错误提示", "定时发送的最小间隔为 20ms\r\n请确保输入的值 >=20");
        }
    }
}

//tab2 数字标号单次发送按钮槽函数
void MainWindow::slot_pushButtonMultiSend_n_clicked()
{
    //获取信号发送者
    QPushButton *pushButton = qobject_cast<QPushButton *>(sender());
    QString strSendData;
    QByteArray ba;

    if(pushButton == ui->pushButtonMultiSend_1)
        strSendData = ui->lineEditMultiSend_1->text();
    if(pushButton == ui->pushButtonMultiSend_2)
        strSendData = ui->lineEditMultiSend_2->text();
    if(pushButton == ui->pushButtonMultiSend_3)
        strSendData = ui->lineEditMultiSend_3->text();
    if(pushButton == ui->pushButtonMultiSend_4)
        strSendData = ui->lineEditMultiSend_4->text();
    if(pushButton == ui->pushButtonMultiSend_5)
        strSendData = ui->lineEditMultiSend_5->text();
    if(pushButton == ui->pushButtonMultiSend_6)
        strSendData = ui->lineEditMultiSend_6->text();
    if(pushButton == ui->pushButtonMultiSend_7)
        strSendData = ui->lineEditMultiSend_7->text();
    if(pushButton == ui->pushButtonMultiSend_8)
        strSendData = ui->lineEditMultiSend_8->text();
    if(pushButton == ui->pushButtonMultiSend_9)
        strSendData = ui->lineEditMultiSend_9->text();
    if(pushButton == ui->pushButtonMultiSend_10)
        strSendData = ui->lineEditMultiSend_10->text();

    // 只16进制发送
    ba = QByteArray::fromHex(strSendData.toLocal8Bit());    // GB2312编码输出

    QString     targetIP=ui->comboBoxTargetIP->currentText(); //目标IP
    QHostAddress    targetAddr(targetIP);
    quint16     targetPort=ui->spinBoxTargetPort->value();    //目标port

    // 如发送成功，会返回发送的字节长度。失败，返回-1。
    qint64 ret = udpSocket->writeDatagram(ba,targetAddr,targetPort);      //发出数据报

    if(ret > 0)
        curSendNum += ret;
}

//tab2 定时发送选框槽函数(开启多发定时器)
void MainWindow::on_checkBoxMultiSend_stateChanged(int arg1)
{
    int TimerInterval = ui->lineEditMultiSend->text().toInt();

    if(arg1 == Qt::Unchecked){
        timerMultiSend->stop();
        ui->lineEditMultiSend->setEnabled(true);
    }else{
        // 对输入的值做限幅，小于20ms会弹出对话框提示
        if(TimerInterval >= 20){
            timerMultiSend->start(TimerInterval);    // 设置定时时长
            ui->lineEditMultiSend->setEnabled(false);
        }else{
            ui->checkBoxSingleSend->setCheckState(Qt::Unchecked);
            QMessageBox::critical(this, "错误提示", "定时发送的最小间隔为 20ms\r\n请确保输入的值 >=20");
        }
    }
}

//tab2 轮训多发定时器
void MainWindow::slot_timerMultiSend_timeout()
{
    QString strSendData;
    QByteArray ba;

    if(ui->checkBoxMultiSend_1->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_1->text();
    if(ui->checkBoxMultiSend_2->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_2->text();
    if(ui->checkBoxMultiSend_3->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_3->text();
    if(ui->checkBoxMultiSend_4->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_4->text();
    if(ui->checkBoxMultiSend_5->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_5->text();
    if(ui->checkBoxMultiSend_6->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_6->text();
    if(ui->checkBoxMultiSend_7->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_7->text();
    if(ui->checkBoxMultiSend_8->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_8->text();
    if(ui->checkBoxMultiSend_9->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_9->text();
    if(ui->checkBoxMultiSend_10->checkState()==Qt::Checked)
        strSendData += ui->lineEditMultiSend_10->text();

    // 只16进制发送
    ba = QByteArray::fromHex(strSendData.toLocal8Bit());    // GB2312编码输出

    QString     targetIP=ui->comboBoxTargetIP->currentText(); //目标IP
    QHostAddress    targetAddr(targetIP);
    quint16     targetPort=ui->spinBoxTargetPort->value();    //目标port

    // 如发送成功，会返回发送的字节长度。失败，返回-1。
    qint64 ret = udpSocket->writeDatagram(ba,targetAddr,targetPort);      //发出数据报

    if(ret > 0)
        curSendNum += ret;
}

//数据接收
void MainWindow::slot_udpSocket_readyRead(void)
{
    QByteArray baRecvData;

    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray   datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),datagram.size(),&peerUdpAddr,&peerUdpPort);
        baRecvData += datagram;
    }

    QString str;
    QByteArray ba;

    //调用私有成员函数解析数据协议
    processRecvProtocol(&baRecvData);

    //已接收字节统计
    int baRecvDataSize = baRecvData.size();
    curRecvNum += baRecvDataSize;

    //是否显示接收内容
    if(ui->checkBoxStopShow->checkState() == Qt::Unchecked)
    {
        //以字符或16进制显示
        if(ui->checkBoxHexRec->checkState() == Qt::Unchecked){
            // 字符编码显示
            //str = QString::fromUtf8(baRecvData);      // Unicode编码
            str = QString::fromLocal8Bit(baRecvData);   // GB2312编码输入
        }else{
            // 16进制显示，用空格分隔，转换为大写
            ba = baRecvData.toHex(' ').toUpper();
            str = QString(ba).append(' ');
        }
        // 在当前位置插入文本，不会发生换行。(如果使用appendPlainText，则会自动换行)
        ui->plainTextEditRec->insertPlainText(str);
        // 移动光标到文本结尾.使得文本超出当前界面显示范围时，滚动轴自动向下滚动，以显示最新文本
        ui->plainTextEditRec->moveCursor(QTextCursor::End);

        // 判断多长的数据没有换行符，如果超过2000，会人为向数据接收区添加换行，来保证CPU占用率不会过高，不会导致卡顿
        // 但由于是先插入换行，后插入接收到的数据，所以每一箩数据并不是2000
        static int cnt=0;
        if(baRecvData.contains('\n')){
            cnt=0;
        }
        else{
            cnt += baRecvDataSize;
            if(cnt > 2000){
                ui->plainTextEditRec->appendPlainText(""); //添加一个回车换行
                cnt=0;
            }
        }
    }
}

//对接收数据流进行协议解析
void MainWindow::processRecvProtocol(QByteArray *baRecvData)
{
    int num = baRecvData->size();

    if(showFramData){
        QString str1 = QString(baRecvData->toHex(' ').toUpper());
        ui->plainTextEditFrameData->appendPlainText(str1);    //自动换行
    }

    if(num) {
        for(int i=0; i<num; i++){
            switch(STATE) {
            //查找第一个帧头
            case FIND_HEADER1:
                if(baRecvData->at(i) == 0x3A){
                    STATE = FIND_HEADER2;
                }
                break;
            //查找第二个帧头
            case FIND_HEADER2:
                if(baRecvData->at(i) == 0x3B){
                    //找到第二个帧头
                    STATE = CHECK_FUNWORD;
                }
                else if(baRecvData->at(i) == 0x3A){
                    //避免漏掉3A 3A 3B xx情况
                    STATE = FIND_HEADER2;
                }
                else
                    STATE = FIND_HEADER1;
                break;
            //检查功能字是否合规
            case CHECK_FUNWORD:
                if(baRecvData->at(i) == 0x01){
                    funWord = baRecvData->at(i);
                    STATE = CHECK_DATALEN;
                }
                else{
                    funWord = 0;
                    STATE = FIND_HEADER1;
                }
                break;
            //判断数据长度是否合规
            case CHECK_DATALEN:
                if(baRecvData->at(i) <= 32*2){
                    //限制最大接收长度64字节，即32个ch
                    dataLen = baRecvData->at(i);
                    STATE = TAKE_DATA;
                }
                else{
                    dataLen = 0;
                    STATE = FIND_HEADER1;
                }
                break;
            //接收所有数据
            case TAKE_DATA:
                if(dataLenCnt < dataLen){
                    baRecvDataBuf.append(baRecvData->at(i));
                    dataLenCnt++;
                    //满足条件后立刻转入下个状态
                    if(dataLenCnt >= dataLen){
                        dataLenCnt = 0;
                        STATE = CHECK_CRC;
                    }
                }
                break;
            //CRC校验
            case CHECK_CRC:
                crc = 0x3A + 0X3B + funWord + dataLen;
                for(int c=0; c<dataLen; c++){
                    crc += baRecvDataBuf[c];
                }

                //校验通过，数据有效
                if(crc == baRecvData->at(i)){
                    if(showPlotData){
                        QString str2 = QString(baRecvDataBuf.toHex(' ').toUpper());
                        ui->plainTextEditPlotData->appendPlainText(str2);    //自动换行
                    }
                    plot->onNewDataArrived(baRecvDataBuf);
                    saveCsvFile(baRecvDataBuf);

                    curRecvFrameNum++; //有效帧数量计数
                }
                else{
                    recvFrameCrcErrNum++; //误码帧数量计数
                }

                //清除暂存数据
                baRecvDataBuf.clear();
                funWord=0;
                dataLen=0;
                dataLenCnt=0;
                crc=0;
                STATE = FIND_HEADER1;
                break;


            default:
                STATE = FIND_HEADER1;
                break;
            }
        }
    }
}
//-----------------------

void MainWindow::on_checkBoxFrameData_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    showFramData = ui->checkBoxFrameData->isChecked();
}

void MainWindow::on_checkBoxPlotData_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    showPlotData = ui->checkBoxPlotData->isChecked();
}

void MainWindow::on_pushButtonClearFramePlotData_clicked()
{
    curRecvFrameNum = 0;
    lastRecvFrameNum=0;
    recvFrameCrcErrNum = 0;
    recvFrameMissNum = 0;
    ui->plainTextEditFrameData->clear();
    ui->plainTextEditPlotData->clear();
}

// 显示绘图窗口
void MainWindow::on_actionPlotShow_triggered()
{
    plot->show();
}

void MainWindow::slot_timerWaveGene_timeout()
{
    static float x;
    char y1,y2;
    char crc;

    x += 0.01;
    y1 = sin(x)*50;
    y2 = cos(x)*100;
    crc = 0x3A+0x3B+0x01+0x02+y1+y2;

    QByteArray ba;
    ba.append(0x3A).append(0x3B).append(0x01).append(0x02).append(y1).append(y2).append(crc);

    QString     targetIP=ui->comboBoxTargetIP->currentText(); //目标IP
    QHostAddress    targetAddr(targetIP);
    quint16     targetPort=ui->spinBoxTargetPort->value();    //目标port

    // 如发送成功，会返回发送的字节长度。失败，返回-1。
    qint64 ret = udpSocket->writeDatagram(ba,targetAddr,targetPort);      //发出数据报

    if(ret > 0)
        curSendNum += ret;
}


void MainWindow::on_checkBoxWaveGeneStart_stateChanged(int arg1)
{
    int TimerInterval = ui->lineEditWaveGeneInterval->text().toInt();

    if(arg1 == Qt::Checked){
        ui->lineEditWaveGeneInterval->setEnabled(false);
        timerWaveGene->start(TimerInterval);
    }

    else if(arg1 == Qt::Unchecked){
        timerWaveGene->stop();
        ui->lineEditWaveGeneInterval->setEnabled(true);
    }
}

//打开文件
void MainWindow::openCsvFile(void)
{
    m_csvFile = new QFile(QDateTime::currentDateTime().toString("yyyy-MM-d-HH-mm-ss-")+"data-out.csv");
    if(!m_csvFile)
      return;
    if (!m_csvFile->open(QIODevice::ReadWrite | QIODevice::Text))
      return;
    m_csvFileTextStream = new QTextStream(m_csvFile);
}

//关闭文件
void MainWindow::closeCsvFile(void)
{
    if(!m_csvFile)
        return;

    m_csvFile->close();

    if(m_csvFile)
        delete m_csvFile;
    if(m_csvFileTextStream)
        delete m_csvFileTextStream;
    m_csvFile = nullptr;
    m_csvFileTextStream = nullptr;
}

//保存CRC正确数据至文件
void MainWindow::saveCsvFile(QByteArray baRecvData)
{
    if(!m_csvFile)
        return;
    if(ui->actionSaveCsv->isChecked())
    {
        QByteArray ba = baRecvData.toHex(' ').toUpper();
        QString str(ba);
        *m_csvFileTextStream << str;
        *m_csvFileTextStream << "\n";
    }
}


