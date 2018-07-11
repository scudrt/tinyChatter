#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QTcpServer>
#include <QScrollBar>
#include <QtNetwork>
#include <QtGlobal>
#include <QPainter>
#include <conio.h>
#include <QDebug>
#include <ctime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //renew the local IP
    void refreshLocalIP();

    //
    void cutShowLabel();

protected:
    //event filter for alerting
    bool eventFilter(QObject*,QEvent*);

    /***mouse drag***/
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private slots:
    void onServerReadingData();

    void onSocketReadingData();

    void onServerNewConnection();

    void onGuestDisconnected();

    void onServerDisconnected();

    void on_sendPersonButton_clicked();

    void on_connectButton_clicked();

    void on_sendGuestButton_clicked();

    void on_refreshIPButton_clicked();

    void on_theButton_clicked();

    void on_showLabel_textChanged();

    void on_exitButton_clicked();

    void on_minimizeButton_clicked();

    void on_changeSkinButton_clicked();

    void onTimeOut();

    void on_switchPersonButton_clicked();

    void on_sendMessageButton_clicked();

private:
    QString myLocalIP;

    //if the user is connecting to a server,it might be true
    bool connecting;

    //if current object is the guest,it is true
    bool chattingWithGuest;

    Ui::MainWindow *ui;

    /***varieties for TCP dialog***/
    QTcpSocket *socket,*serverSocket;
    QTcpServer *server;

    /***varieties for mouse drag***/
    bool isMousePressing;
    QPoint beginMousePoint;

    /***timer for limiting the sending frequency***/
    QTimer *sendingTimer;
    int sendingChance;
};

#endif // MAINWINDOW_H
