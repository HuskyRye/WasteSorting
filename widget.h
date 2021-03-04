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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WIDGET_H
#define WIDGET_H

#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QWidget>

#include <QFileDialog>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>

#include <QBuffer>
#include <QByteArray>
#include <QFile>

#include <QDateTime>
#include <QTextCodec>
#include <QTimer>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <QCamera>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QCameraViewfinder>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    Widget(QWidget* parent = nullptr);
    ~Widget();

private:
    Ui::Widget* ui;

    QTimer* videoTimer;
    QMediaPlayer* player;
    QVideoWidget* videoWidget;
    QMediaPlaylist* playList;

    QSerialPort* serialPort;
    void initSerial();
    void serialWrite(const char data);

    QCamera* camera;
    QCameraViewfinder* viewFinder;
    QCameraImageCapture* imageCapture;
    void initCamera();
    void captureImage();

    QNetworkAccessManager* networkManager;
    QNetworkRequest* networkRequest;
    QUrl* url;
    void sendRequest(QByteArray& imageBase64);
    void classifyFinished(QString cate_name);

    qint64 number;

private slots:
    void timerUpdate();
    void videoTimerUpdate();
    void serialRead();
    void onImageCaptured(int, QImage image);
    void onRequestFinished(QNetworkReply* reply);

signals:
    void imageCaptured(int, QImage);
};

#endif // WIDGET_H
