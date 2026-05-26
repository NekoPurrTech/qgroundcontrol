import QtQuick
import QtQuick.Shapes
import QtQuick.Layouts

import QGroundControl.Controllers 1.0

Item {
    id: root
    anchors.fill: parent

    EngineStatusController {
        id: engineCtrl
    }

    // QML-accessible command sender (forward to active Vehicle)
    EngineCommandSender {
        id: engSender
    }

    component ToggleSwitchBlue: Rectangle {
        id: toggleBlue
        width: 180
        height: 60
        radius: 6
        color: checked ? "#42A5F5" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleBlue.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleBlue.checked = !toggleBlue.checked
        }
    }

    component ToggleSwitchGreen: Rectangle {
        id: toggleGreen
        width: 180
        height: 60
        radius: 6
        color: checked ? "#4CAF50" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleGreen.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleGreen.checked = !toggleGreen.checked
        }
    }

    component ToggleSwitchRed: Rectangle {
        id: toggleRed
        width: 180
        height: 60
        radius: 6
        color: checked ? "#424242" : "#af504c"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleRed.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleRed.checked = !toggleRed.checked
        }
    }
    component ToggleSwitchAmber: Rectangle {
        id: toggleAmber
        width: 180
        height: 60
        radius: 6
        color: checked ? "#ffbf00" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleAmber.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleAmber.checked = !toggleAmber.checked
        }
    }

    component CommandButton: Rectangle {
        id: commandButton
        width: 180
        height: 38
        radius: 6
        color: selected ? "#4CAF50" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool selected: false
        signal clicked()

        Text {
            anchors.centerIn: parent
            text: commandButton.text
            color: "white"
            font.bold: true
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }

        MouseArea {
            anchors.fill: parent
            onClicked: commandButton.clicked()
        }
    }

    component FeedbackValue: Rectangle {
        id: feedbackValue
        width: 50
        height: 38
        radius: 4
        color: "#1a1a1a"
        border.color: "#666"
        border.width: 1

        property string valueText: "0"

        Text {
            anchors.centerIn: parent
            width: parent.width - 8
            text: feedbackValue.valueText
            color: "white"
            font.bold: true
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component FanStatusLight: Rectangle {
        id: fanStatusLight
        width: 20
        height: 20
        radius: 3
        color: active ? "#4CAF50" : "#424242"
        border.color: active ? "white" : "#666"
        border.width: 1

        property bool active: false
    }

    component LegendItem: Row {
        id: legendItem
        spacing: 8
        height: 18

        property color swatchColor: "white"
        property string label: ""

        Rectangle {
            width: 14
            height: 14
            radius: 3
            color: legendItem.swatchColor
            border.color: "white"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: legendItem.label
            color: "white"
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    component FaultItem: Rectangle {
        id: faultItem
        width: parent ? parent.width : 200
        height: faultText.implicitHeight + 12
        radius: 4
        color: "#7f1d1d"
        border.color: "#ff5252"
        border.width: 2

        property string faultTextValue: ""

        Text {
            id: faultText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            text: faultItem.faultTextValue
            color: "white"
            font.bold: true
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }


    component BarGauge: Rectangle {
        id: gauge
        width: 70
        height: 240
        color: "#2a2a2a"
        border.color: highlighted ? "white" : "#555"
        border.width: highlighted ? 2 : 1
        radius: 4

        property string label: ""
        property real value: 0
        property real maxValue: 100
        property string unit: ""
        property color barColor: "#2196F3"
        property bool highlighted: false
        property int statusIndex: 0
        property color statusColor: root.engineStatusColor(statusIndex, value)
        property var thresholdValues: root.visibleEngineStatusThresholds(statusIndex, maxValue)
        property real percentage: Math.min(Math.max(value / maxValue, 0), 1)

        Column {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            Text {
                width: parent.width
                text: gauge.value.toFixed(1)
                color: gauge.statusColor
                font.bold: true
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                height: 16
            }

            Text {
                width: parent.width
                text: gauge.unit
                color: "#aaa"
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                height: 12
            }

            Item {
                width: parent.width
                height: parent.height - 76
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    anchors.fill: parent
                    color: "#1a1a1a"
                    radius: 2
                }

                Rectangle {
                    width: parent.width - 10
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: parent.height * gauge.percentage
                    radius: 2
                    color: gauge.statusColor
                }

                Repeater {
                    model: 5
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#444"
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: parent.height / 4 * index
                        opacity: 0.5
                    }
                }

                Repeater {
                    model: gauge.thresholdValues

                    Item {
                        width: parent.width
                        height: 10
                        y: root.thresholdMarkerTop(Number(modelData), gauge.maxValue, parent.height, height)

                        Rectangle {
                            width: Math.max(10, parent.width - 28)
                            height: 1
                            x: index % 2 === 0 ? 4 : 24
                            y: parent.height / 2
                            color: "white"
                            opacity: 0.85
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: index % 2 === 0 ? parent.width - width - 2 : 2
                            width: 22
                            text: root.thresholdLabel(gauge.statusIndex, Number(modelData))
                            color: "white"
                            font.bold: true
                            font.pixelSize: 8
                            horizontalAlignment: index % 2 === 0 ? Text.AlignRight : Text.AlignLeft
                        }
                    }
                }
            }

            Text {
                width: parent.width
                height: 40
                text: gauge.label
                color: "white"
                font.bold: true
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                lineHeight: 1.2
            }
        }
    }

    // 左上角开关布局
    property int selectedMode: -1
    property bool hasThrottleRequest: false
    property real throttleRequestValue: 0
    property string throttleRequestText: ""
    property bool hasRpmRequest: false
    property real rpmRequestValue: 0
    property string rpmRequestText: ""
    property real escRawMaxValue: 8191.0
    property color statusGreen: "#4CAF50"
    property color statusYellow: "#ffbf00"
    property color statusRed: "#f44336"

    function escRawToActuatorValue(value) {
        return (Number(value) - 1.0) / (escRawMaxValue - 1.0)
    }

    function actuatorTest() {
        // Prefer globals.activeVehicle when available (matches Actuator UI). Fall back to 'vehicle' if present.
        var v = null
        if (typeof globals !== 'undefined' && globals.activeVehicle) {
            v = globals.activeVehicle
        } else if (typeof vehicle !== 'undefined') {
            v = vehicle
        }

        if (!v || !v.actuators || !v.actuators.actuatorTest) {
            return null
        }

        return v.actuators.actuatorTest
    }

    function sendEscChannel(channel, value) {
        var at = actuatorTest()
        if (!at) {
            return false
        }

        at.setActive(true)
        at.setChannelTo(channel, escRawToActuatorValue(value))
        return true
    }

    function stopEscChannel(channel) {
        var at = actuatorTest()
        if (!at) {
            return false
        }

        at.stopControl(channel)
        return true
    }

    function pumpFanBitmask() {
        var value = 0
        if (pump1.checked) value |= 1
        if (pump2.checked) value |= 2
        if (fan1.checked) value |= 4
        if (fan2.checked) value |= 8
        if (intercooler.checked) value |= 16
        return value
    }

    function fanStatusOn(bitIndex) {
        var bits = Number(engineCtrl.fanStatusBits)
        if (isNaN(bits)) {
            return false
        }
        return ((bits >> bitIndex) & 1) === 1
    }

    function numberText(value, decimals) {
        var numberValue = Number(value)
        if (isNaN(numberValue)) {
            numberValue = 0
        }
        return numberValue.toFixed(decimals)
    }

    function intText(value) {
        var numberValue = Number(value)
        if (isNaN(numberValue)) {
            numberValue = 0
        }
        return Math.round(numberValue).toString()
    }

    function engineStatusColor(index, value) {
        var v = Number(value)
        if (isNaN(v)) {
            return statusRed
        }

        switch (index) {
        case 1:     // 发动机转速, r/min
            if (v >= 1600 && v <= 5800) return statusGreen
            if (v >= 0 && v < 1600) return statusYellow
            if (v > 5800 && v <= 6000) return statusYellow
            return statusRed
        case 2:     // 滑油温度, ℃
            if (v >= 90 && v <= 110) return statusGreen
            if (v >= 50 && v < 90) return statusYellow
            if (v > 110 && v <= 130) return statusYellow
            return statusRed
        case 3:     // 滑油压力, bar
            if (v >= 1.8 && v <= 6.0) return statusGreen
            if (v >= 1.5 && v < 1.8) return statusYellow
            if (v > 6.0 && v <= 7.0) return statusYellow
            return statusRed
        case 4:     // 排气温度, ℃
            if (v >= 0 && v <= 900) return statusGreen
            if (v > 900 && v <= 950) return statusYellow
            return statusRed
        case 5:     // 冷却液温度, ℃
            if (v >= 85 && v <= 110) return statusGreen
            if (v >= 50 && v < 85) return statusYellow
            if (v > 110 && v <= 120) return statusYellow
            return statusRed
        case 6:     // 歧管温度, ℃
            if (v >= 10 && v <= 60) return statusGreen
            if (v >= -20 && v < 10) return statusYellow
            if (v > 60 && v <= 70) return statusYellow
            return statusRed
        case 7:     // 废气阀位置, %
            if (v >= 0 && v <= 100) return statusGreen
            return statusRed
        case 8:     // 节气门位置, %
            if (v >= 7.5 && v <= 100) return statusGreen
            if (v >= 6 && v < 7.5) return statusYellow
            if (v >= 0 && v < 6) return statusRed
            return statusRed
        case 9:     // 歧管压力, hPa
            if (v >= 340 && v <= 1460) return statusGreen
            if (v > 1460 && v <= 1470) return statusYellow
            return statusRed
        case 10:    // 燃油压力, bar
            if (v >= 2.0 && v <= 5.0) return statusGreen
            if (v >= 1.75 && v < 2.0) return statusYellow
            if (v > 5.0 && v <= 5.5) return statusYellow
            return statusRed
        case 11:    // 燃油压差, bar
            if (v >= 1.8 && v <= 3.8) return statusGreen
            if (v >= 1.6 && v < 1.8) return statusYellow
            if (v > 3.8 && v <= 4.0) return statusYellow
            return statusRed
        case 12:    // ECU电源电压, V
            if (v >= 12.5 && v <= 14.5) return statusGreen
            if (v >= 10 && v < 12.5) return statusYellow
            if (v > 14.5 && v <= 16) return statusYellow
            return statusRed
        default:
            return "#2196F3"
        }
    }

    function engineStatusThresholds(index) {
        switch (index) {
        case 1:
            return [1600, 5800, 6000]
        case 2:
            return [50, 90, 110, 130]
        case 3:
            return [1.5, 1.8, 6.0, 7.0]
        case 4:
            return [900, 950]
        case 5:
            return [50, 85, 110, 120]
        case 6:
            return [-20, 10, 60, 70]
        case 7:
            return [0, 100]
        case 8:
            return [6, 7.5]
        case 9:
            return [340, 1460, 1470]
        case 10:
            return [1.75, 2.0, 5.0, 5.5]
        case 11:
            return [1.6, 1.8, 3.8, 4.0]
        case 12:
            return [10, 12.5, 14.5, 16]
        default:
            return []
        }
    }

    function visibleEngineStatusThresholds(index, maxValue) {
        var max = Number(maxValue)
        var raw = engineStatusThresholds(index)
        var visible = []
        for (var i = 0; i < raw.length; i++) {
            var value = Number(raw[i])
            if (!isNaN(value) && value > 0 && value < max) {
                visible.push(value)
            }
        }
        return visible
    }

    function thresholdRatio(value, maxValue) {
        var v = Number(value)
        var max = Number(maxValue)
        if (isNaN(v) || isNaN(max) || max <= 0) {
            return 0
        }
        return Math.min(Math.max(v / max, 0), 1)
    }

    function thresholdMarkerTop(value, maxValue, barHeight, markerHeight) {
        var ratio = thresholdRatio(value, maxValue)
        var top = Number(barHeight) * (1 - ratio) - Number(markerHeight) / 2
        return Math.max(0, Math.min(Number(barHeight) - Number(markerHeight), top))
    }

    function thresholdLabel(index, value) {
        var v = Number(value)
        if (isNaN(v)) {
            return ""
        }
        if (Math.abs(v - Math.round(v)) < 0.001) {
            return Math.round(v).toString()
        }
        if (v < 10) {
            return v.toFixed(2).replace(/0+$/, "").replace(/\.$/, "")
        }
        return v.toFixed(1).replace(/0+$/, "").replace(/\.$/, "")
    }

    function sendPumpFanBitmask() {
        return sendEscChannel(3, pumpFanBitmask())
    }

    function selectMode(modeValue) {
        selectedMode = modeValue
        return sendEscChannel(0, modeValue)
    }

    function confirmThrottleRequest() {
        var value = Number(throttleRequestText)
        if (isNaN(value)) {
            return false
        }

        throttleRequestValue = value
        hasThrottleRequest = true
        return sendEscChannel(1, value)
    }

    function confirmRpmRequest() {
        var value = Number(rpmRequestText)
        if (isNaN(value)) {
            return false
        }

        rpmRequestValue = value
        hasRpmRequest = true
        return sendEscChannel(2, value)
    }

    Rectangle {
        id: colorLegend
        width: 190
        height: legendColumn.implicitHeight + 18
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        radius: 4
        color: "#2a2a2a"
        border.color: "#777"
        border.width: 1

        Column {
            id: legendColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 9
            spacing: 6

            Text {
                width: parent.width
                text: "颜色图例"
                color: "white"
                font.bold: true
                font.pixelSize: 13
            }

            LegendItem { swatchColor: root.statusGreen; label: "正常" }
            LegendItem { swatchColor: root.statusYellow; label: "警告" }
            LegendItem { swatchColor: root.statusRed; label: "报警" }
            LegendItem { swatchColor: "white"; label: "白框重点参数" }
        }
    }

    Rectangle {
        id: faultPanel
        width: 230
        height: 430
        anchors.top: colorLegend.bottom
        anchors.topMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 20
        radius: 4
        color: "#241f1f"
        border.color: engineCtrl.engineFaults.length > 0 ? "#ff5252" : "#777"
        border.width: engineCtrl.engineFaults.length > 0 ? 2 : 1

        Text {
            id: faultHeader
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            height: 18
            text: "故障状态"
            color: engineCtrl.engineFaults.length > 0 ? "#ff5252" : "#bdbdbd"
            font.bold: true
            font.pixelSize: 14
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: faultHeader.bottom
            anchors.margins: 10
            text: "当前无故障"
            color: "#bdbdbd"
            font.bold: true
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            visible: engineCtrl.engineFaults.length === 0
        }

        Flickable {
            id: faultList
            anchors.top: faultHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 10
            clip: true
            contentHeight: faultColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            visible: engineCtrl.engineFaults.length > 0

            Column {
                id: faultColumn
                width: faultList.width
                spacing: 6

                Repeater {
                    model: engineCtrl.engineFaults

                    FaultItem {
                        width: faultColumn.width
                        faultTextValue: modelData
                    }
                }
            }
        }
    }

    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 0
        spacing: 12

        Rectangle {
            width: 180
            height: 36
            radius: 4
            color: "#2a2a2a"
            border.color: "#666"
            border.width: 1

            Row {
                anchors.centerIn: parent
                spacing: 8

                Rectangle {
                    width: 18
                    height: 18
                    radius: 3
                    color: engineCtrl.engineDataConnected === true ? "#4CAF50" : "#f44336"
                    border.color: "white"
                    border.width: 1
                }

                Text {
                    text: "Engine Link"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Row {
            spacing: 8
            ToggleSwitchAmber {
                id: pump1
                text: "Fuel Pump 1"
                onCheckedChanged: root.sendPumpFanBitmask()
            }
            Item {
                width: 20
                height: 60
                FanStatusLight {
                    anchors.centerIn: parent
                    active: root.fanStatusOn(0)
                }
            }
        }

        Row {
            spacing: 8
            ToggleSwitchAmber {
                id: pump2
                text: "Fuel Pump 2"
                onCheckedChanged: root.sendPumpFanBitmask()
            }
            Item {
                width: 20
                height: 60
                FanStatusLight {
                    anchors.centerIn: parent
                    active: root.fanStatusOn(1)
                }
            }
        }

        Row {
            spacing: 8
            ToggleSwitchBlue {
                id: fan1
                text: "Water Cooler Fan 1"
                onCheckedChanged: root.sendPumpFanBitmask()
            }
            Item {
                width: 20
                height: 60
                FanStatusLight {
                    anchors.centerIn: parent
                    active: root.fanStatusOn(2)
                }
            }
        }

        Row {
            spacing: 8
            ToggleSwitchBlue {
                id: fan2
                text: "Water Cooler Fan 2"
                onCheckedChanged: root.sendPumpFanBitmask()
            }
            Item {
                width: 20
                height: 60
                FanStatusLight {
                    anchors.centerIn: parent
                    active: root.fanStatusOn(3)
                }
            }
        }

        Row {
            spacing: 8
            ToggleSwitchBlue {
                id: intercooler
                text: "Inter Cooler Fan"
                onCheckedChanged: root.sendPumpFanBitmask()
            }
            Item {
                width: 20
                height: 60
                FanStatusLight {
                    anchors.centerIn: parent
                    active: root.fanStatusOn(4)
                }
            }
        }

        ToggleSwitchGreen {
            id: engStart
            text: "Engine Start"
            onCheckedChanged: checked ? root.sendEscChannel(5, 4000.0) : root.stopEscChannel(5)
        }

        ToggleSwitchRed {
            id: estop
            text: "Emergency STOP"
            onCheckedChanged: checked ? root.sendEscChannel(6, 4000.0) : root.stopEscChannel(6)
        }

        Column {
            width: 236
            spacing: 8

            Text {
                width: parent.width
                text: "Mode Select"
                color: "white"
                font.bold: true
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                width: parent.width
                spacing: 6

                CommandButton {
                    width: 56
                    height: 36
                    text: "Mechanical"
                    selected: root.selectedMode === 0
                    onClicked: root.selectMode(0)
                }

                CommandButton {
                    width: 56
                    height: 36
                    text: "Position"
                    selected: root.selectedMode === 1
                    onClicked: root.selectMode(1)
                }

                CommandButton {
                    width: 56
                    height: 36
                    text: "Speed"
                    selected: root.selectedMode === 2
                    onClicked: root.selectMode(2)
                }

                FeedbackValue {
                    width: 50
                    height: 36
                    valueText: root.intText(engineCtrl.engineMode)
                }
            }

            Text {
                width: parent.width
                text: "Throttle Request"
                color: "white"
                font.bold: true
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                width: parent.width
                spacing: 6

                Rectangle {
                    width: 114
                    height: 38
                    radius: 4
                    color: "#1a1a1a"
                    border.color: "#666"
                    border.width: 1

                    TextInput {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: root.throttleRequestText
                        color: "white"
                        font.pixelSize: 15
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        validator: DoubleValidator {}
                        onTextChanged: root.throttleRequestText = text
                        onAccepted: root.confirmThrottleRequest()
                    }
                }

                CommandButton {
                    width: 60
                    height: 38
                    text: "Send"
                    onClicked: root.confirmThrottleRequest()
                }

                FeedbackValue {
                    valueText: root.numberText(engineCtrl.throttleRequestFeedback, 1)
                }
            }

            Text {
                width: parent.width
                text: "RPM Request"
                color: "white"
                font.bold: true
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                width: parent.width
                spacing: 6

                Rectangle {
                    width: 114
                    height: 38
                    radius: 4
                    color: "#1a1a1a"
                    border.color: "#666"
                    border.width: 1

                    TextInput {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: root.rpmRequestText
                        color: "white"
                        font.pixelSize: 15
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        validator: DoubleValidator {}
                        onTextChanged: root.rpmRequestText = text
                        onAccepted: root.confirmRpmRequest()
                    }
                }

                CommandButton {
                    width: 60
                    height: 38
                    text: "Send"
                    onClicked: root.confirmRpmRequest()
                }

                FeedbackValue {
                    valueText: root.numberText(engineCtrl.rpmRequestFeedback, 0)
                }
            }
        }
    }

    // Central keepalive timer: periodically refresh active actuator channels so ActuatorTesting watchdog
    // doesn't stop them. When any toggle is checked this timer runs and repeatedly sends setChannelTo.
    property bool pumpFanControlsActive: pump1.checked || pump2.checked || fan1.checked || fan2.checked || intercooler.checked
    property bool anyActuatorActive: pumpFanControlsActive || engStart.checked || estop.checked || selectedMode >= 0 || hasThrottleRequest || hasRpmRequest

    Timer {
        id: actuatorKeepalive
        interval: 80
        repeat: true
        running: anyActuatorActive
        onTriggered: {
            var at = root.actuatorTest()

            // If any active, ensure actuator testing is active
            if (at && anyActuatorActive) {
                at.setActive(true)

                if (selectedMode >= 0) at.setChannelTo(0, escRawToActuatorValue(selectedMode))
                if (hasThrottleRequest) at.setChannelTo(1, escRawToActuatorValue(throttleRequestValue))
                if (hasRpmRequest) at.setChannelTo(2, escRawToActuatorValue(rpmRequestValue))
                if (pumpFanControlsActive) at.setChannelTo(3, escRawToActuatorValue(pumpFanBitmask()))

                if (engStart.checked) at.setChannelTo(5, escRawToActuatorValue(4000.0))
                else at.stopControl(5)

                if (estop.checked) at.setChannelTo(6, escRawToActuatorValue(4000.0))
                else at.stopControl(6)
            } else if (at) {
                // no active toggles, stop everything and deactivate
                at.stopControl(-1)
                at.setActive(false)
            }
        }
    }

    // 重点参数单独置顶，其余发动机数据分区显示。
    Column {
        id: mappedGrid
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        spacing: 24

        Row {
            id: highlightedGaugeRow
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            BarGauge {
                label: "发动机转速"
                unit: "r/min"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.rpm : 0
                maxValue: 6500
                statusIndex: 1
                highlighted: true
            }
            BarGauge {
                label: "滑油压力"
                unit: "bar"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.oilPressure ) : 0
                maxValue: 8
                statusIndex: 3
                highlighted: true
            }
            BarGauge {
                label: "燃油压力"
                unit: "bar"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.fuelPressure ) : 0
                maxValue: 6
                statusIndex: 10
                highlighted: true
            }
            BarGauge {
                label: "滑油温度"
                unit: "℃"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.temperature : 0
                maxValue: 150
                statusIndex: 2
                highlighted: true
            }
            BarGauge {
                label: "歧管温度"
                unit: "℃"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.intakeTemp : 0
                maxValue: 100
                statusIndex: 6
                highlighted: true
            }
        }

        Rectangle {
            width: Math.max(highlightedGaugeRow.width, otherGaugeRow1.width, otherGaugeRow2.width)
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            color: "white"
            opacity: 0.65
        }

        Row {
            id: otherGaugeRow1
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            BarGauge {
                label: "废气阀位置"
                unit: "%"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.throttleOpeningSendVal : 0
                maxValue: 100
                statusIndex: 7
            }
            BarGauge {
                label: "节气门位置"
                unit: "%"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.throttlePosA : 0
                maxValue: 100
                statusIndex: 8
            }
            BarGauge {
                label: "ECU电源电压"
                unit: "V"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.voltage : 0
                maxValue: 16
                statusIndex: 12
            }
            BarGauge { label: "排气温度1"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp1 : 0; maxValue: 1000; statusIndex: 4 }
            BarGauge { label: "排气温度2"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp2 : 0; maxValue: 1000; statusIndex: 4 }
            BarGauge { label: "排气温度3"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp3 : 0; maxValue: 1000; statusIndex: 4 }
            BarGauge { label: "排气温度4"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp4 : 0; maxValue: 1000; statusIndex: 4 }
        }

        Row {
            id: otherGaugeRow2
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            BarGauge { label: "冷却液温度1"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp1 : 0; maxValue: 150; statusIndex: 5 }
            BarGauge { label: "冷却液温度2"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp2 : 0; maxValue: 150; statusIndex: 5 }
            BarGauge { label: "冷却液温度3"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp3 : 0; maxValue: 150; statusIndex: 5 }
            BarGauge { label: "冷却液温度4"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp4 : 0; maxValue: 150; statusIndex: 5 }
            BarGauge {
                label: "歧管压力"
                unit: "hPa"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.manifoldPreA ) : 0
                maxValue: 1500
                statusIndex: 9
            }
            BarGauge {
                label: "燃油压差"
                unit: "bar"
                value: 0  // 暂未找到直接对应的变量，先置0
                maxValue: 5
                statusIndex: 11
            }
        }
    }
    // Values are bound to EngineStatusController when available; Timer simulation removed.
}
