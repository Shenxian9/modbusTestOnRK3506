#include "mainwindow.h"

#include <QSerialPort>

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QMessageBox>
#include <QStringList>

#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_serialPort(new QSerialPort(this))
{
    setupUi();
    setupConnections();
}

MainWindow::~MainWindow()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Modbus RTU RS485 Tester"));
    resize(960, 700);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *serialGroup = new QGroupBox(QStringLiteral("串口配置"), central);
    auto *serialLayout = new QGridLayout(serialGroup);

    m_portEdit = new QLineEdit(QStringLiteral("/dev/ttyS2"), serialGroup);

    m_baudSpin = new QSpinBox(serialGroup);
    m_baudSpin->setRange(1200, 4000000);
    m_baudSpin->setValue(9600);

    m_slaveSpin = new QSpinBox(serialGroup);
    m_slaveSpin->setRange(1, 247);
    m_slaveSpin->setValue(1);

    m_openButton = new QPushButton(QStringLiteral("打开串口"), serialGroup);
    m_closeButton = new QPushButton(QStringLiteral("关闭串口"), serialGroup);
    m_statusLabel = new QLabel(QStringLiteral("已关闭"), serialGroup);

    serialLayout->addWidget(new QLabel(QStringLiteral("端口名:"), serialGroup), 0, 0);
    serialLayout->addWidget(m_portEdit, 0, 1);
    serialLayout->addWidget(new QLabel(QStringLiteral("波特率:"), serialGroup), 0, 2);
    serialLayout->addWidget(m_baudSpin, 0, 3);
    serialLayout->addWidget(new QLabel(QStringLiteral("从站地址:"), serialGroup), 0, 4);
    serialLayout->addWidget(m_slaveSpin, 0, 5);
    serialLayout->addWidget(m_openButton, 1, 0, 1, 2);
    serialLayout->addWidget(m_closeButton, 1, 2, 1, 2);
    serialLayout->addWidget(new QLabel(QStringLiteral("状态:"), serialGroup), 1, 4);
    serialLayout->addWidget(m_statusLabel, 1, 5);

    auto *readGroup = new QGroupBox(QStringLiteral("读寄存器操作"), central);
    auto *readLayout = new QGridLayout(readGroup);

    m_startAddrSpin = new QSpinBox(readGroup);
    m_startAddrSpin->setRange(0, 65535);
    m_startAddrSpin->setValue(8192);

    m_quantitySpin = new QSpinBox(readGroup);
    m_quantitySpin->setRange(1, 125);
    m_quantitySpin->setValue(2);

    m_readButton = new QPushButton(QStringLiteral("读取保持寄存器"), readGroup);

    readLayout->addWidget(new QLabel(QStringLiteral("起始地址:"), readGroup), 0, 0);
    readLayout->addWidget(m_startAddrSpin, 0, 1);
    readLayout->addWidget(new QLabel(QStringLiteral("数量:"), readGroup), 0, 2);
    readLayout->addWidget(m_quantitySpin, 0, 3);
    readLayout->addWidget(m_readButton, 0, 4);

    auto *quickGroup = new QGroupBox(QStringLiteral("快捷与调试"), central);
    auto *quickLayout = new QHBoxLayout(quickGroup);

    m_readFlowRateButton = new QPushButton(QStringLiteral("读取瞬时流量"), quickGroup);
    m_readFlowVelocityButton = new QPushButton(QStringLiteral("读取瞬时流速"), quickGroup);
    m_injectFrameButton = new QPushButton(QStringLiteral("注入测试帧"), quickGroup);
    m_clearLogButton = new QPushButton(QStringLiteral("清空日志"), quickGroup);

    quickLayout->addWidget(m_readFlowRateButton);
    quickLayout->addWidget(m_readFlowVelocityButton);
    quickLayout->addWidget(m_injectFrameButton);
    quickLayout->addWidget(m_clearLogButton);
    quickLayout->addStretch();

    auto *resultGroup = new QGroupBox(QStringLiteral("结果显示"), central);
    auto *resultLayout = new QGridLayout(resultGroup);

    m_lastFrameLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastRegsLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastFloatLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastExceptionLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastCrcLabel = new QLabel(QStringLiteral("-"), resultGroup);

    m_lastFrameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastRegsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastFloatLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastExceptionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastCrcLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    resultLayout->addWidget(new QLabel(QStringLiteral("最近完整帧:"), resultGroup), 0, 0);
    resultLayout->addWidget(m_lastFrameLabel, 0, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近寄存器值:"), resultGroup), 1, 0);
    resultLayout->addWidget(m_lastRegsLabel, 1, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近Float值:"), resultGroup), 2, 0);
    resultLayout->addWidget(m_lastFloatLabel, 2, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近异常码:"), resultGroup), 3, 0);
    resultLayout->addWidget(m_lastExceptionLabel, 3, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近CRC状态:"), resultGroup), 4, 0);
    resultLayout->addWidget(m_lastCrcLabel, 4, 1);

    auto *logGroup = new QGroupBox(QStringLiteral("日志"), central);
    auto *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QPlainTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    logLayout->addWidget(m_logEdit);

    mainLayout->addWidget(serialGroup);
    mainLayout->addWidget(readGroup);
    mainLayout->addWidget(quickGroup);
    mainLayout->addWidget(resultGroup);
    mainLayout->addWidget(logGroup, 1);

    setCentralWidget(central);
}

void MainWindow::setupConnections()
{
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::openSerialPort);
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::closeSerialPort);
    connect(m_readButton, &QPushButton::clicked, this, &MainWindow::onReadHoldingRegisters);
    connect(m_readFlowRateButton, &QPushButton::clicked, this, &MainWindow::onReadFlowRate);
    connect(m_readFlowVelocityButton, &QPushButton::clicked, this, &MainWindow::onReadFlowVelocity);
    connect(m_injectFrameButton, &QPushButton::clicked, this, &MainWindow::injectTestFrame);
    connect(m_clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);

    connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &MainWindow::onSerialErrorOccurred);
}

void MainWindow::appendLog(const QString &text)
{
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    m_logEdit->appendPlainText(QStringLiteral("[%1] %2").arg(ts, text));
}

void MainWindow::openSerialPort()
{
    if (m_serialPort->isOpen()) {
        appendLog(QStringLiteral("串口已处于打开状态。"));
        return;
    }

    m_serialPort->setPortName(m_portEdit->text().trimmed());
    m_serialPort->setBaudRate(m_baudSpin->value());
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_statusLabel->setText(QStringLiteral("已打开"));
        appendLog(QStringLiteral("串口打开成功: %1, %2 8N1")
                      .arg(m_serialPort->portName())
                      .arg(m_baudSpin->value()));
    } else {
        m_statusLabel->setText(QStringLiteral("打开失败"));
        appendLog(QStringLiteral("串口打开失败: %1").arg(m_serialPort->errorString()));
    }
}

void MainWindow::closeSerialPort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    m_statusLabel->setText(QStringLiteral("已关闭"));
    appendLog(QStringLiteral("串口已关闭。"));
}

void MainWindow::onReadHoldingRegisters()
{
    const quint8 slave = static_cast<quint8>(m_slaveSpin->value());
    const quint16 startAddr = static_cast<quint16>(m_startAddrSpin->value());
    const quint16 quantity = static_cast<quint16>(m_quantitySpin->value());
    sendReadRequest(slave, startAddr, quantity);
}

void MainWindow::onReadFlowRate()
{
    m_startAddrSpin->setValue(8192);
    m_quantitySpin->setValue(2);
    onReadHoldingRegisters();
}

void MainWindow::onReadFlowVelocity()
{
    m_startAddrSpin->setValue(8194);
    m_quantitySpin->setValue(2);
    onReadHoldingRegisters();
}

void MainWindow::onReadyRead()
{
    const qint64 avail = m_serialPort->bytesAvailable();
    appendLog(QStringLiteral("readyRead触发, bytesAvailable=%1").arg(avail));

    const QByteArray chunk = m_serialPort->readAll();
    appendLog(QStringLiteral("readAll[%1]: %2").arg(chunk.size()).arg(bytesToHexString(chunk)));

    m_rxBuffer.append(chunk);
    appendLog(QStringLiteral("RX缓冲区[%1]: %2").arg(m_rxBuffer.size()).arg(bytesToHexString(m_rxBuffer)));

    processRxBuffer();
}

void MainWindow::onSerialErrorOccurred()
{
    if (m_serialPort->error() == QSerialPort::NoError) {
        return;
    }

    appendLog(QStringLiteral("串口错误: %1").arg(m_serialPort->errorString()));
}

void MainWindow::injectTestFrame()
{
    const QByteArray frame = QByteArray::fromHex("01030400000000FA33");
    appendLog(QStringLiteral("注入测试帧: %1").arg(bytesToHexString(frame)));

    m_rxBuffer.append(frame);
    appendLog(QStringLiteral("RX缓冲区(注入后)[%1]: %2").arg(m_rxBuffer.size()).arg(bytesToHexString(m_rxBuffer)));

    processRxBuffer();
}

void MainWindow::clearLog()
{
    m_logEdit->clear();
    appendLog(QStringLiteral("日志已清空。"));
}

bool MainWindow::sendReadRequest(quint8 slave, quint16 startAddr, quint16 quantity)
{
    if (!m_serialPort->isOpen()) {
        const QString msg = QStringLiteral("串口未打开，无法发送读取请求。");
        appendLog(msg);
        QMessageBox::warning(this, QStringLiteral("提示"), msg);
        return false;
    }

    const QByteArray request = buildReadHoldingRegistersRequest(slave, startAddr, quantity);
    const qint64 written = m_serialPort->write(request);
    if (written != request.size()) {
        appendLog(QStringLiteral("发送失败，实际写入字节数: %1/%2").arg(written).arg(request.size()));
        return false;
    }

    appendLog(QStringLiteral("TX: %1").arg(bytesToHexString(request)));
    return true;
}

QString MainWindow::bytesToHexString(const QByteArray &data)
{
    QStringList parts;
    parts.reserve(data.size());

    for (const char b : data) {
        parts << QStringLiteral("%1").arg(static_cast<quint8>(b), 2, 16, QLatin1Char('0')).toUpper();
    }

    return parts.join(QLatin1Char(' '));
}

quint16 MainWindow::calcModbusCrc(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    for (const char b : data) {
        crc ^= static_cast<quint8>(b);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            } else {
                crc = static_cast<quint16>(crc >> 1);
            }
        }
    }

    return crc;
}

QByteArray MainWindow::buildReadHoldingRegistersRequest(quint8 slave, quint16 startAddr, quint16 quantity)
{
    QByteArray frame;
    frame.reserve(8);

    frame.append(static_cast<char>(slave));
    frame.append(static_cast<char>(0x03));
    frame.append(static_cast<char>((startAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddr & 0xFF));
    frame.append(static_cast<char>((quantity >> 8) & 0xFF));
    frame.append(static_cast<char>(quantity & 0xFF));

    const quint16 crc = calcModbusCrc(frame);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));

    return frame;
}

void MainWindow::processRxBuffer()
{
    QByteArray frame;
    while (tryExtractOneFrame(frame)) {
        appendLog(QStringLiteral("提取完整帧: %1").arg(bytesToHexString(frame)));
        parseModbusResponse(frame);
    }
}

bool MainWindow::tryExtractOneFrame(QByteArray &frame)
{
    frame.clear();

    while (true) {
        if (m_rxBuffer.size() < 5) {
            return false;
        }

        const quint8 expectedSlave = static_cast<quint8>(m_slaveSpin->value());

        if (static_cast<quint8>(m_rxBuffer.at(0)) != expectedSlave) {
            const quint8 dropped = static_cast<quint8>(m_rxBuffer.at(0));
            appendLog(QStringLiteral("重同步: 地址不匹配(期望=%1 实际=0x%2), 丢弃1字节")
                          .arg(expectedSlave)
                          .arg(QString::number(dropped, 16).toUpper().rightJustified(2, QLatin1Char('0'))));
            m_rxBuffer.remove(0, 1);
            continue;
        }

        const quint8 func = static_cast<quint8>(m_rxBuffer.at(1));
        int frameLen = -1;

        if (func == 0x03) {
            if (m_rxBuffer.size() < 3) {
                return false;
            }
            const quint8 byteCount = static_cast<quint8>(m_rxBuffer.at(2));
            frameLen = 3 + byteCount + 2;
        } else if (func == 0x83) {
            frameLen = 5;
        } else {
            appendLog(QStringLiteral("重同步: 未知/暂不支持功能码0x%1, 丢弃1字节")
                          .arg(QString::number(func, 16).toUpper().rightJustified(2, QLatin1Char('0'))));
            m_rxBuffer.remove(0, 1);
            continue;
        }

        if (frameLen < 5) {
            appendLog(QStringLiteral("重同步: 计算得到非法帧长%1, 丢弃1字节").arg(frameLen));
            m_rxBuffer.remove(0, 1);
            continue;
        }

        if (m_rxBuffer.size() < frameLen) {
            appendLog(QStringLiteral("半帧等待: 当前缓冲%1字节, 期望%2字节").arg(m_rxBuffer.size()).arg(frameLen));
            return false;
        }

        const QByteArray candidate = m_rxBuffer.left(frameLen);
        const QByteArray body = candidate.left(frameLen - 2);
        const quint16 crcCalc = calcModbusCrc(body);
        const quint16 crcRecv = static_cast<quint16>(static_cast<quint8>(candidate.at(frameLen - 2))) |
                                static_cast<quint16>(static_cast<quint8>(candidate.at(frameLen - 1)) << 8);

        if (crcCalc != crcRecv) {
            appendLog(QStringLiteral("CRC 校验失败(提取阶段): calc=0x%1 recv=0x%2, 丢弃首字节重同步")
                          .arg(QString::number(crcCalc, 16).toUpper().rightJustified(4, QLatin1Char('0')))
                          .arg(QString::number(crcRecv, 16).toUpper().rightJustified(4, QLatin1Char('0'))));
            m_lastCrcLabel->setText(QStringLiteral("失败"));
            m_rxBuffer.remove(0, 1);
            continue;
        }

        appendLog(QStringLiteral("CRC 校验通过: 0x%1")
                      .arg(QString::number(crcRecv, 16).toUpper().rightJustified(4, QLatin1Char('0'))));
        m_lastCrcLabel->setText(QStringLiteral("通过"));

        frame = candidate;
        m_rxBuffer.remove(0, frameLen);
        appendLog(QStringLiteral("提取后剩余缓冲[%1]: %2").arg(m_rxBuffer.size()).arg(bytesToHexString(m_rxBuffer)));
        return true;
    }
}

void MainWindow::parseModbusResponse(const QByteArray &frame)
{
    if (frame.size() < 5) {
        appendLog(QStringLiteral("解析失败: 帧长度不足(最小5字节)。"));
        return;
    }

    m_lastFrameLabel->setText(bytesToHexString(frame));

    const quint8 slave = static_cast<quint8>(frame.at(0));
    const quint8 func = static_cast<quint8>(frame.at(1));

    if (func == 0x03) {
        const quint8 byteCount = static_cast<quint8>(frame.at(2));
        appendLog(QStringLiteral("解析正常响应: slave=%1 func=0x03 byteCount=%2").arg(slave).arg(byteCount));

        if (frame.size() != 3 + byteCount + 2) {
            appendLog(QStringLiteral("解析失败: 帧长与byteCount不匹配, frame=%1 expected=%2")
                          .arg(frame.size())
                          .arg(3 + byteCount + 2));
            return;
        }

        if ((byteCount % 2) != 0U) {
            appendLog(QStringLiteral("解析失败: byteCount=%1 不是偶数").arg(byteCount));
            return;
        }

        QList<quint16> regs;
        regs.reserve(byteCount / 2);
        for (int i = 0; i < byteCount; i += 2) {
            const quint16 reg = static_cast<quint16>(static_cast<quint8>(frame.at(3 + i)) << 8)
                                | static_cast<quint16>(static_cast<quint8>(frame.at(3 + i + 1)));
            regs.append(reg);
        }

        QStringList regList;
        for (int i = 0; i < regs.size(); ++i) {
            regList << QStringLiteral("R%1=0x%2(%3)")
                           .arg(i + 1)
                           .arg(QString::number(regs.at(i), 16).toUpper().rightJustified(4, QLatin1Char('0')))
                           .arg(regs.at(i));
        }

        m_lastRegsLabel->setText(regList.join(QStringLiteral(", ")));
        m_lastExceptionLabel->setText(QStringLiteral("-"));
        appendLog(QStringLiteral("寄存器解析: %1").arg(m_lastRegsLabel->text()));

        if (regs.size() == 2) {
            const float f = registersToBigEndianFloat(regs.at(0), regs.at(1));
            m_lastFloatLabel->setText(QString::number(f, 'g', 10));
            appendLog(QStringLiteral("Big-Endian Float解析: %1").arg(m_lastFloatLabel->text()));
        } else {
            m_lastFloatLabel->setText(QStringLiteral("-"));
        }

        return;
    }

    if ((func & 0x80) != 0U) {
        if (frame.size() != 5) {
            appendLog(QStringLiteral("异常响应长度错误: 实际%1 期望5").arg(frame.size()));
            return;
        }

        const quint8 ex = static_cast<quint8>(frame.at(2));
        const QString exText = QStringLiteral("0x%1")
                                   .arg(QString::number(ex, 16).toUpper().rightJustified(2, QLatin1Char('0')));
        m_lastExceptionLabel->setText(exText);
        m_lastRegsLabel->setText(QStringLiteral("-"));
        m_lastFloatLabel->setText(QStringLiteral("-"));

        appendLog(QStringLiteral("异常响应: slave=%1 func=0x%2 exception=%3")
                      .arg(slave)
                      .arg(QString::number(func, 16).toUpper().rightJustified(2, QLatin1Char('0')))
                      .arg(exText));
        return;
    }

    appendLog(QStringLiteral("未处理功能码: 0x%1").arg(QString::number(func, 16).toUpper()));
}

float MainWindow::registersToBigEndianFloat(quint16 regHi, quint16 regLo)
{
    const quint32 raw = (static_cast<quint32>(regHi) << 16) | static_cast<quint32>(regLo);
    float value = 0.0f;
    static_assert(sizeof(float) == sizeof(quint32), "float must be 4 bytes");
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}
