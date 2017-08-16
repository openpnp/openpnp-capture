#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
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

public slots:
    void doFrameUpdate();
    void changeCamera(int index);

private:
    CapFormatInfo           m_finfo;
    std::vector<uint8_t>    m_frameData;
    QLabel*     m_frameDisplay;

    QTimer*     m_refreshTimer;
    CapContext  m_ctx;
    int32_t     m_streamID;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
