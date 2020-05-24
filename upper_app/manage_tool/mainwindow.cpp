﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "protocol.h"
#include "uartthread.h"
#include "socketclient.h"

static CUartProtocolInfo *pMainUartProtocolInfo;
static CTcpClientSocket *pCTcpClientSocket;

static PROTOCOL_STATUS protocol_flag = PROTOCOL_NULL;

uint8_t led_on_cmd[] = {
    0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x01
};
uint8_t led_off_cmd[] = {
    0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00
};
static uint8_t beep_on_cmd[] = {
    0x02, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, 0x02
};
uint8_t beep_off_cmd[] = {
    0x02, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, 0x00
};

#define FRAM_STYLE  "QFrame{border-radius:10px}"

//函数声明
void init_btn_disable(Ui::MainWindow *ui);
void init_btn_enable(Ui::MainWindow *ui);
void QFrame_Init(Ui::MainWindow *ui);

//类的实现
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFrame_Init(ui);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void QFrame_Init(Ui::MainWindow *ui)
{
    ui->centralwidget->setMinimumSize(QSize(710, 720));
    ui->frame_uart->setFrameShape(QFrame::Shape::Box);
    ui->frame_uart->setFrameShadow(QFrame::Shadow::Sunken);
    ui->frame_dev->setFrameShape(QFrame::Shape::Box);
    ui->frame_dev->setFrameShadow(QFrame::Shadow::Sunken);
    ui->frame_socket->setFrameShape(QFrame::Shape::Box);
    ui->frame_socket->setFrameShadow(QFrame::Shadow::Sunken);
}

void MainWindow::init()
{
    //添加COM口
    QStringList comList;
    for (int i = 1; i <= 20; i++) {
        comList << QString("COM%1").arg(i);
    }
    ui->combo_box_com->addItems(comList);

    //波特率选项
    QStringList BaudList;
    BaudList <<"9600"<<"38400"<<"76800"<<"115200"<<"230400";
    ui->combo_box_baud->addItems(BaudList);
    ui->combo_box_baud->setCurrentIndex(3);

    //数据位选项
    QStringList dataBitsList;
    dataBitsList <<"6" << "7" << "8"<<"9";
    ui->combo_box_data->addItems(dataBitsList);
    ui->combo_box_data->setCurrentIndex(2);

    //停止位选项
    QStringList StopBitsList;
    StopBitsList<<"1"<<"2";
    ui->combo_box_stop->addItems(StopBitsList);
    ui->combo_box_stop->setCurrentIndex(0);

    //校验位
    QStringList ParityList;
    ParityList<<"N"<<"Odd"<<"Even";
    ui->combo_box_parity->addItems(ParityList);
    ui->combo_box_parity->setCurrentIndex(0);

    //正则限制部分输入需要为数据
    QRegExp regx("[0-9]+$");
    QValidator *validator_time = new QRegExpValidator(regx,  ui->line_edit_time);
    ui->line_edit_time->setValidator( validator_time );
    QValidator *validator_id = new QRegExpValidator(regx,  ui->line_edit_dev_id);
    ui->line_edit_dev_id->setValidator( validator_id );

    //默认按键配置不可操作
    init_btn_disable(ui);
    ui->btn_uart_close->setDisabled(true);

    //信号槽通讯连接
    //启动应用相关的处理
    UartThreadInit();
    pMainUartProtocolInfo = GetUartProtocolInfo();
    connect(pMainUartProtocolInfo->m_pThread, SIGNAL(send_edit_recv(QString)), this, SLOT(append_text_edit_recv(QString)));
    connect(pMainUartProtocolInfo->m_pThread, SIGNAL(send_edit_test(QString)), this, SLOT(append_text_edit_test(QString)));
    pMainUartProtocolInfo->m_pThread->start();

    CTcpClientSocketInit();
    pCTcpClientSocket = GetTcpClientSocket();
}

void MainWindow::append_text_edit_recv(QString s)
{
    ui->text_edit_recv->append(s);
}

void MainWindow::append_text_edit_test(QString s)
{
    ui->text_edit_test->append(s);
}

//关闭应用后能操作按键配置
void init_btn_disable(Ui::MainWindow *ui)
{
    ui->btn_led_on->setDisabled(true);
    ui->btn_led_off->setDisabled(true);
    ui->btn_reboot->setDisabled(true);
    ui->btn_beep_on->setDisabled(true);
    ui->btn_beep_off->setDisabled(true);
    ui->btn_refresh->setDisabled(true);
    ui->btn_send_cmd->setDisabled(true);
    ui->line_edit_dev_id->setReadOnly(false);
}

//开启应用后能操作按键配置
void init_btn_enable(Ui::MainWindow *ui)
{
    ui->btn_led_on->setEnabled(true);
    ui->btn_led_off->setEnabled(true);
    ui->btn_reboot->setEnabled(true);
    ui->btn_beep_on->setEnabled(true);
    ui->btn_beep_off->setEnabled(true);
    ui->btn_refresh->setEnabled(true);
    ui->btn_send_cmd->setEnabled(true);
    ui->line_edit_dev_id->setReadOnly(true);
}

//打开LED
void MainWindow::on_btn_led_on_clicked()
{
    QString Strbuf;
    SSendBuffer *info = new SSendBuffer(sizeof(led_on_cmd), led_on_cmd, false);
    if(protocol_flag == PROTOCOL_UART)
    {
        pMainUartProtocolInfo->m_pUartQueue->QueuePost(info);
    }
    else if(protocol_flag == PROTOCOL_SOCKET)
    {
        pCTcpClientSocket->SocketProcess(info);
    }
    else
    {
        delete info;
    }
}

//关闭LED
void MainWindow::on_btn_led_off_clicked()
{
    SSendBuffer *info = new SSendBuffer(sizeof(led_off_cmd), led_off_cmd, false);
    if(protocol_flag == PROTOCOL_UART)
    {
        pMainUartProtocolInfo->m_pUartQueue->QueuePost(info);
    }
    else if(protocol_flag == PROTOCOL_SOCKET)
    {

    }
    else
    {
        delete info;
    }
}

//打开蜂鸣器
void MainWindow::on_btn_beep_on_clicked()
{
    SSendBuffer *info = new SSendBuffer(sizeof(beep_on_cmd), beep_on_cmd, false);
    if(protocol_flag == PROTOCOL_UART)
    {
        pMainUartProtocolInfo->m_pUartQueue->QueuePost(info);
    }
    else if(protocol_flag == PROTOCOL_SOCKET)
    {

    }
    else
    {
        delete info;
    }
}

//关闭蜂鸣器
void MainWindow::on_btn_beep_off_clicked()
{
    SSendBuffer *info = new SSendBuffer(sizeof(beep_off_cmd), beep_off_cmd, false);
    if(protocol_flag == PROTOCOL_UART)
    {
        pMainUartProtocolInfo->m_pUartQueue->QueuePost(info);
    }
    else if(protocol_flag == PROTOCOL_SOCKET)
    {

    }
    else
    {
        delete info;
    }
}

//指令直接发送的命令
void MainWindow::on_btn_send_cmd_clicked()
{
    static uint8_t nCacheBuf[256];
    bool bStatus;
    uint8_t nSize;
    QStringList QStrArr = ui->line_edit_cmd->text().split(" ");

    nSize = QStrArr.size();
    for(int index=0; index<nSize;index++){
        nCacheBuf[index] = QStrArr[index].toInt(&bStatus, 16);
    }

    SSendBuffer *info = new SSendBuffer(nSize, nCacheBuf, true);

    if(protocol_flag == PROTOCOL_UART)
    {
        pMainUartProtocolInfo->m_pUartQueue->QueuePost(info);
    }
    else if(protocol_flag == PROTOCOL_SOCKET)
    {

    }
    else
    {
        delete info;
    }
}

//关闭串口
void MainWindow::on_btn_uart_close_clicked()
{
    init_btn_disable(ui);
    pMainUartProtocolInfo->m_pComInfo->com_status = false;
    pMainUartProtocolInfo->m_pComInfo->com->close();
    pMainUartProtocolInfo->m_pComInfo->com->deleteLater();
    ui->btn_uart_close->setDisabled(true);
    ui->btn_uart_open->setEnabled(true);
    ui->btn_socket_open->setEnabled(true);
    ui->btn_socket_close->setEnabled(true);
    ui->combo_box_com->setEnabled(true);
    ui->combo_box_baud->setEnabled(true);
    ui->combo_box_data->setEnabled(true);
    ui->combo_box_stop->setEnabled(true);
    ui->combo_box_parity->setEnabled(true);
    append_text_edit_test(QString::fromLocal8Bit("串口关闭"));
}

//开启串口
void MainWindow::on_btn_uart_open_clicked()
{
    CComInfo *pComInfo;

    pComInfo = pMainUartProtocolInfo->m_pComInfo;
    pComInfo->com = new QextSerialPort(ui->combo_box_com->currentText(), QextSerialPort::Polling);
    pComInfo->com_status = pComInfo->com->open(QIODevice::ReadWrite);

    if(pComInfo->com_status)
    {
        //清除缓存区
        pComInfo->com->flush();
        //设置波特率
        pComInfo->com->setBaudRate((BaudRateType)ui->combo_box_baud->currentText().toInt());
        //设置数据位
        pComInfo->com->setDataBits((DataBitsType)ui->combo_box_data->currentText().toInt());
        //设置校验位
        pComInfo->com->setParity((ParityType)ui->combo_box_parity->currentText().toInt());
        //设置停止位
        pComInfo->com->setStopBits((StopBitsType)ui->combo_box_stop->currentText().toInt());
        pComInfo->com->setFlowControl(FLOW_OFF);
        pComInfo->com->setTimeout(10);
        init_btn_enable(ui);
        pMainUartProtocolInfo->SetId(ui->line_edit_dev_id->text().toShort());
        ui->btn_uart_close->setEnabled(true);
        ui->btn_uart_open->setDisabled(true);
        ui->btn_socket_open->setDisabled(true);
        ui->btn_socket_close->setDisabled(true);
        ui->combo_box_com->setDisabled(true);
        ui->combo_box_baud->setDisabled(true);
        ui->combo_box_data->setDisabled(true);
        ui->combo_box_stop->setDisabled(true);
        ui->combo_box_parity->setDisabled(true);
        append_text_edit_test(QString::fromLocal8Bit("串口打开成功!"));
        protocol_flag = PROTOCOL_UART;
    }
    else
    {
        pComInfo->com->deleteLater();
        pComInfo->com_status = false;
        append_text_edit_test(QString::fromLocal8Bit("串口打开失败"));
    }
}

//清理接收数据框
void MainWindow::on_btn_clear_clicked()
{
    ui->text_edit_test->clear();
    ui->text_edit_recv->clear();
}

//数组转换成指针发送
QString byteArrayToHexString(QString head, uint8_t* str, uint16_t size, QString tail)
{
    QString result = head;
    QString s;

    for(int i = 0; i < size; ++i)
    {
        s = QString("%1").arg(str[i], 0, 16);
        if(s.length() == 1)
            result.append("0x");
        result.append(s.toUpper());
        result.append(' ');
    }
    result += tail;
    result.chop(1);
    return result;
}

void MainWindow::on_btn_socket_open_clicked()
{
    init_btn_enable(ui);
    pCTcpClientSocket->SetId(ui->line_edit_dev_id->text().toShort());
    ui->btn_uart_close->setDisabled(true);
    ui->btn_uart_open->setDisabled(true);
    protocol_flag = PROTOCOL_SOCKET;
}

void MainWindow::on_btn_socket_close_clicked()
{
    init_btn_disable(ui);
    ui->btn_uart_close->setEnabled(true);
    ui->btn_uart_open->setEnabled(true);
    protocol_flag = PROTOCOL_NULL;
}
