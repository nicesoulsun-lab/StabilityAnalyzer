import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.VirtualKeyboard 2.4

Window {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("Modbus Task Scheduler")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: "Modbus Task Scheduler"
                font.bold: true
                font.pixelSize: 18
                Layout.fillWidth: true
            }
            
            Button {
                id: startButton
                text: taskScheduler.isRunning ? "Stop Scheduler" : "Start Scheduler"
                onClicked: {
                    if (taskScheduler.isRunning) {
                        taskScheduler.stopScheduler()
                    } else {
                        taskScheduler.startScheduler()
                    }
                }
            }
        }

        // 状态信息
        GroupBox {
            title: "System Status"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                anchors.fill: parent
                
                Label { text: "Scheduler Status:" }
                Label { 
                    text: taskScheduler.isRunning ? "Running" : "Stopped"
                    color: taskScheduler.isRunning ? "green" : "red"
                }
                
                Label { text: "Total Devices:" }
                Label { text: taskScheduler.totalDevices }
                
                Label { text: "Connected Devices:" }
                Label { 
                    text: taskScheduler.connectedDevices
                    color: taskScheduler.connectedDevices > 0 ? "green" : "red"
                }
                
                Label { text: "Config File:" }
                Label { text: taskScheduler.configFile }
            }
        }

        // 设备列表
        GroupBox {
            title: "Device List"
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ScrollView {
                anchors.fill: parent
                
                ListView {
                    id: deviceList
                    model: taskScheduler.getDeviceList()
                    
                    delegate: ItemDelegate {
                        width: deviceList.width
                        height: 60
                        
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            
                            RowLayout {
                                Layout.fillWidth: true
                                
                                Label {
                                    text: modelData
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                                
                                Label {
                                    text: taskScheduler.getDeviceStatus(modelData)
                                    color: taskScheduler.getDeviceStatus(modelData) === "Connected" ? "green" : "red"
                                }
                            }
                            
                            Row {
                                spacing: 5
                                
                                Repeater {
                                    model: taskScheduler.getTaskList(modelData)
                                    
                                    Button {
                                        text: modelData
                                        onClicked: {
                                            taskScheduler.executeUserTask(modelData, modelData)
                                        }
                                        enabled: taskScheduler.isRunning && taskScheduler.getDeviceStatus(modelData) === "Connected"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 日志输出
        GroupBox {
            title: "Log Output"
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            
            ScrollView {
                anchors.fill: parent
                
                TextArea {
                    id: logArea
                    readOnly: true
                    wrapMode: Text.Wrap
                    text: "Task scheduler log will appear here...\n"
                }
            }
        }
    }

    // 连接任务调度器的信号
    Connections {
        target: taskScheduler
        
        onTaskCompleted: {
            logArea.append("Task completed - Device: " + deviceId + ", Task: " + taskId + 
                          ", Success: " + success + ", Result: " + result)
        }
        
        onErrorOccurred: {
            logArea.append("ERROR: " + error)
        }
        
        onSchedulerStarted: {
            logArea.append("Scheduler started successfully")
        }
        
        onSchedulerStopped: {
            logArea.append("Scheduler stopped")
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
