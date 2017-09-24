#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include <vector>
#include "openpnp-capture.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void readCameraSettings();

public slots:
    void doFrameUpdate();
    void changeCamera(int index);

    void onAutoExposure(bool state);
    void onAutoWhiteBalance(bool state);
    void onExposureSlider(int value);
    void onWhiteBalanceSlider(int value);

private:
    CapFormatInfo           m_finfo;
    std::vector<uint8_t>    m_frameData;
    QLabel*     m_frameDisplay;

    QCheckBox*  m_autoWhiteBalance;
    QCheckBox*  m_autoExposure;
    QSlider*    m_exposureSlider;
    QSlider*    m_whiteBalanceSlider;


    QTimer*     m_refreshTimer;
    CapContext  m_ctx;
    int32_t     m_streamID;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
