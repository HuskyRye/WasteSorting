/*
 *  Copyright (C) 2021 刘臣轩
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(close()));
    ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/主.png);}");
    ui->textEdit->append("开始初始化设备");

    // Time
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(500);

    initSerial();
#ifdef Q_OS_WIN
    initCamera();
#else
    connect(this, SIGNAL(imageCaptured(int, QImage)), this, SLOT(onImageCaptured(int, QImage)));
    ui->textEdit->append("摄像头初始化成功");
#endif

    // Network
    networkManager = new QNetworkAccessManager();
    networkRequest = new QNetworkRequest();
    networkRequest->setHeader(QNetworkRequest::ContentTypeHeader, "	application/json;charset=UTF-8");
    url = new QUrl("https://aiapi.jd.com/jdai/garbageImageSearch");
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRequestFinished(QNetworkReply*)));

    ui->label_3->setText("工训大赛");
    ui->label_4->setStyleSheet("border-image: url(:/new/prefix1/image/工训大赛.png);\nborder-radius: 10px;\n");
    ui->label_4->setVisible(true);
    ui->label_5->setVisible(false);
    ui->textEdit->append("设备初始化成功√");
    number = 0;

    // Video
    player = new QMediaPlayer;
    videoWidget = new QVideoWidget(this);
    playList = new QMediaPlaylist;
#ifdef Q_OS_WIN
    playList->addMedia(QUrl::fromLocalFile("../WasteSorting/test.mp4"));
#else
    playList->addMedia(QUrl::fromLocalFile("/home/pi/Desktop/WasteSorting/test.mp4"));
#endif
    playList->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    player->setPlaylist(playList);
    player->setVideoOutput(videoWidget);
    ui->verticalLayout->addWidget(videoWidget);
    videoWidget->setVisible(false);

    videoTimer = new QTimer(this);
    connect(videoTimer, SIGNAL(timeout()), this, SLOT(videoTimerUpdate()));
    videoTimer->setSingleShot(true);
    videoTimer->start(10000);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::timerUpdate()
{
    QString str = QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss");
    ui->label->setText(str);
}

void Widget::videoTimerUpdate()
{
    ui->label_4->setVisible(false);
    player->play();
    videoWidget->setVisible(true);
    ui->label_3->setText("播放视频");
    ui->textEdit->append("播放视频");
}

void Widget::initSerial()
{
    ui->textEdit->append("开始初始化串口");
    serialPort = new QSerialPort();
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(serialRead()));
    if (QSerialPortInfo::availablePorts().length() == 0) {
        QMessageBox::critical(this, "错误", "无可用串口设备，请检查硬件连接后重试");
        exit(0);
    }
#ifdef Q_OS_WIN
    QString portName = QSerialPortInfo::availablePorts()[1].portName();
    qDebug() << portName;
    ui->textEdit->append("尝试连接串口" + portName);
    serialPort->setPortName(portName);
#else
    QString portName = "ttyUSB0";
    ui->textEdit->append("尝试连接串口ttyUSB0");
    serialPort->setPortName("ttyUSB0");
#endif
    if (serialPort->open(QIODevice::ReadWrite)) {
        ui->textEdit->append("串口连接成功");
        serialPort->setBaudRate(115200);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
    } else {
        QMessageBox::critical(this, "错误", "串口设备" + portName + "无法打开，请检查硬件连接后重试");
        exit(0);
    }
    ui->textEdit->append("串口初始化成功");
}

void Widget::initCamera()
{
    // for (QCameraInfo& info : QCameraInfo::availableCameras()) {
    //     qDebug() << info.deviceName();
    // }
    int camNum = QCameraInfo::availableCameras().length();
    if (!camNum) {
        QMessageBox::critical(this, "错误", "无可用摄像头，请检查硬件连接后重试");
        exit(0);
    }
    ui->textEdit->append("开始初始化摄像头");
    camera = new QCamera(QCameraInfo::availableCameras()[0], this);
    imageCapture = new QCameraImageCapture(camera);
    connect(imageCapture, SIGNAL(imageCaptured(int, QImage)), this, SLOT(onImageCaptured(int, QImage)));
    camera->setCaptureMode(QCamera::CaptureStillImage);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    camera->start();
    ui->textEdit->append("摄像头初始化成功");
}

void Widget::serialRead()
{
    QByteArray buffer = serialPort->readAll();
    // qDebug() << buffer;
    if (buffer[0] == '\x03' && buffer[1] == '\xFC'
        && buffer[3] == '\xFC' && buffer[4] == '\x03') {
        switch (buffer[2]) {
        case '\x00':
            ui->textEdit->append("取消警报");
            ui->label_3->setText("取消警报");
            ui->label_4->setVisible(true);
            ui->label_5->setVisible(false);
            ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/主.png);}");
            videoTimer->start(10000);
            break;
        case '\x01':
            ui->textEdit->append("触发拍照信号");
            ui->label_3->setText("触发拍照");
            videoTimer->stop();
            videoWidget->setVisible(false);
            player->stop();
#ifdef Q_OS_WIN
            imageCapture->capture();
#else
            captureImage();
#endif
            break;
        case '\x02':
            ui->textEdit->append("投递完毕");
            ui->label_3->setText("投递完毕");
            ui->label_4->setVisible(true);
            ui->label_5->setVisible(false);
            ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/主.png);}");
            videoTimer->start(10000);
            break;
        case '\x04':
            ui->textEdit->append("满载警报");
            ui->label_3->setText("满载警报");
            ui->label_4->setVisible(false);
            videoTimer->stop();
            videoWidget->setVisible(false);
            player->stop();
            ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/满载警报.png);}");
            break;
        case '\x08':
            // qDebug() << "倾倒警报";
            ui->textEdit->append("倾倒警报");
            ui->label_3->setText("倾倒警报");
            ui->label_4->setVisible(false);
            videoTimer->stop();
            videoWidget->setVisible(false);
            player->stop();
            ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/倾倒警报.png);}");
            break;
        case '\xFF':
            break;
        }
    }
}

void Widget::serialWrite(const char data)
{
    QByteArray buffer("\x30\xCF\x0F\xCF\x30");
    buffer[2] = data;
    serialPort->write(buffer);
}

void Widget::captureImage()
{
    system("raspistill -o ../WasteSorting/WasteSorting.jpg -t 1 -br 70 -hf -awb sun");
    QImage image("../WasteSorting/WasteSorting.jpg");
    emit(imageCaptured(0, image));
}

void Widget::onImageCaptured(int, QImage image)
{
    // 显示图片
    ui->label_4->setVisible(false);
    ui->label_5->setPixmap(QPixmap::fromImage(image).scaled(405, 306));
    ui->label_5->setVisible(true);
    ui->label_3->setText("识别中");
    ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/识别中.png);}");

    /* 使用京东垃圾识别 API
    // Base64编码
    QByteArray ba;
    QBuffer buf(&ba);
    image.save(&buf, "JPG", -1);
    QByteArray imageBase64 = ba.toBase64();
    // qDebug() << "data:image/jpg;base64," + imageBase64;

    // Request
    sendRequest(imageBase64);
    */

    /* 使用TensorFlow Lite */
#ifdef Q_OS_WIN
    image.save("../WasteSorting/WasteSorting.jpg");
    FILE* fp = _popen("python ../WasteSorting/tensorflow/label_image.py", "rt");
    char buf[255] = { 0 };
    fscanf(fp, "%s", buf);
    _pclose(fp);
#else
    FILE* fp = popen("python3 ../WasteSorting/tensorflow/label_image.py", "r");
    char buf[255] = { 0 };
    printf("%s", buf);
    fscanf(fp, "%s", buf);
    pclose(fp);
#endif
    QString cate_name = QString::fromLocal8Bit(buf);
    qDebug() << cate_name;
    classifyFinished(cate_name);
}

void Widget::sendRequest(QByteArray& imageBase64)
{
    QUrlQuery query;
    query.addQueryItem("appkey", "3a24b33468565b633d25d426eb0c660c");
    qint64 timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    query.addQueryItem("timestamp", QString::number(timestamp));
    QString sign = "58125e5985e6ef2d385ebfaa646987ba" + QString::number(timestamp);
    QByteArray ba = QCryptographicHash::hash(sign.toUtf8(), QCryptographicHash::Md5);
    query.addQueryItem("sign", ba.toHex());
    url->setQuery(query);
    networkRequest->setUrl(*url);
    // qDebug() << query.toString();

    QJsonObject json;
    json.insert("imgBase64", QString(imageBase64));
    json.insert("cityId", "440300");
    QJsonDocument document;
    document.setObject(json);
    QByteArray data = document.toJson(QJsonDocument::Compact);
    // qDebug() << QString(data);

    networkManager->post(*networkRequest, data);
}

void Widget::onRequestFinished(QNetworkReply* reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!statusCode.isValid()) {
        QMessageBox::critical(this, "错误", "网络连接不可用，请检查后重试");
        exit(0);
    }
    QByteArray replyData = reply->readAll();
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(replyData, &jsonError);
    if (!document.isNull() && (jsonError.error == QJsonParseError::NoError)) {
        QJsonObject object = document.object().value("result").toObject();
        QJsonArray array = object.value("garbage_info").toArray();
        QJsonObject max = std::max_element(array.begin(), array.end(),
            [](QJsonValue const& a, QJsonValue const& b) { return a.toObject().value("confidence").toDouble()
                                                               < b.toObject().value("confidence").toDouble(); })
                              ->toObject();
        QString cate_name = max.value("cate_name").toString();
        QString garbage_name = max.value("garbage_name").toString();
        double confidence = max.value("confidence").toDouble();
        // qDebug() << "cate_name:" << cate_name;
        // qDebug() << "garbage_name:" << garbage_name;
        // qDebug() << "confidence:" << confidence;
        // ui->textEdit->append("cate_name: " + cate_name);
        // ui->textEdit->append("garbage_name: " + garbage_name);
        // ui->textEdit->append("confidence: " + QString::number(confidence));
        classifyFinished(cate_name);

    } else {
        serialWrite('\xFF');
        ui->textEdit->append("识别失败，请重试");
        ui->label_3->setText("识别失败");
        ui->label_4->setVisible(true);
        ui->label_5->setVisible(false);
        ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/主.png);}");
    }
}

void Widget::classifyFinished(QString cate_name)
{
    number += 1;
    ui->textEdit->append(QString::number(number) + " " + cate_name + " 1 OK!");

    ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/" + cate_name + ".PNG);}");
    ui->label_3->setText("投递中");
    if (cate_name == "识别失败") {
        serialWrite('\xFF');
        ui->textEdit->append("识别失败，请重试");
        ui->label_3->setText("识别失败");
        ui->label_4->setVisible(true);
        ui->label_5->setVisible(false);
        ui->frame->setStyleSheet("#frame {border-image: url(:/new/prefix1/image/主.png);}");
    } else if (cate_name == "可回收垃圾")
        serialWrite('\x01');
    else if (cate_name == "厨余垃圾")
        serialWrite('\x02');
    else if (cate_name == "有害垃圾")
        serialWrite('\x04');
    else
        serialWrite('\x08');
}
