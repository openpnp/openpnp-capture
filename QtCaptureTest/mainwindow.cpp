#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QBoxLayout>
#include <QPixmap>

struct CustomComboBoxData
{
    uint32_t m_device;
    uint32_t m_format;
};

Q_DECLARE_METATYPE(CustomComboBoxData)

static MainWindow *g_logWindowPtr = 0;
void customLogFunction(uint32_t logLevel, const char *message)
{
    if (g_logWindowPtr != 0)
    {
        g_logWindowPtr->logMessage(logLevel, message);
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // install a custom log function so Qt can
    // show us all the messages!
    if (g_logWindowPtr == nullptr)
    {
        g_logWindowPtr = this;
        Cap_installCustomLogFunction(customLogFunction);
    }

    Cap_setLogLevel(8);
    qDebug() << Cap_getLibraryVersion();

    m_ctx = Cap_createContext();
    qDebug() << "Context = " << m_ctx;

    CapDeviceID deviceID = 0;
    CapFormatID formatID = 0;

    m_streamID = Cap_openStream(m_ctx, deviceID, formatID);
    Cap_getFormatInfo(m_ctx, deviceID, formatID, &m_finfo);

    ui->setupUi(this);
    m_frameData.resize(m_finfo.width*m_finfo.height*3);

    ui->frameDisplay->setFixedSize(QSize(m_finfo.width, m_finfo.height));
    ui->frameDisplay->setStyleSheet("border: 1px solid red");

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

    connect(ui->cameraChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera()));

    // add exposure and white balance checkboxes
    connect(ui->autoExposure, SIGNAL(toggled(bool)), this, SLOT(onAutoExposure(bool)));
    connect(ui->autoWhiteBalance, SIGNAL(toggled(bool)),this, SLOT(onAutoWhiteBalance(bool)));
    connect(ui->exposureSlider, SIGNAL(valueChanged(int)),this, SLOT(onExposureSlider(int)));
    connect(ui->whitebalanceSlider, SIGNAL(valueChanged(int)),this, SLOT(onWhiteBalanceSlider(int)));
    connect(ui->autoGain, SIGNAL(toggled(bool)), this, SLOT(onAutoGain(bool)));
    connect(ui->autoFocus, SIGNAL(toggled(bool)), this, SLOT(onAutoFocus(bool)));
    connect(ui->gainSlider, SIGNAL(valueChanged(int)), this, SLOT(onGainSlider(int)));
    connect(ui->contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(onContrastSlider(int)));
    connect(ui->brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(onBrightnessSlider(int)));
    connect(ui->gammaSlider, SIGNAL(valueChanged(int)), this, SLOT(onGammaSlider(int)));
    connect(ui->focusSlider, SIGNAL(valueChanged(int)), this, SLOT(onFocusSlider(int)));
    connect(ui->zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(onZoomSlider(int)));
    connect(ui->hueSlider, SIGNAL(valueChanged(int)), this, SLOT(onHueSlider(int)));
    connect(ui->backlightSlider, SIGNAL(valueChanged(int)), this, SLOT(onBacklightSlider(int)));
    connect(ui->sharpnessSlider, SIGNAL(valueChanged(int)), this, SLOT(onSharpnessSlider(int)));
    connect(ui->powerlineFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(onColorEnableSlider(int)));
    connect(ui->saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(onSaturationSlider(int)));

    // add timer to refresh the frame display
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, SIGNAL(timeout()), this, SLOT(doFrameUpdate()));
    m_refreshTimer->start(50);

    // update GUI to reflect the actual camera settings
    readCameraSettings();
}

MainWindow::~MainWindow()
{
    m_refreshTimer->stop();
    delete m_refreshTimer;

    Cap_closeStream(m_ctx, m_streamID);
    Cap_releaseContext(m_ctx);
    delete ui;

    g_logWindowPtr = nullptr;
}

void MainWindow::logMessage(uint32_t level, const char *message)
{
    QMutexLocker locker(&m_logMutex);
    m_logMessages.push_back(message);
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

        ui->frameDisplay->setPixmap(QPixmap::fromImage(img));

        QString frameInfo = QString::asprintf("%d x %d frames:%d",
                                              m_finfo.width,
                                              m_finfo.height,
                                              Cap_getStreamFrameCount(m_ctx, m_streamID));

        ui->statusBar->showMessage(frameInfo);
    }

    // also update the log messages, if there are some
    updateLogMessages();
}

void MainWindow::updateLogMessages()
{
    QMutexLocker locker(&m_logMutex);
    if (m_logMessages.size() == 0)
    {
        return;
    }

    for(uint32_t i=0; i<m_logMessages.size(); i++)
    {
        // if there is an end-of-line CR,
        // remove it, as appendPlainText already does this
        // by itself ..
        if (m_logMessages[i].at(m_logMessages[i].size()-1)==10)
        {
            m_logMessages[i].erase(m_logMessages[i].size()-1);
        }

        ui->logWidget->appendPlainText(m_logMessages[i].c_str());
    }
    m_logMessages.clear();
}

void MainWindow::changeCamera()
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

    ui->frameDisplay->setFixedSize(QSize(m_finfo.width, m_finfo.height));
    ui->frameDisplay->setStyleSheet("border: 1px solid red");

    readCameraSettings();
}

void MainWindow::onAutoExposure(bool state)
{
    qDebug() << "Auto exposure set to " << state;
    Cap_setAutoProperty(m_ctx, m_streamID, CAPPROPID_EXPOSURE, state ? 1 : 0);
    ui->exposureSlider->setEnabled((!state) & m_hasExposure);
}

void MainWindow::onAutoWhiteBalance(bool state)
{
    qDebug() << "Auto white balance set to " << state;
    Cap_setAutoProperty(m_ctx, m_streamID, CAPPROPID_WHITEBALANCE, state ? 1 : 0);
    ui->whitebalanceSlider->setEnabled((!state) & m_hasWhiteBalance);
}

void MainWindow::onAutoGain(bool state)
{
    qDebug() << "Auto gain set to " << state;
    Cap_setAutoProperty(m_ctx, m_streamID, CAPPROPID_GAIN, state ? 1 : 0);
    ui->gainSlider->setEnabled((!state) & m_hasGain);
}

void MainWindow::onAutoFocus(bool state)
{
    qDebug() << "Auto focus set to " << state;
    Cap_setAutoProperty(m_ctx, m_streamID, CAPPROPID_FOCUS, state ? 1 : 0);
    ui->gainSlider->setEnabled((!state) & m_hasGain);
}

void MainWindow::readCameraSettings()
{
    qDebug() << "readCameraSettings -> Context = " << m_ctx;

    // enable the sliders that also have an AUTO setting.
    // we will disable them later on when we check the
    // AUTO settings.

    m_hasBrightness = false;
    m_hasExposure = false;
    m_hasGamma = false;
    m_hasWhiteBalance = false;
    m_hasGain = false;
    m_hasContrast = false;
    m_hasSaturation = false;
    m_hasFocus = false;
    m_hasZoom = false;
    m_hasColorEnable = false;

    ui->exposureSlider->setEnabled(false);
    ui->gainSlider->setEnabled(false);
    ui->whitebalanceSlider->setEnabled(false);
    ui->focusSlider->setEnabled(false);

    // ********************************************************************************
    //   AUTO EXPOSURE
    // ********************************************************************************

    uint32_t bValue;
    if (Cap_getAutoProperty(m_ctx, m_streamID, CAPPROPID_EXPOSURE, &bValue) != CAPRESULT_OK)
    {
        ui->autoExposure->setEnabled(false);
        ui->autoExposure->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->autoExposure->setEnabled(true);
        ui->autoExposure->setCheckState((bValue==0) ? Qt::Unchecked : Qt::Checked);
    }

    // ********************************************************************************
    //   AUTO GAIN
    // ********************************************************************************

    if (Cap_getAutoProperty(m_ctx, m_streamID, CAPPROPID_GAIN, &bValue) != CAPRESULT_OK)
    {
        ui->autoGain->setEnabled(false);
        ui->autoGain->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->autoGain->setEnabled(true);
        ui->autoGain->setCheckState((bValue==0) ? Qt::Unchecked : Qt::Checked);
    }

    // ********************************************************************************
    //   AUTO WHITE BALANCE
    // ********************************************************************************

    if (Cap_getAutoProperty(m_ctx, m_streamID, CAPPROPID_WHITEBALANCE, &bValue) != CAPRESULT_OK)
    {
        ui->autoWhiteBalance->setEnabled(false);
        ui->autoWhiteBalance->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->autoWhiteBalance->setEnabled(true);
        ui->autoWhiteBalance->setCheckState((bValue==0) ? Qt::Unchecked : Qt::Checked);
    }

    // ********************************************************************************
    //   AUTO FOCUS
    // ********************************************************************************

    if (Cap_getAutoProperty(m_ctx, m_streamID, CAPPROPID_FOCUS, &bValue) != CAPRESULT_OK)
    {
        qDebug() << "Autofocus unsupported";
        ui->autoFocus->setEnabled(false);
        ui->autoFocus->setCheckState(Qt::Unchecked);
    }
    else
    {
        qDebug() << "Autofocus supported";
        ui->autoFocus->setEnabled(true);
        ui->autoFocus->setCheckState((bValue==0) ? Qt::Unchecked : Qt::Checked);
    }

    // ********************************************************************************
    //   EXPOSURE
    // ********************************************************************************

    int32_t emin,emax,edefault;
    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_EXPOSURE, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Exposure min: " << emin;
        qDebug() << "Exposure max: " << emax;
        qDebug() << "Exposure default: " << edefault;
        ui->exposureSlider->setRange(emin, emax);
        m_hasExposure = true;
        if (!ui->autoExposure->isChecked())
        {
            ui->exposureSlider->setEnabled(true);
        }
    }
    else
    {
        qDebug() << "Failed to get exposure limits";
        ui->exposureSlider->setEnabled(false);
        ui->exposureSlider->setRange(0, 0);
    }

    int32_t exposure;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_EXPOSURE, &exposure)==CAPRESULT_OK)
    {
        qDebug() << "Exposure: " << exposure;
        ui->exposureSlider->setValue(exposure);
    }    
    else
    {
        qDebug() << "Failed to get exposure value";
        ui->exposureSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   WHITE BALANCE
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_WHITEBALANCE, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "White balance min: " << emin;
        qDebug() << "White balance max: " << emax;
        qDebug() << "White balance default: " << edefault;
        ui->whitebalanceSlider->setRange(emin, emax);
        m_hasWhiteBalance = true;
        if (!ui->autoWhiteBalance->isChecked())
        {
            ui->whitebalanceSlider->setEnabled(true);
        }
    }
    else
    {
        qDebug() << "Failed to get white balance limits";
        ui->whitebalanceSlider->setRange(0, 0);
        ui->whitebalanceSlider->setEnabled(false);
    }

    int32_t whitebalance;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_WHITEBALANCE, &whitebalance)==CAPRESULT_OK)
    {
        qDebug() << "White Balance: " << whitebalance;
        ui->whitebalanceSlider->setValue(whitebalance);
    }
    else
    {
        qDebug() << "Failed to get gain value";
        ui->whitebalanceSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   GAIN
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_GAIN, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Gain min: " << emin;
        qDebug() << "Gain max: " << emax;
        qDebug() << "Gain default: " << edefault;
        ui->gainSlider->setRange(emin, emax);
        m_hasGain = true;
        if (!ui->autoGain->isChecked())
        {
            ui->gainSlider->setEnabled(true);
        }
    }
    else
    {
        qDebug() << "Failed to get gain limits";
        ui->gainSlider->setRange(0, 0);
        ui->gainSlider->setEnabled(false);
    }

    int32_t gain;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_GAIN, &gain)==CAPRESULT_OK)
    {
        qDebug() << "Gain: " << gain;
        ui->gainSlider->setValue(gain);
    }
    else
    {
        qDebug() << "Failed to get gain value";
        ui->gainSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   CONTRAST
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_CONTRAST, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Contrast min: " << emin;
        qDebug() << "Contrast max: " << emax;
        qDebug() << "Contrast default: " << edefault;
        ui->contrastSlider->setEnabled(true);
        ui->contrastSlider->setRange(emin, emax);
        m_hasContrast = true;
    }
    else
    {
        ui->contrastSlider->setRange(0, 0);
        ui->contrastSlider->setEnabled(false);
    }

    int32_t contrast;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_CONTRAST, &contrast)==CAPRESULT_OK)
    {
        qDebug() << "Contrast: " << contrast;
        ui->contrastSlider->setValue(contrast);
    }
    else
    {
        qDebug() << "Failed to get contrast value";
        ui->contrastSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   BRIGHTNESS
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_BRIGHTNESS, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Brightness min: " << emin;
        qDebug() << "Brightness max: " << emax;
        qDebug() << "Brightness default: " << edefault;
        ui->brightnessSlider->setEnabled(true);
        ui->brightnessSlider->setRange(emin, emax);
        m_hasBrightness = true;
    }
    else
    {
        ui->brightnessSlider->setRange(0, 0);
        ui->brightnessSlider->setEnabled(false);
    }

    int32_t brightness;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_BRIGHTNESS, &brightness)==CAPRESULT_OK)
    {
        qDebug() << "Brightness: " << brightness;
        ui->brightnessSlider->setEnabled(true);
        ui->brightnessSlider->setValue(brightness);
    }
    else
    {
        qDebug() << "Failed to get brightness value";
        ui->brightnessSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   GAMMA
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_GAMMA, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Gamma min: " << emin;
        qDebug() << "Gamma max: " << emax;
        qDebug() << "Gamma default: " << edefault;
        ui->gammaSlider->setEnabled(true);
        ui->gammaSlider->setRange(emin, emax);
        m_hasGamma = true;
    }
    else
    {
        ui->gammaSlider->setRange(0, 0);
        ui->gammaSlider->setEnabled(false);
    }

    int32_t gamma;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_GAMMA, &gamma)==CAPRESULT_OK)
    {
        qDebug() << "Gamma: " << gamma;
        ui->gammaSlider->setEnabled(true);
        ui->gammaSlider->setValue(gamma);
    }
    else
    {
        qDebug() << "Failed to get gamma value";
        ui->gammaSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   SATURATION
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_SATURATION, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "Saturation min: " << emin;
        qDebug() << "Saturation max: " << emax;
        qDebug() << "Saturation default: " << edefault;
        ui->saturationSlider->setEnabled(true);
        ui->saturationSlider->setRange(emin, emax);
        m_hasSaturation = true;
    }
    else
    {
        ui->saturationSlider->setRange(0, 0);
        ui->saturationSlider->setEnabled(false);
    }

    int32_t saturation;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_SATURATION, &saturation)==CAPRESULT_OK)
    {
        qDebug() << "Saturation: " << saturation;
        ui->saturationSlider->setEnabled(true);
        ui->saturationSlider->setValue(saturation);
    }
    else
    {
        qDebug() << "Failed to get saturation value";
        ui->saturationSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   ZOOM
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_ZOOM, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "zoom min: " << emin;
        qDebug() << "zoom max: " << emax;
        qDebug() << "zoom default: " << edefault;
        ui->zoomSlider->setEnabled(true);
        ui->zoomSlider->setRange(emin, emax);
        m_hasZoom = true;
    }
    else
    {
        ui->zoomSlider->setRange(0, 0);
        ui->zoomSlider->setEnabled(false);
    }

    int32_t zoom;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_ZOOM, &zoom)==CAPRESULT_OK)
    {
        qDebug() << "zoom: " << zoom;
        ui->zoomSlider->setEnabled(true);
        ui->zoomSlider->setValue(zoom);
    }
    else
    {
        qDebug() << "Failed to get zoom value";
        ui->zoomSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   FOCUS
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_FOCUS, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "focus min: " << emin;
        qDebug() << "focus max: " << emax;
        qDebug() << "focus default: " << edefault;
        ui->focusSlider->setEnabled(true);
        ui->focusSlider->setRange(emin, emax);
        m_hasFocus = true;
    }
    else
    {
        ui->focusSlider->setRange(0, 0);
        ui->focusSlider->setEnabled(false);
    }

    int32_t focus;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_FOCUS, &focus)==CAPRESULT_OK)
    {
        qDebug() << "focus: " << focus;
        ui->focusSlider->setEnabled(true);
        ui->focusSlider->setValue(focus);
    }
    else
    {
        qDebug() << "Failed to get focus value";
        ui->focusSlider->setEnabled(false);
    }


    // ********************************************************************************
    //   HUE
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_HUE, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "hue min: " << emin;
        qDebug() << "hue max: " << emax;
        qDebug() << "hue default: " << edefault;
        ui->hueSlider->setEnabled(true);
        ui->hueSlider->setRange(emin, emax);
        m_hasHue = true;
    }
    else
    {
        ui->hueSlider->setRange(0, 0);
        ui->hueSlider->setEnabled(false);
    }

    int32_t hue;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_HUE, &hue)==CAPRESULT_OK)
    {
        qDebug() << "hue: " << hue;
        ui->hueSlider->setEnabled(true);
        ui->hueSlider->setValue(hue);
    }
    else
    {
        qDebug() << "Failed to get hue value";
        ui->hueSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   SHARPNESS
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_SHARPNESS, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "sharpness min: " << emin;
        qDebug() << "sharpness max: " << emax;
        qDebug() << "sharpness default: " << edefault;
        ui->sharpnessSlider->setEnabled(true);
        ui->sharpnessSlider->setRange(emin, emax);
        m_hasSharpness = true;
    }
    else
    {
        ui->sharpnessSlider->setRange(0, 0);
        ui->sharpnessSlider->setEnabled(false);
    }

    int32_t sharpness;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_SHARPNESS, &sharpness)==CAPRESULT_OK)
    {
        qDebug() << "sharpness: " << sharpness;
        ui->sharpnessSlider->setEnabled(true);
        ui->sharpnessSlider->setValue(sharpness);
    }
    else
    {
        qDebug() << "Failed to get sharpness value";
        ui->sharpnessSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   BACKLIGHT COMP
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_BACKLIGHTCOMP, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "backlightcomp min: " << emin;
        qDebug() << "backlightcomp max: " << emax;
        qDebug() << "backlightcomp default: " << edefault;
        ui->backlightSlider->setEnabled(true);
        ui->backlightSlider->setRange(emin, emax);
        m_hasBacklightcomp = true;
    }
    else
    {
        ui->backlightSlider->setRange(0, 0);
        ui->backlightSlider->setEnabled(false);
    }

    int32_t backlight;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_BACKLIGHTCOMP, &backlight)==CAPRESULT_OK)
    {
        qDebug() << "backlight: " << backlight;
        ui->backlightSlider->setEnabled(true);
        ui->backlightSlider->setValue(backlight);
    }
    else
    {
        qDebug() << "Failed to get backlight value";
        ui->backlightSlider->setEnabled(false);
    }

    // ********************************************************************************
    //   COLOR ENABLE
    // ********************************************************************************

    if (Cap_getPropertyLimits(m_ctx, m_streamID, CAPPROPID_POWERLINEFREQ, &emin, &emax, &edefault)==CAPRESULT_OK)
    {
        qDebug() << "power line freq min: " << emin;
        qDebug() << "power line freq max: " << emax;
        qDebug() << "power line freq default: " << edefault;
        ui->powerlineFreqSlider->setEnabled(true);
        ui->powerlineFreqSlider->setRange(emin, emax);
        m_hasBacklightcomp = true;
    }
    else
    {
        ui->powerlineFreqSlider->setRange(0, 0);
        ui->powerlineFreqSlider->setEnabled(false);
    }

    int32_t powerline;
    if (Cap_getProperty(m_ctx, m_streamID, CAPPROPID_POWERLINEFREQ, &powerline)==CAPRESULT_OK)
    {
        qDebug() << "power line : " << powerline;
        ui->powerlineFreqSlider->setEnabled(true);
        ui->powerlineFreqSlider->setValue(powerline);
    }
    else
    {
        qDebug() << "Failed to get power line frequency value";
        ui->powerlineFreqSlider->setEnabled(false);
    }

}

void MainWindow::onExposureSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_EXPOSURE, value);
}

void MainWindow::onWhiteBalanceSlider(int value)
{
    if (Cap_setProperty(m_ctx, m_streamID, CAPPROPID_WHITEBALANCE, value) != CAPRESULT_OK)
    {
        qDebug() << "Setting white balance failed";
    }
}

void MainWindow::onGainSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_GAIN, value);
}

void MainWindow::onContrastSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_CONTRAST, value);
}

void MainWindow::onBrightnessSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_BRIGHTNESS, value);
}

void MainWindow::onGammaSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_GAMMA, value);
}

void MainWindow::onSaturationSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_SATURATION, value);
}

void MainWindow::onFocusSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_FOCUS, value);
}

void MainWindow::onZoomSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_ZOOM, value);
}

void MainWindow::onHueSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_HUE, value);
}

void MainWindow::onBacklightSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_BACKLIGHTCOMP, value);
}

void MainWindow::onSharpnessSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_SHARPNESS, value);
}

void MainWindow::onColorEnableSlider(int value)
{
    Cap_setProperty(m_ctx, m_streamID, CAPPROPID_POWERLINEFREQ, value);
}
