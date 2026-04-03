#include "mainwindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>

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
    resize(900, 650);

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

    auto *quickGroup = new QGroupBox(QStringLiteral("快捷读取"), central);
    auto *quickLayout = new QHBoxLayout(quickGroup);

    m_readFlowRateButton = new QPushButton(QStringLiteral("读取瞬时流量"), quickGroup);
    m_readFlowVelocityButton = new QPushButton(QStringLiteral("读取瞬时流速"), quickGroup);
    quickLayout->addWidget(m_readFlowRateButton);
    quickLayout->addWidget(m_readFlowVelocityButton);
    quickLayout->addStretch();

    auto *resultGroup = new QGroupBox(QStringLiteral("结果显示"), central);
    auto *resultLayout = new QGridLayout(resultGroup);

    m_lastRegsLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastRegsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastFloatLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastFloatLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastExceptionLabel = new QLabel(QStringLiteral("-"), resultGroup);
    m_lastExceptionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    resultLayout->addWidget(new QLabel(QStringLiteral("最近寄存器值:"), resultGroup), 0, 0);
    resultLayout->addWidget(m_lastRegsLabel, 0, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近Float值:"), resultGroup), 1, 0);
    resultLayout->addWidget(m_lastFloatLabel, 1, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("最近异常码:"), resultGroup), 2, 0);
    resultLayout->addWidget(m_lastExceptionLabel, 2, 1);

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

    connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::onSerialReadyRead);
    connect(m_serialPort,
            &QSerialPort::errorOccurred,
            this,
            &MainWindow::onSerialErrorOccurred);
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
        appendLog(QStringLiteral("发送失败，实际写入字节数: %1/%2")
                      .arg(written)
                      .arg(request.size()));
        return false;
    }

    m_lastSlave = slave;
    m_lastStartAddr = startAddr;
    m_lastQuantity = quantity;
    m_waitingResponse = true;

    appendLog(QStringLiteral("读取请求: slave=%1, start=%2, qty=%3")
                  .arg(slave)
                  .arg(startAddr)
                  .arg(quantity));
    appendLog(QStringLiteral("TX: %1").arg(bytesToHexString(request)));

    return true;
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

void MainWindow::onSerialReadyRead()
{
    m_rxBuffer.append(m_serialPort->readAll());
    handleReceivedData();
}

void MainWindow::onSerialErrorOccurred()
{
    if (m_serialPort->error() == QSerialPort::NoError) {
        return;
    }

    appendLog(QStringLiteral("串口错误: %1").arg(m_serialPort->errorString()));
}

void MainWindow::handleReceivedData()
{
    while (true) {
        if (m_rxBuffer.size() < 5) {
            return;
        }

        const quint8 addr = static_cast<quint8>(m_rxBuffer.at(0));
        const quint8 func = static_cast<quint8>(m_rxBuffer.at(1));

        int expectedLen = -1;
        if ((func & 0x80) != 0U) {
            expectedLen = 5;
        } else if (func == 0x03) {
            const quint8 byteCount = static_cast<quint8>(m_rxBuffer.at(2));
            expectedLen = 3 + byteCount + 2;
        } else {
            appendLog(QStringLiteral("收到未知功能码响应: addr=%1 func=0x%2，丢弃1字节重同步")
                          .arg(addr)
                          .arg(QString::number(func, 16).toUpper()));
            m_rxBuffer.remove(0, 1);
            continue;
        }

        if (expectedLen <= 0) {
            appendLog(QStringLiteral("响应长度计算异常。"));
            return;
        }

        if (m_rxBuffer.size() < expectedLen) {
            return;
        }

        const QByteArray frame = m_rxBuffer.left(expectedLen);
        m_rxBuffer.remove(0, expectedLen);

        appendLog(QStringLiteral("RX: %1").arg(bytesToHexString(frame)));
        parseModbusResponse(frame);
        m_waitingResponse = false;
    }
}

void MainWindow::parseModbusResponse(const QByteArray &frame)
{
    if (frame.size() < 5) {
        appendLog(QStringLiteral("响应长度不足，至少应为5字节。"));
        return;
    }

    const QByteArray payload = frame.left(frame.size() - 2);
    const quint16 crcCalc = calcModbusCrc(payload);
    const quint16 crcRecv = static_cast<quint16>(static_cast<quint8>(frame.at(frame.size() - 2))) |
                            static_cast<quint16>(static_cast<quint8>(frame.at(frame.size() - 1)) << 8);

    if (crcCalc != crcRecv) {
        appendLog(QStringLiteral("CRC 校验失败: calc=0x%1 recv=0x%2")
                      .arg(QString::number(crcCalc, 16).toUpper().rightJustified(4, QLatin1Char('0')))
                      .arg(QString::number(crcRecv, 16).toUpper().rightJustified(4, QLatin1Char('0'))));
        return;
    }

    const quint8 slave = static_cast<quint8>(frame.at(0));
    const quint8 func = static_cast<quint8>(frame.at(1));

    if ((func & 0x80) != 0U) {
        if (frame.size() != 5) {
            appendLog(QStringLiteral("异常响应长度错误，期望5字节，实际%1字节。").arg(frame.size()));
            return;
        }

        const quint8 exceptionCode = static_cast<quint8>(frame.at(2));
        m_lastExceptionLabel->setText(QStringLiteral("0x%1")
                                          .arg(QString::number(exceptionCode, 16).toUpper().rightJustified(2, QLatin1Char('0'))));
        appendLog(QStringLiteral("Modbus异常响应: slave=%1 func=0x%2 exception=0x%3")
                      .arg(slave)
                      .arg(QString::number(func, 16).toUpper())
                      .arg(QString::number(exceptionCode, 16).toUpper().rightJustified(2, QLatin1Char('0'))));
        return;
    }

    if (func != 0x03) {
        appendLog(QStringLiteral("暂不支持的正常响应功能码: 0x%1").arg(QString::number(func, 16).toUpper()));
        return;
    }

    const quint8 byteCount = static_cast<quint8>(frame.at(2));
    if (frame.size() != byteCount + 5) {
        appendLog(QStringLiteral("响应长度不足或不匹配: byteCount=%1, frame=%2")
                      .arg(byteCount)
                      .arg(frame.size()));
        return;
    }

    if ((byteCount % 2) != 0U) {
        appendLog(QStringLiteral("数据字节数不是偶数，无法按寄存器解析: %1").arg(byteCount));
        return;
    }

    QList<quint16> regs;
    regs.reserve(byteCount / 2);
    for (int i = 0; i < byteCount; i += 2) {
        const quint16 reg = static_cast<quint16>(static_cast<quint8>(frame.at(3 + i)) << 8) |
                            static_cast<quint16>(static_cast<quint8>(frame.at(3 + i + 1)));
        regs.append(reg);
    }

    QStringList regTextList;
    for (int i = 0; i < regs.size(); ++i) {
        regTextList << QStringLiteral("R%1=0x%2(%3)")
                           .arg(i)
                           .arg(QString::number(regs.at(i), 16).toUpper().rightJustified(4, QLatin1Char('0')))
                           .arg(regs.at(i));
    }
    const QString regsText = regTextList.join(QStringLiteral(", "));
    m_lastRegsLabel->setText(regsText.isEmpty() ? QStringLiteral("-") : regsText);
    m_lastExceptionLabel->setText(QStringLiteral("-"));
    appendLog(QStringLiteral("解析寄存器: %1").arg(m_lastRegsLabel->text()));

    if (regs.size() == 2) {
        const float value = registersToBigEndianFloat(regs.at(0), regs.at(1));
        m_lastFloatLabel->setText(QString::number(value, 'g', 10));
        appendLog(QStringLiteral("解析Big-Endian Float: %1").arg(m_lastFloatLabel->text()));
    } else {
        m_lastFloatLabel->setText(QStringLiteral("-"));
    }
}

float MainWindow::registersToBigEndianFloat(quint16 regHi, quint16 regLo)
{
    quint32 raw = (static_cast<quint32>(regHi) << 16) | static_cast<quint32>(regLo);
    float value = 0.0f;
    static_assert(sizeof(float) == sizeof(quint32), "float size must be 4 bytes");
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}

QString MainWindow::bytesToHexString(const QByteArray &data)
{
    QStringList parts;
    parts.reserve(data.size());

    for (const char b : data) {
        parts << QStringLiteral("%1")
                     .arg(static_cast<quint8>(b), 2, 16, QLatin1Char('0'))
                     .toUpper();
    }

    return parts.join(QLatin1Char(' '));
}
