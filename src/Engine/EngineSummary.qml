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
    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        spacing: 20

        ToggleSwitchAmber {
            id: pump1
            text: "Fuel Pump 1"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchAmber {
            id: pump2
            text: "Fuel Pump 2"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchBlue {
            id: fan1
            text: "Water Cooler Fan 1"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchBlue {
            id: fan2
            text: "Water Cooler Fan 2"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchBlue {
            id: intercooler
            text: "Inter Cooler Fan"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchGreen {
            id: engStart
            text: "Engine Start"
            // onCheckedChanged handled by central keepalive timer
        }

        ToggleSwitchRed {
            id: estop
            text: "Emergency STOP"
            // onCheckedChanged handled by central keepalive timer
        }
    }

    // Central keepalive timer: periodically refresh active actuator channels so ActuatorTesting watchdog
    // doesn't stop them. When any toggle is checked this timer runs and repeatedly sends setChannelTo.
    property bool anyActuatorActive: pump1.checked || pump2.checked || fan1.checked || fan2.checked || intercooler.checked || engStart.checked || estop.checked

    Timer {
        id: actuatorKeepalive
        interval: 80
        repeat: true
        running: anyActuatorActive
        onTriggered: {
            // Prefer globals.activeVehicle when available (matches Actuator UI). Fall back to 'vehicle' if present.
            var v = null
            if (typeof globals !== 'undefined' && globals.activeVehicle) {
                v = globals.activeVehicle
            } else if (typeof vehicle !== 'undefined') {
                v = vehicle
            }

            if (!v || !v.actuators || !v.actuators.actuatorTest) {
                return
            }

            var at = v.actuators.actuatorTest

            // If any active, ensure actuator testing is active
            if (anyActuatorActive) {
                at.setActive(true)

                // write each active channel repeatedly
                if (pump1.checked) at.setChannelTo(0, 4000.0)
                else at.stopControl(0)

                if (pump2.checked) at.setChannelTo(1, 4000.0)
                else at.stopControl(1)

                if (fan1.checked) at.setChannelTo(2, 4000.0)
                else at.stopControl(2)

                if (fan2.checked) at.setChannelTo(3, 4000.0)
                else at.stopControl(3)

                if (intercooler.checked) at.setChannelTo(4, 4000.0)
                else at.stopControl(4)

                if (engStart.checked) at.setChannelTo(5, 4000.0)
                else at.stopControl(5)

                if (estop.checked) at.setChannelTo(6, 4000.0)
                else at.stopControl(6)
            } else {
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
