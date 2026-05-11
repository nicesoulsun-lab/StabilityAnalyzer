import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.VirtualKeyboard 2.1

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

        // 鏍囬鏍?
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

        // 鐘舵€佷俊鎭?
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

        // 璁惧鍒楄〃
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

        // 鏃ュ織杈撳嚭
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

    // 杩炴帴浠诲姟璋冨害鍣ㄧ殑淇″彿
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

