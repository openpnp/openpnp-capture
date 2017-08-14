#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qDebug>
#include <QBoxLayout>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //setWindowTitle("ARSE");
    m_ctx = Cap_createContext();

    qDebug() << Cap_getLibraryVersion();

    CapDeviceID deviceID = 1;
    CapFormatID formatID = 7;

    m_streamID = Cap_openStream(m_ctx, deviceID, formatID);
    Cap_getFormatInfo(m_ctx, deviceID, formatID, &m_finfo);

    ui->setupUi(this);
    m_frameData.resize(m_finfo.width*m_finfo.height*3);

    QVBoxLayout *layout = new QVBoxLayout();
    m_frameDisplay = new QLabel();
    m_frameDisplay->setText("ARSE");
    m_frameDisplay->setFixedSize(QSize(m_finfo.width, m_finfo.height));
    m_frameDisplay->setStyleSheet("border: 1px solid red");

    qDebug() << "Frame size:" << m_finfo.width << m_finfo.height;

    layout->addWidget(m_frameDisplay);

    ui->centralWidget->setLayout(layout);

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, SIGNAL(timeout()), this, SLOT(doFrameUpdate()));
    m_refreshTimer->start(50);
}

MainWindow::~MainWindow()
{
    m_refreshTimer->stop();
    delete m_refreshTimer;

    Cap_closeStream(m_ctx, m_streamID);
    Cap_releaseContext(m_ctx);
    delete ui;
}

void MainWindow::doFrameUpdate()
{
    if (Cap_hasNewFrame(m_ctx, m_streamID))
    {
        Cap_captureFrame(m_ctx, m_streamID, &m_frameData[0], m_frameData.size());
        QImage img((const uint8_t*)&m_frameData[0],
                m_finfo.width,
                m_finfo.height,
                //m_finfo.width*3,
                QImage::Format_RGB888);

        if (img.isNull())
        {
            qDebug() << "Cannot create image..";
            return;
        }

        m_frameDisplay->setPixmap(QPixmap::fromImage(img));

        QString frameInfo = QString::asprintf("%d x %d frames:%d",
                                              m_finfo.width,
                                              m_finfo.height,
                                              Cap_getStreamFrameCount(m_ctx, m_streamID));

        ui->statusBar->showMessage(frameInfo);
    }
}

