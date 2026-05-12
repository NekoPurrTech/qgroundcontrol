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


    component BarGauge: Rectangle {
        id: gauge
        width: 70
        height: 240
        color: "#2a2a2a"
        border.color: "#555"
        border.width: 1
        radius: 4

        property string label: ""
        property real value: 0
        property real maxValue: 100
        property string unit: ""
        property color barColor: "#2196F3"
        property real percentage: Math.min(Math.max(value / maxValue, 0), 1)

        Column {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            Text {
                width: parent.width
                text: gauge.value.toFixed(1)
                color: "white"
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
                    color: gauge.percentage > 0.8 ? "#f44336" : gauge.barColor
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

    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        spacing: 20

        ToggleSwitchAmber {
            id: pump1
            text: "Fuel Pump 1"
            onCheckedChanged: root.sendPumpFanBitmask()
        }

        ToggleSwitchAmber {
            id: pump2
            text: "Fuel Pump 2"
            onCheckedChanged: root.sendPumpFanBitmask()
        }

        ToggleSwitchBlue {
            id: fan1
            text: "Water Cooler Fan 1"
            onCheckedChanged: root.sendPumpFanBitmask()
        }

        ToggleSwitchBlue {
            id: fan2
            text: "Water Cooler Fan 2"
            onCheckedChanged: root.sendPumpFanBitmask()
        }

        ToggleSwitchBlue {
            id: intercooler
            text: "Inter Cooler Fan"
            onCheckedChanged: root.sendPumpFanBitmask()
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
            width: 180
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

    // 按照图表要求重构布局：分为3排，完全替换以前的右侧列和下方网格
    Column {
        id: mappedGrid
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        spacing: 30

        // 第1排：序号 1, 7, 8, 12
        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            BarGauge {
                label: "发动机转速"
                unit: "r/min"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.rpm : 0
                maxValue: 6500
            }
            BarGauge {
                label: "废气阀位置"
                unit: "%"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.throttleOpeningSendVal : 0
                maxValue: 100
            }
            BarGauge {
                label: "节气门位置"
                unit: "%"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.throttlePosA : 0
                maxValue: 100
            }
            BarGauge {
                label: "ECU电源电压"
                unit: "V"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.voltage : 0
                maxValue: 16
            }
        }

        // 第2排：序号 2, 4(四个), 5(四个), 6
        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            // (2) 滑油温度
            BarGauge {
                label: "滑油温度"
                unit: "℃"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.temperature : 0
                maxValue: 150
            }

            // (4) 排气温度 (四个)
            BarGauge { label: "排气温度1"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp1 : 0; maxValue: 1000 }
            BarGauge { label: "排气温度2"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp2 : 0; maxValue: 1000 }
            BarGauge { label: "排气温度3"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp3 : 0; maxValue: 1000 }
            BarGauge { label: "排气温度4"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.exhaustTemp4 : 0; maxValue: 1000 }

            // (5) 冷却液温度 (四个)
            BarGauge { label: "冷却液温度1"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp1 : 0; maxValue: 150 }
            BarGauge { label: "冷却液温度2"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp2 : 0; maxValue: 150 }
            BarGauge { label: "冷却液温度3"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp3 : 0; maxValue: 150 }
            BarGauge { label: "冷却液温度4"; unit: "℃"; value: (typeof engineCtrl !== 'undefined') ? engineCtrl.coolantTemp4 : 0; maxValue: 150 }

            // (6) 歧管温度
            BarGauge {
                label: "歧管温度"
                unit: "℃"
                value: (typeof engineCtrl !== 'undefined') ? engineCtrl.intakeTemp : 0
                maxValue: 100
            }
        }

        // 第3排：序号 3, 9, 10, 11
        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            BarGauge {
                label: "滑油压力"
                unit: "bar"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.oilPressure ) : 0
                maxValue: 8
            }
            BarGauge {
                label: "歧管压力"
                unit: "hPa"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.manifoldPreA ) : 0
                maxValue: 1500
            }
            BarGauge {
                label: "燃油压力"
                unit: "bar"
                value: (typeof engineCtrl !== 'undefined') ? (engineCtrl.fuelPressure ) : 0
                maxValue: 6
            }
            BarGauge {
                label: "燃油压差"
                unit: "bar"
                value: 0  // 暂未找到直接对应的变量，先置0
                maxValue: 5
            }
        }
    }
    // Values are bound to EngineStatusController when available; Timer simulation removed.
}
