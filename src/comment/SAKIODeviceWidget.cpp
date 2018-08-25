﻿/*******************************************************************************
* The file is encoding with utf-8 (with BOM)
*
* I write the comment with English, it's not because that I'm good at English,
* but for "installing B".
*
* Copyright (C) 2018-2018 wuhai persionnal. All rights reserved.
*******************************************************************************/
#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")
#endif

#include <QDateTime>
#include <QIntValidator>
#include <QScrollBar>
#include <QFileDialog>
#include <QTextStream>

#include "SAKIODeviceWidget.h"

#include "ui_SAKIODeviceWidget.h"

SAKIODeviceWidget::SAKIODeviceWidget(SAKIODevice *_device, SAKIODeviceControler *_controler, QWidget *parent)
    :QWidget(parent)
    ,device(_device)
    ,controler(_controler)
    ,ui(new Ui::SAKIODeviceWidget)
    ,cycleTimer(new QTimer)
    ,customControlerLayout(new QHBoxLayout)
{
    ui->setupUi(this);
    Connect();
    setCustomControler(controler);
    initUI();
}

SAKIODeviceWidget::~SAKIODeviceWidget()
{
    delete ui;
}

void SAKIODeviceWidget::initUI()
{
    ui->checkBoxOutputMS->setEnabled(true);
    ui->checkBoxOutputDate->setChecked(true);
    ui->checkBoxOutputTime->setChecked(true);
    ui->lineEditCycleTime->setValidator(new QIntValidator(10, 24*60*60*1000, this));

#if 0
    ui->pushButtonClearInput->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogResetButton));
    ui->pushButtonClearOutput->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogResetButton));
    ui->pushButtonReadInFile->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->pushButtonSaveOutput->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->pushButtonRefresh->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
#endif
}

void SAKIODeviceWidget::outputTimeInfoCheckBoxClicked(bool checked)
{
    if (checked){
        ui->checkBoxOutputMS->setEnabled(true);
    }else {
        ui->checkBoxOutputMS->setChecked(false);
        ui->checkBoxOutputMS->setEnabled(false);
    }
}

void SAKIODeviceWidget::outputReceiveDataOnlyCheckBoxClicked(bool cheaked)
{
    ui->textBrowserOutputData->clear();

    if (cheaked){
        ui->checkBoxOutputDate->setChecked(false);
        ui->checkBoxOutputDate->setEnabled(false);
        ui->checkBoxOutputTime->setChecked(false);
        ui->checkBoxOutputTime->setEnabled(false);
        ui->checkBoxOutputMS->setChecked(false);
        ui->checkBoxOutputMS->setEnabled(false);
    }else{
        ui->checkBoxOutputDate->setChecked(true);
        ui->checkBoxOutputDate->setEnabled(true);
        ui->checkBoxOutputTime->setChecked(true);
        ui->checkBoxOutputTime->setEnabled(true);
        ui->checkBoxOutputMS->setChecked(false);
        ui->checkBoxOutputMS->setEnabled(true);
    }
}

void SAKIODeviceWidget::openOrClose()
{
    if (device->isOpen()){
        close();
    }else{
        open();
    }
}

void SAKIODeviceWidget::setCustomControler(SAKIODeviceControler *controler)
{
    customControlerLayout->addWidget(controler);
    customControlerLayout->setMargin(0);
    ui->groupBoxCustom->setLayout(customControlerLayout);
}

void SAKIODeviceWidget::setRefreshPBtText(QString text)
{
    ui->pushButtonRefresh->setText(text);
}

void SAKIODeviceWidget::Connect()
{
    /// 打开/关闭设备
    connect(ui->pushButtonOpen, SIGNAL(clicked(bool)), this, SLOT(openOrClose()));
    connect(this, SIGNAL(need2open()), controler, SLOT(open()));
    connect(this, SIGNAL(need2close()), device, SLOT(close()));
    connect(controler, SIGNAL(need2open(QString,QString)), device, SLOT(open(QString,QString)));
    connect(controler, SIGNAL(need2open(QString,QString,QString,QString)), device, SLOT(open(QString,QString,QString,QString)));
    connect(controler, SIGNAL(need2open(QString,QString,QString,QString,QString)), device, SLOT(open(QString,QString,QString,QString,QString)));
    connect(device, SIGNAL(deviceOpenSuccessfully()), this, SLOT(afterDeviceOpen()));
    connect(device, SIGNAL(deviceCloseSuccessfully()), this, SLOT(afterDeviceClose()));

    /// 刷新设备
    connect(ui->pushButtonRefresh, SIGNAL(clicked(bool)), controler, SLOT(refresh()));

    connect(cycleTimer, SIGNAL(timeout()), this, SLOT(cycleTimerTimeout()));
    connect(ui->checkBoxCycle, SIGNAL(clicked(bool)), this, SLOT(checkedBoxCycleClicked(bool)));
    connect(device, SIGNAL(errorStr(QString)), this, SLOT(outputErrorString(QString)));
    connect(device, SIGNAL(infoStr(QString)), this, SLOT(outputInformationString(QString)));
    connect(ui->textEditInputData, SIGNAL(textChanged()), this, SLOT(textFormatControl()));
    connect(ui->pushButtonReadInFile, SIGNAL(clicked(bool)), this, SLOT(openFile()));
    connect(ui->pushButtonSaveOutput, SIGNAL(clicked(bool)), this, SLOT(saveOutputData()));

    connect(ui->checkBoxOutputTime, SIGNAL(clicked(bool)), this, SLOT(outputTimeInfoCheckBoxClicked(bool)));
    connect(ui->checkBoxOutputReceiveDataOnly, SIGNAL(clicked(bool)), this, SLOT(outputReceiveDataOnlyCheckBoxClicked(bool)));

    /// 数据收发
    connect(ui->pushButtonSendData, SIGNAL(clicked(bool)), this, SLOT(writeBytes()));
    connect(this, SIGNAL(need2writeBytes(QByteArray)), device, SLOT(writeBytes(QByteArray)));
    connect(device, SIGNAL(bytesRead(QByteArray)), this, SLOT(bytesRead(QByteArray)));
    connect(device, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));

    /// 重置收发计数
    connect(ui->pushButtonResetReceiveDataCount, SIGNAL(clicked(bool)), this, SLOT(resetReceiveDataCount()));
    connect(ui->pushButtonResetSendDataCount, SIGNAL(clicked(bool)), this, SLOT(resetSendDataCount()));
}

void SAKIODeviceWidget::afterDeviceOpen()
{
    controler->afterDeviceOpen();
    ui->pushButtonRefresh->setEnabled(false);
    ui->pushButtonOpen->setText(tr("关闭"));
}

void SAKIODeviceWidget::afterDeviceClose()
{
    controler->afterDeviceClose();
    ui->pushButtonRefresh->setEnabled(true);
    ui->pushButtonOpen->setText(tr("打开"));

    if (cycleTimer->isActive()){
        cycleTimer->stop();
        ui->pushButtonSendData->setEnabled(true);
        ui->lineEditCycleTime->setEnabled(true);
    }
}

void SAKIODeviceWidget::outputInfo(QString info, QString color, bool prefix)
{
    QString strTemp;
    if (prefix){
        strTemp += QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
        strTemp = QString("<font color=silver>%1 </font>").arg(strTemp);
    }

    strTemp += QString("<font color=%1>%2</font>").arg(color).arg(info);
    ui->labelOutput->setText(strTemp);
}

void SAKIODeviceWidget::outputErrorString(QString str)
{
    outputInfo(str, "red");
}

void SAKIODeviceWidget::outputInformationString(QString str)
{
    outputInfo(str, "green");
}

void SAKIODeviceWidget::writeBytes()
{
    QString tipStr = tr("设备未就绪，本次发送操作取消！");

    if (ui->checkBoxCycle->isChecked()){
        if (device->isOpen()){
            qlonglong cycleTime = ui->lineEditCycleTime->text().toLongLong();
            cycleTimer->start(cycleTime);
            ui->pushButtonSendData->setEnabled(false);
            ui->lineEditCycleTime->setEnabled(false);
        }else {
             outputInfo(tipStr, "red");
        }
    }else{
        if (device->isOpen()){
            emit need2writeBytes(dataBytes());
        }else {
            outputInfo(tipStr, "red");
        }
    }
}

void SAKIODeviceWidget::checkedBoxCycleClicked(bool checked)
{
    if (checked){

    }else{
        cycleTimer->stop();
        ui->pushButtonSendData->setEnabled(true);
        ui->lineEditCycleTime->setEnabled(true);
    }
}

void SAKIODeviceWidget::cycleTimerTimeout()
{
    if (device->isOpen()){
        emit need2writeBytes(dataBytes());
    }else {
        outputInfo(tr("设备未就绪，本次发送操作取消！"), "red");
    }
}

QByteArray SAKIODeviceWidget::dataBytes()
{
    QByteArray data;

    QString str = ui->textEditInputData->toPlainText();
    if (str.isEmpty()){
        str = "(null)";
        data = str.toLatin1();
        return data;
    }

    if (ui->radioButtonBinInput->isChecked()){
        QStringList strList = str.split(' ');
        foreach (QString str, strList){
            data.append((uint8_t)QString(str).toInt(NULL, 2));
        }
    }else if (ui->radioButtonOctInput->isChecked()){
        QStringList strList = str.split(' ');
        foreach (QString str, strList){
            data.append((uint8_t)QString(str).toInt(NULL, 8));
        }
    }else if (ui->radioButtonDecInput->isChecked()){
        QStringList strList = str.split(' ');
        foreach (QString str, strList){
            data.append((uint8_t)QString(str).toInt(NULL, 10));
        }
    }else if (ui->radioButtonHexInput->isChecked()){
        QStringList strList = str.split(' ');
        foreach (QString str, strList){
            data.append((uint8_t)QString(str).toInt(NULL, 16));
        }
    }else if (ui->radioButtonAsciiInput->isChecked()){
        data = ui->textEditInputData->toPlainText().toLatin1();
    }else if (ui->radioButtonUtf8Input->isChecked()){
        data = ui->textEditInputData->toPlainText().toLocal8Bit();
    }else {
        qWarning() << tr("未知输入各式！");
    }

    return data;
}

void SAKIODeviceWidget::textFormatControl()
{
    disconnect(ui->textEditInputData, SIGNAL(textChanged()), this, SLOT(textFormatControl()));
    QString plaintext = ui->textEditInputData->toPlainText();
    if (plaintext.isEmpty()){
        connect(ui->textEditInputData, SIGNAL(textChanged()), this, SLOT(textFormatControl()));
        return;
    }else {
        if (ui->radioButtonBinInput->isChecked()){  /// 二进制输入
            QString strTemp;
            plaintext.remove(QRegExp("[^0-1]"));
            for (int i = 0; i < plaintext.length(); i++){
                if ((i != 0) && (i % 8 == 0)){
                    strTemp.append(QChar(' '));
                }
                strTemp.append(plaintext.at(i));
            }
            ui->textEditInputData->setText(strTemp);
            ui->textEditInputData->moveCursor(QTextCursor::End);
        }else if(ui->radioButtonOctInput->isChecked()) { /// 八进制输入
            QString strTemp;
            plaintext.remove(QRegExp("[^0-7]"));
            for (int i = 0; i < plaintext.length(); i++){
                if ((i != 0) && (i % 2 == 0)){
                    strTemp.append(QChar(' '));
                }
                strTemp.append(plaintext.at(i));
            }
            ui->textEditInputData->setText(strTemp);
            ui->textEditInputData->moveCursor(QTextCursor::End);
        }else if(ui->radioButtonDecInput->isChecked()) { /// 十进制
            QString strTemp;
            plaintext.remove(QRegExp("[^0-9]"));
            for (int i = 0; i < plaintext.length(); i++){
                if ((i != 0) && (i % 2 == 0)){
                    strTemp.append(QChar(' '));
                }
                strTemp.append(plaintext.at(i));
            }
            ui->textEditInputData->setText(strTemp);
            ui->textEditInputData->moveCursor(QTextCursor::End);
        }else if(ui->radioButtonHexInput->isChecked()) { /// 十六进制
            QString strTemp;
            plaintext.remove(QRegExp("[^0-9a-fA-F]"));
            for (int i = 0; i < plaintext.length(); i++){
                if ((i != 0) && (i % 2 == 0)){
                    strTemp.append(QChar(' '));
                }
                strTemp.append(plaintext.at(i));
            }
            ui->textEditInputData->setText(strTemp.toUpper());
            ui->textEditInputData->moveCursor(QTextCursor::End);
        }else if(ui->radioButtonAsciiInput->isChecked()) { /// ascii
            plaintext.remove(QRegExp("[^\u0021-\u007E]"));
            ui->textEditInputData->setText(plaintext);
            ui->textEditInputData->moveCursor(QTextCursor::End);
        }else if(ui->radioButtonUtf8Input->isChecked()) {    /// utf-8

        }else {
            qWarning() << tr("未知输入选项！");
        }
    }
    connect(ui->textEditInputData, SIGNAL(textChanged()), this, SLOT(textFormatControl()));
}

void SAKIODeviceWidget::bytesRead(QByteArray data)
{
    /// 更新接收统计
    qlonglong receiveFrameCount = ui->labelReceiveFrames->text().toLongLong();
    receiveFrameCount += 1;
    ui->labelReceiveFrames->setText(QString::number(receiveFrameCount));
    qlonglong receiveBytesCount = ui->labelReceiveBytes->text().toLongLong();
    receiveBytesCount += data.length();
    ui->labelReceiveBytes->setText(QString::number(receiveBytesCount));

    if (ui->checkBoxPause->isChecked()){
        return;
    }

    QString str;
    if (ui->checkBoxOutputDate->isChecked()){
        str.append(QDate::currentDate().toString("yyyy/MM/dd "));
        str = QString("<font color=silver>%1</font>").arg(str);
    }

    if (ui->checkBoxOutputTime->isChecked()){
        if (ui->checkBoxOutputMS->isChecked()){
            str.append(QTime::currentTime().toString("hh:mm:ss.z "));
        }else {
            str.append(QTime::currentTime().toString("hh:mm:ss "));
        }
        str = QString("<font color=silver>%1</font>").arg(str);
    }

    if (ui->radioButtonBinOutput->isChecked()){
        for (int i = 0; i < data.length(); i++){
            str.append(QString("%1 ").arg(QString::number((uint8_t)data.at(i), 2), 8, '0'));
        }
    }else if (ui->radioButtonOctOutput->isChecked()){
        for (int i = 0; i < data.length(); i++){
            str.append(QString("%1 ").arg(QString::number((uint8_t)data.at(i), 8), 2, '0'));
        }
    }else if (ui->radioButtonDecOutput->isChecked()){
        for (int i = 0; i < data.length(); i++){
            str.append(QString("%1 ").arg(QString::number((uint8_t)data.at(i), 10), 2, '0'));
        }
    }else if (ui->radioButtonHexOutput->isChecked()){
        for (int i = 0; i < data.length(); i++){
            str.append(QString("%1 ").arg(QString::number((uint8_t)data.at(i), 16), 2, '0'));
        }
    }else if (ui->radioButtonAsciiOutput->isChecked()){
        str.append(QString(data));
    }else if (ui->radioButtonUtf8Output->isChecked()){
        str.append(QString(data));
    }else {
        qWarning() << "Unknown output model!";
    }

    if (ui->checkBoxOutputReceiveDataOnly->isChecked()){
        /**
         * 以下这种追加文本的方式存在问题，在文本较多，追加频繁时，界面卡顿。
         **/
        if (!ui->textBrowserOutputData->toPlainText().isEmpty()){
            str = ui->textBrowserOutputData->toPlainText() + " " + str;
        }
        ui->textBrowserOutputData->setText(QString("<font color=blue>%1</font>").arg(str));
        ui->textBrowserOutputData->verticalScrollBar()->setSliderPosition(ui->textBrowserOutputData->verticalScrollBar()->maximum());
    }else {
        ui->textBrowserOutputData->append(str);
    }
}

void SAKIODeviceWidget::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"));
    if (!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QFile::ReadOnly)){
            QByteArray data = file.readAll();
            ui->textEditInputData->setText(QString(data).toUtf8());
            file.close();
        }else{
            qWarning() << QString("%1 %2").arg(tr("无法打开文件")).arg(fileName);
        }
    }
}

void SAKIODeviceWidget::saveOutputData()
{
    QString outFileName = QFileDialog::getSaveFileName();
    QFile outFile(outFileName);

    outFile.open(QIODevice::WriteOnly|QIODevice::Text);

    QTextStream outStream(&outFile);
    outStream << ui->textBrowserOutputData->toPlainText();

    outFile.flush();
    outFile.close();
}

void SAKIODeviceWidget::bytesWritten(qint64 bytes)
{
    qlonglong writeBytes = ui->labelSendBytes->text().toLongLong();
    writeBytes += bytes;
    ui->labelSendBytes->setText(QString::number(writeBytes));

    qlonglong writeFrame = ui->labelSendFrames->text().toLongLong();
    writeFrame += 1;
    ui->labelSendFrames->setText(QString::number(writeFrame));
}

void SAKIODeviceWidget::resetSendDataCount()
{
    ui->labelSendBytes->setText("0");
    ui->labelSendFrames->setText("0");
}

void SAKIODeviceWidget::resetReceiveDataCount()
{
    ui->labelReceiveBytes->setText("0");
    ui->labelReceiveFrames->setText("0");
}