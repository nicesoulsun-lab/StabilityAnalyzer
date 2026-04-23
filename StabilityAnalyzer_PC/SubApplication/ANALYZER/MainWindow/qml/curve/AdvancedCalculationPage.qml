import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."
import "../component"

Rectangle {
    id: advancedPanel

    property var detailPage
    // 流体力学部分支持“五选一反算”，这里维护目标量的显示名称和单位。
    property var fluidParameters: [
        { key: "diameter", title: qsTr("平均颗粒粒径"), unit: "μm" },
        { key: "concentration", title: qsTr("体积浓度"), unit: "%" },
        { key: "dispersedDensity", title: qsTr("分散相密度"), unit: "g/cm³" },
        { key: "continuousViscosity", title: qsTr("连续相粘度"), unit: "cP" },
        { key: "continuousDensity", title: qsTr("连续相密度"), unit: "g/cm³" }
    ]
    property var fluidValues: ({
        diameter: "",
        concentration: "",
        dispersedDensity: "",
        continuousViscosity: "",
        continuousDensity: ""
    })
    property var opticalValues: ({
        diameter: "",
        concentration: "",
        dispersedRefractive: "",
        dispersedAbsorption: "",
        continuousRefractive: "",
        continuousExtinction: ""
    })
    property var fluidInputRows: []
    property var opticalInputRows: []

    color: "#FFFFFF"

    function toNumber(value) {
        var parsed = Number(value)
        return isNaN(parsed) ? 0 : parsed
    }

    function fluidTargetKey() {
        return fluidParameters[fluidModeCombo.currentIndex].key
    }

    function fluidTargetTitle() {
        return fluidParameters[fluidModeCombo.currentIndex].title
    }

    function fluidTargetUnit() {
        return fluidParameters[fluidModeCombo.currentIndex].unit
    }

    function fluidInputParameters() {
        var targetKey = fluidTargetKey()
        return fluidParameters.filter(function(parameter) { return parameter.key !== targetKey })
    }

    function opticalTargetKey() {
        return opticalModeCombo.currentIndex === 0 ? "diameter" : "concentration"
    }

    function opticalTargetTitle() {
        return opticalTargetKey() === "diameter" ? qsTr("粒径") : qsTr("体积浓度")
    }

    function opticalTargetUnit() {
        return opticalTargetKey() === "diameter" ? "μm" : "%"
    }

    function opticalInputParameters() {
        var rows = [
            { key: "dispersedRefractive", title: qsTr("分散相折射率"), unit: "" },
            { key: "dispersedAbsorption", title: qsTr("分散相吸收率"), unit: "" },
            { key: "continuousRefractive", title: qsTr("连续相折射率"), unit: "" },
            { key: "continuousExtinction", title: qsTr("连续相消光范围"), unit: "" }
        ]
        rows.push(opticalTargetKey() === "diameter"
                  ? { key: "concentration", title: qsTr("体积浓度"), unit: "%" }
                  : { key: "diameter", title: qsTr("粒径"), unit: "μm" })
        return rows
    }

    function assignFieldValue(field, value) {
        if (field)
            field.text = value === undefined || value === null ? "" : String(value)
    }

    // 高级计算现在统一走 C++ 后端，QML 只负责把结果写回当前目标输入框。
    function applyBackendResult(result, targetEdit) {
        if (!targetEdit || !result)
            return

        if (result.success)
            assignFieldValue(targetEdit, result.displayText)
        else
            assignFieldValue(targetEdit, "")
    }

    function captureFluidInputs() {
        // 切换“计算目标”前先把当前界面上的值回写到缓存，避免用户已填数据丢失。
        var parameters = fluidInputParameters()
        for (var i = 0; i < Math.min(parameters.length, fluidInputRows.length); ++i) {
            var inputRow = fluidInputRows[i]
            if (inputRow && inputRow.field)
                fluidValues[parameters[i].key] = inputRow.field.text
        }
        if (fluidTargetEdit)
            fluidValues[fluidTargetKey()] = fluidTargetEdit.text
    }

    function refreshFluidInputs() {
        // 下拉框选谁，就把其余四项排到按钮上方，按钮下方固定显示当前目标。
        var parameters = fluidInputParameters()
        for (var i = 0; i < fluidInputRows.length; ++i) {
            var inputRow = fluidInputRows[i]
            if (!inputRow || !inputRow.field)
                continue
            if (i < parameters.length) {
                inputRow.labelText = parameters[i].title + qsTr("：")
                inputRow.unitText = parameters[i].unit
                assignFieldValue(inputRow.field, fluidValues[parameters[i].key])
                inputRow.visible = true
            } else {
                inputRow.visible = false
            }
        }

        if (fluidTargetRow) {
            fluidTargetRow.labelText = qsTr("计算目标：")
            fluidTargetRow.unitText = fluidTargetUnit()
        }
        if (fluidTargetEdit)
            assignFieldValue(fluidTargetEdit, fluidValues[fluidTargetKey()])
        if (fluidTargetNameText)
            fluidTargetNameText.text = fluidTargetTitle()
    }

    function captureOpticalInputs() {
        var parameters = opticalInputParameters()
        for (var i = 0; i < Math.min(parameters.length, opticalInputRows.length); ++i) {
            var inputRow = opticalInputRows[i]
            if (inputRow && inputRow.field)
                opticalValues[parameters[i].key] = inputRow.field.text
        }
        if (opticalTargetEdit)
            opticalValues[opticalTargetKey()] = opticalTargetEdit.text
    }

    function refreshOpticalInputs() {
        // 光学计算只做“粒径/体积浓度”二选一，其余光学参数始终作为输入项保留。
        var parameters = opticalInputParameters()
        for (var i = 0; i < opticalInputRows.length; ++i) {
            var inputRow = opticalInputRows[i]
            if (!inputRow || !inputRow.field)
                continue
            if (i < parameters.length) {
                inputRow.labelText = parameters[i].title + qsTr("：")
                inputRow.unitText = parameters[i].unit
                assignFieldValue(inputRow.field, opticalValues[parameters[i].key])
                inputRow.visible = true
            } else {
                inputRow.visible = false
            }
        }

        if (opticalTargetRow) {
            opticalTargetRow.labelText = qsTr("计算目标：")
            opticalTargetRow.unitText = opticalTargetUnit()
        }
        if (opticalTargetEdit)
            assignFieldValue(opticalTargetEdit, opticalValues[opticalTargetKey()])
        if (opticalTargetNameText)
            opticalTargetNameText.text = opticalTargetTitle()
    }

    function calculateHydrodynamicDiameter() {
        if (!migrationRateEdit || !fluidTargetEdit || !data_ctrl)
            return

        captureFluidInputs()
        var result = data_ctrl.calculateHydrodynamic({
            targetKey: fluidTargetKey(),
            migrationRate: migrationRateEdit.text,
            diameter: fluidValues.diameter,
            concentration: fluidValues.concentration,
            dispersedDensity: fluidValues.dispersedDensity,
            continuousViscosity: fluidValues.continuousViscosity,
            continuousDensity: fluidValues.continuousDensity
        })
        if (result && result.success)
            fluidValues[fluidTargetKey()] = result.displayText
        applyBackendResult(result, fluidTargetEdit)
    }

    function calculateOpticalDiameter() {
        if (!opticalTargetEdit || !data_ctrl)
            return

        captureOpticalInputs()
        var result = data_ctrl.calculateOptical({
            targetKey: opticalTargetKey(),
            diameter: opticalValues.diameter,
            concentration: opticalValues.concentration,
            dispersedRefractive: opticalValues.dispersedRefractive,
            dispersedAbsorption: opticalValues.dispersedAbsorption,
            continuousRefractive: opticalValues.continuousRefractive,
            continuousExtinction: opticalValues.continuousExtinction
        })
        if (result && result.success)
            opticalValues[opticalTargetKey()] = result.displayText
        applyBackendResult(result, opticalTargetEdit)
    }

    Component {
        id: metricInputComponent

        RowLayout {
            property string labelText: ""
            property alias field: field
            property string unitText: ""

            Layout.fillWidth: true
            spacing: 10

            Text {
                Layout.preferredWidth: 84
                text: parent.labelText
                font.pixelSize: 13
                font.family: "Microsoft YaHei"
                color: "#2F3A4A"
                wrapMode: Text.Wrap
            }

            LineEdit {
                id: field
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                input_rules: DoubleValidator { bottom: 0; decimals: 6 }
                border_color: "#B9D2F4"
                m_radius: 3
            }

            Text {
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignLeft
                text: parent.unitText
                font.pixelSize: 13
                font.family: "Microsoft YaHei"
                color: "#2F3A4A"
            }
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 18
        contentWidth: width
        contentHeight: container.implicitHeight
        clip: true

        ColumnLayout {
            id: container
            width: parent.width
            spacing: 14

            Text {
                Layout.fillWidth: true
                text: qsTr("高级计算")
                font.pixelSize: 16
                font.family: "Microsoft YaHei"
                color: "#2F3A4A"
                visible: false
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 32
                layoutDirection: Qt.LeftToRight

                SectionBox {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 320
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: migrationColumn.implicitHeight + 50
                    border.color: "#AED0F6"
                    border.width: 1
                    title: qsTr("颗粒迁移速率")
                    background_color: "#3B87E4"
                    font_color: "#FFFFFF"

                    ColumnLayout {
                        id: migrationColumn
                        anchors.fill: parent
                        anchors.topMargin: 42
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 12

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                item.labelText = qsTr("迁移速率：")
                                item.unitText = "mm/h"
                                advancedPanel.migrationRateEdit = item.field
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            text: qsTr("手动输入")
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#6F8096"
                        }

                    }
                }

                SectionBox {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 320
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: fluidColumn.implicitHeight + 50
                    border.color: "#AED0F6"
                    border.width: 1
                    title: qsTr("流体力学计算")
                    background_color: "#3B87E4"
                    font_color: "#FFFFFF"

                    ColumnLayout {
                        id: fluidColumn
                        anchors.fill: parent
                        anchors.topMargin: 42
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Text {
                                Layout.preferredWidth: 84
                                text: qsTr("计算参数选择：")
                                font.pixelSize: 13
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                wrapMode: Text.Wrap
                            }

                            UiComboBox {
                                id: fluidModeCombo
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                pixelSize: 12
                                model: [qsTr("平均颗粒粒径"),qsTr("体积浓度"),qsTr("分散相密度"),qsTr("连续相粘度"),qsTr("连续相密度")]
                                currentIndex: 0
                                hover_color: "#FFFFFF"
                                border_color: "#B9D2F4"
                                onCurrentIndexChanged: {
                                    advancedPanel.captureFluidInputs()
                                    advancedPanel.refreshFluidInputs()
                                }
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.fluidInputRows[0] = item
                                advancedPanel.refreshFluidInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.fluidInputRows[1] = item
                                advancedPanel.refreshFluidInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.fluidInputRows[2] = item
                                advancedPanel.refreshFluidInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.fluidInputRows[3] = item
                                advancedPanel.refreshFluidInputs()
                            }
                        }

                        IconButton {
                            Layout.fillWidth: true
                            Layout.leftMargin: 6
                            Layout.rightMargin: 6
                            Layout.preferredHeight: 30
                            button_text: qsTr("计算")
                            button_color: "#3F6DF0"
                            text_color: "#FFFFFF"
                            pixelSize: 13
                            onClicked: advancedPanel.calculateHydrodynamicDiameter()
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                item.labelText = qsTr("计算目标：")
                                item.field.input_en = false
                                advancedPanel.fluidTargetRow = item
                                advancedPanel.fluidTargetEdit = item.field
                                advancedPanel.refreshFluidInputs()
                            }
                        }

                        Text {
                            id: fluidTargetNameText
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#6F8096"
                        }
                    }
                }

                SectionBox {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 320
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: opticalColumn.implicitHeight + 50
                    border.color: "#AED0F6"
                    border.width: 1
                    title: qsTr("光学计算")
                    background_color: "#3B87E4"
                    font_color: "#FFFFFF"

                    ColumnLayout {
                        id: opticalColumn
                        anchors.fill: parent
                        anchors.topMargin: 42
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Text {
                                Layout.preferredWidth: 84
                                text: qsTr("计算参数选择：")
                                font.pixelSize: 13
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                wrapMode: Text.Wrap
                            }

                            UiComboBox {
                                id: opticalModeCombo
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                pixelSize: 12
                                model: [qsTr("粒径"),qsTr("体积浓度")]
                                currentIndex: 0
                                hover_color: "#FFFFFF"
                                border_color: "#B9D2F4"
                                onCurrentIndexChanged: {
                                    advancedPanel.captureOpticalInputs()
                                    advancedPanel.refreshOpticalInputs()
                                }
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.opticalInputRows[0] = item
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.opticalInputRows[1] = item
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.opticalInputRows[2] = item
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.opticalInputRows[3] = item
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                advancedPanel.opticalInputRows[4] = item
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        IconButton {
                            Layout.fillWidth: true
                            Layout.leftMargin: 6
                            Layout.rightMargin: 6
                            Layout.preferredHeight: 30
                            button_text: qsTr("计算")
                            button_color: "#3F6DF0"
                            text_color: "#FFFFFF"
                            pixelSize: 13
                            onClicked: advancedPanel.calculateOpticalDiameter()
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: metricInputComponent
                            onLoaded: {
                                item.labelText = qsTr("计算目标：")
                                item.field.input_en = false
                                advancedPanel.opticalTargetRow = item
                                advancedPanel.opticalTargetEdit = item.field
                                advancedPanel.refreshOpticalInputs()
                            }
                        }

                        Text {
                            id: opticalTargetNameText
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#6F8096"
                        }
                    }
                }
            }
        }
    }

    property var migrationRateEdit
    property var fluidTargetRow
    property var fluidTargetEdit
    property var opticalTargetRow
    property var opticalTargetEdit
}
