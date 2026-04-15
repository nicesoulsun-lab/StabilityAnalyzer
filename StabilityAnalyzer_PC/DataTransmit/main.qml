import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.VirtualKeyboard 2.4
import QtQuick.Controls 2.12
import QtCharts 2.3

Window {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("Data Transmit System")

    // 数据控制器连接
    Connections {
        target: dataController
        
        onDataReceived: {
            // 接收到数据，更新图表显示
            chartView.updateChart(data, channelId, channelName);
            statusText.text = "数据接收中 - 通道: " + channelName + " - 数据点: " + data.length;
        }
        
        onModbusConnectionStatusChanged: {
            modbusStatusText.text = connected ? "Modbus: 已连接" : "Modbus: 未连接";
            modbusStatusText.color = connected ? "green" : "red";
        }
        
        onAlgorithmDataStatusChanged: {
            algorithmStatusText.text = active ? "算法数据: 活跃" : "算法数据: 停止";
            algorithmStatusText.color = active ? "green" : "red";
        }
    }

    // 主界面布局
    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // 状态显示区域
        Row {
            spacing: 20
            
            Text {
                id: modbusStatusText
                text: "Modbus: 未连接"
                color: "red"
                font.pixelSize: 14
            }
            
            Text {
                id: algorithmStatusText
                text: "算法数据: 停止"
                color: "red"
                font.pixelSize: 14
            }
            
            Text {
                id: statusText
                text: "等待数据..."
                font.pixelSize: 14
            }
        }

        // 控制按钮区域
        Row {
            spacing: 10
            
            Button {
                text: "启动数据接收"
                onClicked: {
                    dataController.startDataReceiving();
                }
            }
            
            Button {
                text: "停止数据接收"
                onClicked: {
                    dataController.stopDataReceiving();
                }
            }
            
            Button {
                text: "配置Modbus"
                onClicked: {
                    // 打开Modbus配置对话框
                    configDialog.open();
                }
            }
        }

        // 图表显示区域
        ChartView {
            id: chartView
            width: parent.width
            height: parent.height - 100
            
            // 初始化图表
            Component.onCompleted: {
                initializeChart();
            }
            
            function initializeChart() {
                // 创建折线系列
                var series = chartView.createSeries(ChartView.SeriesTypeLine, "数据曲线");
                series.color = "blue";
                series.width = 2;
            }
            
            function updateChart(data, channelId, channelName) {
                // 更新图表数据
                var series = chartView.series(0);
                if (series) {
                    series.clear();
                    
                    for (var i = 0; i < data.length; i++) {
                        series.append(i, data[i]);
                    }
                    
                    // 更新图表标题
                    chartView.title = channelName + " - 通道ID: " + channelId;
                }
            }
        }
    }

    // Modbus配置对话框
    Dialog {
        id: configDialog
        title: "Modbus配置"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        Column {
            spacing: 10
            
            TextField {
                id: portField
                placeholderText: "串口名称 (如: COM1)"
                text: "COM1"
            }
            
            TextField {
                id: baudRateField
                placeholderText: "波特率"
                text: "9600"
                validator: IntValidator { bottom: 0; top: 115200 }
            }
            
            ComboBox {
                id: dataBitsCombo
                model: ["5", "6", "7", "8"]
                currentIndex: 3
                
                Label {
                    text: "数据位"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            ComboBox {
                id: stopBitsCombo
                model: ["1", "1.5", "2"]
                currentIndex: 0
                
                Label {
                    text: "停止位"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            ComboBox {
                id: parityCombo
                model: ["无", "奇校验", "偶校验"]
                currentIndex: 0
                
                Label {
                    text: "校验位"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        
        onAccepted: {
            // 应用配置
            var parityMap = [0, 1, 2]; // 无、奇、偶
            dataController.setModbusConfig(
                portField.text,
                parseInt(baudRateField.text),
                parseInt(dataBitsCombo.currentText),
                parseInt(stopBitsCombo.currentText),
                parityMap[parityCombo.currentIndex]
            );
        }
    }

    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: window.height
        width: window.width

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: window.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "y"
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
