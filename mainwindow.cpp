#include "mainwindow.h"
#include "ui_mainwindow.h"

#define SHOW_LABEL_SIZE_LIMIT 25*5 //(size per row)*rows
#define TEST_MODE false

void MainWindow::cutShowLabel()
{//this function is made to limit the size of context in showLabel
    int currentSize = ui->showLabel->toPlainText().size();
    if (currentSize > SHOW_LABEL_SIZE_LIMIT)
    {
        QTextCursor cur = ui->showLabel->textCursor();
        cur.setPosition(0);
        cur.movePosition(QTextCursor::EndOfLine,QTextCursor::KeepAnchor);
        cur.removeSelectedText();
        cur.deleteChar();
    }
}

void MainWindow::refreshLocalIP()
{//renew the local IP
    QString str;
    QList<QHostAddress> adds = QHostInfo::fromName(QHostInfo::localHostName()).addresses();
    for (int i=0;i<adds.size();++i)
    {
        QHostAddress add = adds.at(i);
        if (add.protocol() == QAbstractSocket::IPv4Protocol)
        {//found an IP using IPV4 protocol
            str = add.toString();
            myLocalIP = str;
            str = "本机IP: "+str;
            ui->ipLabel->setText(str);
            break;
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->theButton->setVisible(TEST_MODE);

    /***set scrollbar outlook***/
    QString scrollBarStyle = QString("QScrollBar:vertical{width:18px;background:rgba(0,0,0,0);}"
                                     "QScrollBar::handle:vertical{background:rgba(0,0,0,0);}"
                                     "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical"
                                     "{border-image:url(none.png);}"
                                     "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
                                     "{background:rgba(255,255,255,25%);}}");
    QScrollBar *bar = new QScrollBar(Qt::Vertical,ui->showLabel);
    bar->setStyleSheet(scrollBarStyle);
    ui->showLabel->setVerticalScrollBar(bar);
    QScrollBar *bar2 = new QScrollBar(Qt::Vertical,ui->editLabel);
    bar2->setStyleSheet(scrollBarStyle);
    ui->editLabel->setVerticalScrollBar(bar2);

    /***socket connecting***/
    connecting = false;

    /***keyboard event***/
    connect(ui->targetIPLabel,SIGNAL(returnPressed()),this,SLOT(on_connectButton_clicked()));

    /***chatter***/
    chattingWithGuest = false;

    /***timer and random number for generating the background***/
    srand(time(0));
    sendingChance = 3;
    sendingTimer = new QTimer(this);
    connect(sendingTimer,SIGNAL(timeout()),this,SLOT(onTimeOut()));
    sendingTimer->start(1000);

    /***make the editer transparent***/
    ui->editLabel->setStyleSheet("background-color:rgba(255,255,255,100)");
    ui->showLabel->setStyleSheet("background-color:rgba(255,255,255,100)");
    ui->targetIPLabel->setStyleSheet("background-color:rgba(255,255,255,0)");
    ui->guestIPLabel->setStyleSheet("background-color:rgba(255,255,255,0)");
    ui->myNameLabel->setStyleSheet("background-color:rgba(255,255,255,0)");

    /***mouse drag***/
    isMousePressing = false;

    /***background color***/
    QWidget *wid = new QWidget(ui->colorLabel);
    wid->setGeometry(0,0,450,700);
    wid->setStyleSheet("background:qlineargradient(x1:0,y1:0.6,x2:1,y2:0.2,"
    "stop:0 rgb(200,200,200),stop:0.6 rgb(200,230,255),stop:1 rgb(220,220,220))");

    /***window of the application***/
    ui->activeLabel->setStyleSheet("background-color:rgba(100,100,100,150)");
    this->setWindowFlags(Qt::FramelessWindowHint);

    /***event filter***/
    this->QWidget::installEventFilter(this);
    ui->editLabel->QWidget::installEventFilter(this);

    /***socket***/
    socket = new QTcpSocket(this);
    serverSocket = new QTcpSocket(this);

    /***server***/
    server = new QTcpServer(this);
    server->listen(QHostAddress::Any,2333);
    connect(server,&QTcpServer::newConnection,this,&MainWindow::onServerNewConnection);

    /***set the local IP***/
    refreshLocalIP();
}

MainWindow::~MainWindow()
{
    socket->abort();
    serverSocket->abort();
    server->close();
    delete ui;
}

bool MainWindow::eventFilter(QObject* now,QEvent* event)
{
    if (now == this)
    {
        if (event->type() == QEvent::WindowActivate)
        {
            ui->activeLabel->setVisible(false);
            setWindowTitle("密聊");
            return true;
        }
        else if (event->type() == QEvent::WindowDeactivate)
        {
            ui->activeLabel->setVisible(true);
            return true;
        }
    }
    else if (now == ui->editLabel)
    {
        if (event->type() == QKeyEvent::KeyPress)
        {
            auto e = static_cast<QKeyEvent*>(event);
            if (e->modifiers() == Qt::ControlModifier)
            {
                if (e->key() == Qt::Key_Tab)
                {
                    on_switchPersonButton_clicked();
                    return true;
                }
                else if (e->key()==Qt::Key_Enter || e->key()==Qt::Key_Return)
                {
                    ui->editLabel->append("");
                    return true;
                }
            }
            else if (e->key()==Qt::Key_Enter || e->key()==Qt::Key_Return)
            {
                on_sendMessageButton_clicked();
                return true;
            }
        }
    }
    return false;
}

void MainWindow::mousePressEvent(QMouseEvent *_e)
{//when the mouse press the window
    isMousePressing = true;
    beginMousePoint = _e->pos();
    QWidget::mousePressEvent(_e);
}

void MainWindow::mouseMoveEvent(QMouseEvent *_e)
{//move the window with mouse
    if (isMousePressing)
    {
        this->move(QPoint(cursor().pos()-beginMousePoint));
    }
    QWidget::mouseMoveEvent(_e);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *_e)
{//release the window
    isMousePressing = false;
    QWidget::mouseReleaseEvent(_e);
}

void MainWindow::onTimeOut()
{//add a chance every 1000ms,up to 6
    if (sendingChance < 6)
    {
        ++sendingChance;
    }
}

void MainWindow::onServerNewConnection()
{//deal with the new connection
 //notification:the last guest will be abandoned
    serverSocket = server->nextPendingConnection();
    connect(serverSocket,&QTcpSocket::readyRead,this,&MainWindow::onServerReadingData);
    connect(serverSocket,&QTcpSocket::disconnected,this,&MainWindow::onGuestDisconnected);
    ui->guestIPLabel->setText(serverSocket->peerAddress().toString());
}

void MainWindow::onGuestDisconnected()
{//when the guest leaves your room
    ui->guestIPLabel->setText("(访客离开)");
    serverSocket->disconnectFromHost();
}

void MainWindow::onServerDisconnected()
{//the server is offline
    connecting = false;
    ui->connectLabel->setText("断开");
    socket->disconnectFromHost();
}

void MainWindow::onServerReadingData()
{//the server gets the pack
    QByteArray data = serverSocket->readAll();
    if (data.isEmpty() == false)
    {
        ui->showLabel->append(QString("(访客)")+data);
        qDebug() << data;
        cutShowLabel();
    }
}

void MainWindow::onSocketReadingData()
{//the guest get the pack
    QByteArray data = socket->readAll();
    if (data.isEmpty() == false)
    {
        ui->showLabel->append(QString("(密友)")+data);
        cutShowLabel();
    }
}

void MainWindow::on_sendPersonButton_clicked()
{//send message to the server
    if (ui->editLabel->toPlainText().isEmpty() == true)
    {
        ui->showLabel->append("消息为空");
        cutShowLabel();
        return;
    }
    if (sendingChance <= 0) //send too fast
    {
        ui->showLabel->append("发送太频繁");
        cutShowLabel();
        return;
    }
    if (connecting)
    {
        --sendingChance;
        QString str;
        if (ui->myNameLabel->text().isEmpty() == false)
        {
            str = ui->myNameLabel->text() + ": ";
        }
        else
        {
            str = myLocalIP+":";
        }
        socket->write((str+ui->editLabel->toPlainText()).toStdString().c_str());
        socket->flush();
        ui->showLabel->append(QString("to密友: ")+ui->editLabel->toPlainText());
        ui->editLabel->clear();
    }
    else
    {
        ui->showLabel->append(QString(tr("聊天对象不在或未连接")));
    }
    cutShowLabel();
}

void MainWindow::on_sendGuestButton_clicked()
{
    if (ui->editLabel->toPlainText().isEmpty() == true)
    {
        ui->showLabel->append("消息为空");
        cutShowLabel();
        return;
    }
    if (sendingChance <= 0)
    {
        ui->showLabel->append("发送太频繁");
        cutShowLabel();
        return;
    }
    if (ui->guestIPLabel->text().contains("(") == false)
    {
        --sendingChance;
        QString str;
        if (ui->myNameLabel->text().isEmpty() == false)
        {
            str = ui->myNameLabel->text() + ": ";
        }
        else
        {
            str = myLocalIP+":";
        }
        serverSocket->write((str+ui->editLabel->toPlainText()).toStdString().c_str());
        serverSocket->flush();
        ui->showLabel->append(QString("to访客: ")+ui->editLabel->toPlainText());
        ui->editLabel->clear();
    }
    else
    {
        ui->showLabel->append(QString(tr("当前没有访客")));
    }
    cutShowLabel();
}

void MainWindow::on_connectButton_clicked()
{//try to connect to the server
    ui->connectLabel->setText(tr("等待"));
    if (connecting)
    {
        socket->disconnectFromHost();
    }
    QHostAddress targetIP = QHostAddress(ui->targetIPLabel->text().toStdString().c_str());
    socket->connectToHost(targetIP,2333);
    QCoreApplication::processEvents();
    if (socket->waitForConnected(4000) == true)
    {
        connecting = true;
        connect(socket,&QTcpSocket::readyRead,this,&MainWindow::onSocketReadingData);
        connect(socket,&QTcpSocket::disconnected,this,&MainWindow::onServerDisconnected);
        ui->connectLabel->setText(tr("已连"));
        ui->editLabel->setFocus();
    }
    else
    {
        connecting = false;
        socket->disconnectFromHost();
        ui->connectLabel->setText(tr("超时"));
    }
}

void MainWindow::on_refreshIPButton_clicked()
{
    refreshLocalIP();
    ui->showLabel->append("当前IP:"+myLocalIP);
    cutShowLabel();
}

void MainWindow::on_theButton_clicked()
{/***button for test in developping***/
}

void MainWindow::on_showLabel_textChanged()
{
    if (this->isActiveWindow() == false)
    {//tell the user there are some messages
        setWindowTitle("***有新留言***");
    }
}

void MainWindow::on_exitButton_clicked()
{
    qApp->quit();
    qApp->exit(0); //if the last one doesn't work...
}

void MainWindow::on_minimizeButton_clicked()
{
    this->showMinimized();
}

void MainWindow::on_changeSkinButton_clicked()
{
    /***generate the background using linear gradient randomly***/
    double x1=rand()%256/256.0,y1=rand()%256/256.0,x2=rand()%256/256.0,y2=rand()%256/256.0;
    double midLeft=rand()%256/256.0,midRight=rand()%256/256.0;
    int r1=rand()%256,r2=rand()%256,r3=rand()%256,r4=rand()%256,r5=rand()%256;
    int g1=rand()%256,g2=rand()%256,g3=rand()%256,g4=rand()%256,g5=rand()%256;
    int b1=rand()%256,b2=rand()%256,b3=rand()%256,b4=rand()%256,b5=rand()%256;
    char styleStr[281];
    int triger = rand()%100; //choose the gradient function randomly
    if (triger <=49)
    {
        sprintf(styleStr,"background:qlineargradient(x1:%.1lf,y1:%.1lf,x2:%.1lf,y2:%.1lf,"
                "stop:0 rgb(%d,%d,%d),stop:%.2lf rgb(%d,%d,%d),"
                "stop:%.2lf rgb(%d,%d,%d),stop:1.0 rgb(%d,%d,%d),stop:0.5 rgb(%d,%d,%d))",
                x1,y1,x2,y2,r1,g1,b1,midLeft,r2,g2,b2,midRight,r3,g3,b3,r4,g4,b4,r5,g5,b5);
    }
    else
    {
        double k1=rand()%256/256.0,k2=rand()%256/256.0;
        sprintf(styleStr,"background:qradialgradient(cx:%.1lf,cy:%.1lf,fx:%.1lf,fy:%.1lf,radius:%.1lf,"
                         "stop:%.2lf rgb(%d,%d,%d),stop:%.2lf rgb(%d,%d,%d),stop:%2.lf rgb(%d,%d,%d))",
                x1,y1,x2,y2,midLeft,k1,r1,g1,b1,midRight,r2,g2,b2,k2,r3,g3,b3);
    }
    QWidget *wid = new QWidget(ui->colorLabel);
    wid->setGeometry(0,0,441,621);
    wid->setStyleSheet(styleStr);
    wid->show();
}

void MainWindow::on_switchPersonButton_clicked()
{
    if (chattingWithGuest)
    {
        chattingWithGuest = false;
        ui->switchPersonButton->setText(tr("切换(Ctrl+Tab):to密友"));
    }
    else
    {
        chattingWithGuest = true;
        ui->switchPersonButton->setText(tr("切换(Ctrl+Tab):to访客"));
    }
}

void MainWindow::on_sendMessageButton_clicked()
{
    if (chattingWithGuest)
    {
        on_sendGuestButton_clicked();
    }
    else
    {
        on_sendPersonButton_clicked();
    }
}
