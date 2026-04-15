#include "ModbusRTUWidget.h"
#include <QApplication>
#include "config_manager.h"
#include "logmanager.h"
quint16 calculateCRC16(const QByteArray& data, int algorithm = 0)
{
    quint16 crc = 0xFFFF;

    if (algorithm == 0) {
        // 算法1：标准Modbus CRC
        for (int i = 0; i < data.size(); ++i) {
            crc ^= static_cast<quint8>(data.at(i));
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
    } else if (algorithm == 1) {
        // 算法2：另一种常见实现
        for (int i = 0; i < data.size(); ++i) {
            crc ^= static_cast<quint8>(data.at(i));
            for (int j = 0; j < 8; ++j) {
                bool lsb = crc & 0x0001;
                crc >>= 1;
                if (lsb) {
                    crc ^= 0xA001;
                }
            }
        }
    } else if (algorithm == 2) {
        // 算法3：查表法
        static const quint16 table[] = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
            // ... 完整的CRC表（256个值）
        };

        quint8 high = 0xFF;
        quint8 low = 0xFF;

        for (int i = 0; i < data.size(); ++i) {
            quint8 index = low ^ static_cast<quint8>(data.at(i));
            low = high ^ table[index];
            high = table[index] >> 8;
        }

        crc = (static_cast<quint16>(high) << 8) | low;
    }

    return crc;
}
#include <QCoreApplication>
#include <QDebug>
#include <QByteArray>

// 测试不同的CRC算法
void testAllCRCAlgorithms()
{
    QByteArray testData = QByteArray::fromHex("0103021234");

    qDebug() << "===== 测试数据: 01 03 02 12 34 =====";
    qDebug() << "期望CRC: 0x85B6 (低字节在前: B6 85)";
    qDebug() << "";

    // 方法1: 标准Modbus CRC (多项式 0xA001)
    {
        quint16 crc = 0xFFFF;
        for (int i = 0; i < testData.size(); ++i) {
            crc ^= static_cast<quint8>(testData[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        qDebug() << "方法1 (标准Modbus, 0xA001):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }

    // 方法2: 多项式 0x8005（常用变体）
    {
        quint16 crc = 0xFFFF;
        for (int i = 0; i < testData.size(); ++i) {
            crc ^= static_cast<quint8>(testData[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0x8005;
                } else {
                    crc >>= 1;
                }
            }
        }
        qDebug() << "方法2 (多项式 0x8005):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }

    // 方法3: 初始值不同
    {
        quint16 crc = 0x0000;  // 不同的初始值
        for (int i = 0; i < testData.size(); ++i) {
            crc ^= static_cast<quint8>(testData[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        qDebug() << "方法3 (初始值 0x0000):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }

    // 方法4: 输出异或值
    {
        quint16 crc = 0xFFFF;
        for (int i = 0; i < testData.size(); ++i) {
            crc ^= static_cast<quint8>(testData[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        crc ^= 0x0000;  // 输出异或
        qDebug() << "方法4 (输出异或 0x0000):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }

    // 方法5: 反转输入输出
    {
        // 先反转每个字节
        QByteArray reversed;
        for (int i = 0; i < testData.size(); ++i) {
            quint8 byte = static_cast<quint8>(testData[i]);
            // 反转字节
            byte = ((byte & 0x01) << 7) | ((byte & 0x02) << 5) | ((byte & 0x04) << 3) | ((byte & 0x08) << 1) |
                   ((byte & 0x10) >> 1) | ((byte & 0x20) >> 3) | ((byte & 0x40) >> 5) | ((byte & 0x80) >> 7);
            reversed.append(static_cast<char>(byte));
        }

        quint16 crc = 0xFFFF;
        for (int i = 0; i < reversed.size(); ++i) {
            crc ^= static_cast<quint8>(reversed[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        qDebug() << "方法5 (反转输入):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }

    // 方法6: 使用查表法，但使用正确的表
    {
        static const quint16 crcTable[] = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
            0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
            0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
            0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
            0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
            0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
            0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
            0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
            0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
            0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
            0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
            0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
            0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
            0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
            0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
            0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
            0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
            0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
            0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
            0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
            0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
            0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
            0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
            0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
            0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
            0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
            0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
            0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
            0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
            0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
            0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
            0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
        };

        quint8 high = 0xFF;
        quint8 low = 0xFF;

        for (int i = 0; i < testData.size(); ++i) {
            quint8 index = low ^ static_cast<quint8>(testData[i]);
            low = high ^ crcTable[index];
            high = crcTable[index] >> 8;
        }

        quint16 crc = (static_cast<quint16>(high) << 8) | low;
        qDebug() << "方法6 (标准查表法):" << QString::number(crc, 16).rightJustified(4, '0').toUpper();
    }
}
int main(int argc, char *argv[])
{
    //初始化日志模块
    LogManager log;
    log.InitLog("./log",QObject::tr("QMoudbusRTUUnit"),1);
    LOG_INFO(QString("软件启动:[%1],argc[%2]").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
             .arg(argc));
    
    QApplication a(argc, argv);
    //    ModbusRTUWidget w;
    //    w.show();

    //测试代码
//    testAllCRCAlgorithms();
//    // 测试数据
//       QByteArray test1 = QByteArray::fromHex("010300000001");
//       QByteArray test2 = QByteArray::fromHex("0103021234");

//       qDebug() << "测试CRC算法:";

//       for (int alg = 0; alg < 3; ++alg) {
//           quint16 crc1 = calculateCRC16(test1, alg);
//           quint16 crc2 = calculateCRC16(test2, alg);

//           qDebug() << QString("算法%1:").arg(alg);
//           qDebug() << QString("  01 03 00 00 00 01 -> %1").arg(QString::number(crc1, 16).rightJustified(4, '0').toUpper());
//           qDebug() << QString("  01 03 02 12 34 -> %1").arg(QString::number(crc2, 16).rightJustified(4, '0').toUpper());

//    }

    // 创建配置管理器
    ConfigManager configManager;

    // 加载配置文件
    QString path = QCoreApplication::applicationDirPath() + "/config.json";
    LOG_INFO()<<"lllllll"<<path;
    if (configManager.loadFromFile(path)) {
    } else {
        // 添加默认设备
        ConfigManager::DeviceConfig device;
        device.slaveId = 1;
        device.name = "PLC-1";
        device.description = "主控制器";
        device.serialConfig.portName = "COM1";
        device.serialConfig.baudRate = 9600;

        // 添加一些寄存器定义
        RegisterInfo reg1;
        reg1.address = 0;
        reg1.name = "温度";
        reg1.unit = "°C";
        reg1.scaleFactor = 0.1;
        device.registers[0] = reg1;

        RegisterInfo reg2;
        reg2.address = 1;
        reg2.name = "压力";
        reg2.unit = "MPa";
        reg2.scaleFactor = 0.01;
        device.registers[1] = reg2;

        configManager.addDevice(device);

        // 保存默认配置
        configManager.saveToFile("config.json");
    }

    // 创建Modbus客户端
    ModbusClient client;

    // 连接异步操作完成信号
    QObject::connect(&client, &ModbusClient::requestCompleted, 
        [](const QString& tag, const ModbusResult& result) {
            if (result.success) {
                LOG_INFO() << "异步操作完成，标签:" << tag << "成功";
                if (!result.values.isEmpty()) {
                    LOG_INFO() << "返回值:" << result.values.first();
                }
            } else {
                LOG_WARNING() << "异步操作失败，标签:" << tag << "错误:" << result.errorString;
            }
        }
    );

    // 配置客户端
    ModbusClient::ClientConfig clientConfig;
    
    // 确保设备配置存在
    auto deviceConfig = configManager.getDevice(1);
    if (deviceConfig.slaveId == 0) {
        LOG_INFO() << "设备配置不存在，使用默认配置";
        deviceConfig = ConfigManager::defaultDeviceConfig();
        configManager.addDevice(deviceConfig);
    }
    
    clientConfig.serialConfig = deviceConfig.serialConfig;
    clientConfig.maxQueueSize = 500;
//    clientConfig.enableLogging = true;

    if (!client.initialize(clientConfig)) {
        return -1;
    }

    // 连接设备
    if (client.connect()) {
        LOG_INFO() << "Modbus客户端连接成功";

        // 示例1：同步读取保持寄存器
        try {
            LOG_INFO() << "开始同步读取测试...";
            
            // 读取单个寄存器（通过读取1个寄存器来实现）
            auto result = client.readHoldingRegisters(1, 0, 1);
            if (!result.isEmpty()) {
                LOG_INFO() << "读取保持寄存器成功，值:" << result.first();
            } else {
                LOG_WARNING() << "读取保持寄存器失败";
            }
            
            // 读取多个寄存器
            auto multiResult = client.readHoldingRegisters(1, 0, 2);
            if (!multiResult.isEmpty()) {
                LOG_INFO() << "读取多个保持寄存器成功";
                for (int i = 0; i < multiResult.size(); ++i) {
                    LOG_INFO() << "寄存器" << i << "值:" << multiResult[i];
                }
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR() << "同步读取异常:" << e.what();
        }

        // 示例2：异步读取测试
        LOG_INFO() << "开始异步读取测试...";
        
        // 异步读取线圈
        client.readCoilsAsync(1, 0, 8, "test_coils");

        // 异步读取输入寄存器
        client.readInputRegistersAsync(1, 0, 4, "test_input_regs");

        // 示例3：写入操作测试
        LOG_INFO() << "开始写入操作测试...";
        
        // 示例4：批量操作测试
        LOG_INFO() << "开始批量操作测试...";
        
        // 批量读取不同地址的寄存器
        QList<QPair<int, int>> readRequests = {
            {0, 1},  // 地址0，长度1
            {2, 2},  // 地址2，长度2
            {5, 1}   // 地址5，长度1
        };
        
        for (const auto& request : readRequests) {
            client.readHoldingRegistersAsync(1, request.first, request.second, 
                QString("batch_read_%1").arg(request.first));
        }

        // 示例5：定时读取测试
        LOG_INFO() << "开始定时读取测试...";
        
        // 创建定时器，每5秒读取一次数据
        QTimer* timer = new QTimer();
        QObject::connect(timer, &QTimer::timeout, [&client]() {
            static int count = 0;
            client.readHoldingRegistersAsync(1, 0, 1, 
                QString("timer_read_%1").arg(count++));
        });
        timer->start(5000); // 5秒间隔

        LOG_INFO() << "所有测试任务已启动，等待执行...";
        
        // 运行应用
        return a.exec();

    } else {
        LOG_ERROR() << "Modbus客户端连接失败";
        return -1;
    }
}


