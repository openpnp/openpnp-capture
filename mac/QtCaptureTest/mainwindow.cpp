#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qDebug>
#include <QBoxLayout>
#include <QPixmap>

struct CustomComboBoxData
{
    uint32_t m_device;
    uint32_t m_format;
};

Q_DECLARE_METATYPE(CustomComboBoxData)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //setWindowTitle("ARSE");
    m_ctx = Cap_createContext();

    qDebug() << Cap_getLibraryVersion();

    CapDeviceID deviceID = 0;
    CapFormatID formatID = 0;

    m_streamID = Cap_openStream(m_ctx, deviceID, formatID);
    Cap_getFormatInfo(m_ctx, deviceID, formatID, &m_finfo);

    ui->setupUi(this);
    m_frameData.resize(m_finfo.width*m_finfo.height*3);

    m_frameDisplay = new QLabel();
    m_frameDisplay->setFixedSize(QSize(m_finfo.width, m_finfo.height));
    m_frameDisplay->setStyleSheet("border: 1px solid red");

    qDebug() << "Frame size:" << m_finfo.width << m_finfo.height;

    for(uint32_t device=0; device<Cap_getDeviceCount(m_ctx); device++)
    {
        QString deviceName = Cap_getDeviceName(m_ctx, device);
        for(int32_t format=0; format<Cap_getNumFormats(m_ctx, device); format++)
        {
            CapFormatInfo finfo;
            Cap_getFormatInfo(m_ctx, device, format, &finfo);

            QString fourcc;
            for(uint32_t i=0; i<4; i++)
            {
                fourcc += (char)(finfo.fourcc & 0xFF);
                finfo.fourcc >>= 8;
            }

            QString formatName = QString::asprintf("%dx%d %s", finfo.width, finfo.height, fourcc.toLatin1().data());
            QString total = deviceName + " " + formatName;
            CustomComboBoxData customData;
            customData.m_device = device;
            customData.m_format = format;
            QVariant v;
            v.setValue(customData);
            ui->cameraChooser->addItem(total, v);
        }
    }

    ui->verticalLayout->addWidget(m_frameDisplay);

    connect(ui->cameraChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

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

void MainWindow::changeCamera(int index)
{
    QVariant v = ui->cameraChooser->currentData();
    CustomComboBoxData data = v.value<CustomComboBoxData>();

    // kill currently running stream
    Cap_closeStream(m_ctx, m_streamID);

    qDebug() << "Opening new device/format: " << data.m_device << data.m_format;

    // open new stream
    m_streamID = Cap_openStream(m_ctx, data.m_device, data.m_format);

    qDebug() << "New stream ID" << m_streamID;

    // resize the display window
    Cap_getFormatInfo(m_ctx, data.m_device, data.m_format, &m_finfo);
    m_frameData.resize(m_finfo.width*m_finfo.height*3);
    m_frameDisplay->setFixedSize(QSize(m_finfo.width, m_finfo.height));
    m_frameDisplay->setStyleSheet("border: 1px solid red");
}
