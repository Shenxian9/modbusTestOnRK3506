#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class QSerialPort;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void onReadHoldingRegisters();
    void onReadFlowRate();
    void onReadFlowVelocity();
    void onReadyRead();
    void onSerialErrorOccurred();
    void injectTestFrame();
    void clearLog();

private:
    void setupUi();
    void setupConnections();
    void appendLog(const QString &text);

    QString bytesToHexString(const QByteArray &data);
    quint16 calcModbusCrc(const QByteArray &data);
    QByteArray buildReadHoldingRegistersRequest(quint8 slave, quint16 startAddr, quint16 quantity);

    void processRxBuffer();
    bool tryExtractOneFrame(QByteArray &frame);
    void parseModbusResponse(const QByteArray &frame);
    float registersToBigEndianFloat(quint16 regHi, quint16 regLo);

    bool sendReadRequest(quint8 slave, quint16 startAddr, quint16 quantity);

private:
    QSerialPort *m_serialPort;
    QByteArray m_rxBuffer;

    QLineEdit *m_portEdit;
    QSpinBox *m_baudSpin;
    QSpinBox *m_slaveSpin;
    QPushButton *m_openButton;
    QPushButton *m_closeButton;
    QLabel *m_statusLabel;

    QSpinBox *m_startAddrSpin;
    QSpinBox *m_quantitySpin;
    QPushButton *m_readButton;

    QPushButton *m_readFlowRateButton;
    QPushButton *m_readFlowVelocityButton;
    QPushButton *m_injectFrameButton;
    QPushButton *m_clearLogButton;

    QPlainTextEdit *m_logEdit;

    QLabel *m_lastFrameLabel;
    QLabel *m_lastRegsLabel;
    QLabel *m_lastFloatLabel;
    QLabel *m_lastExceptionLabel;
    QLabel *m_lastCrcLabel;
};

#endif // MAINWINDOW_H
