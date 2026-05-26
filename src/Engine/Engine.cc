// EngineStatusController implementation

#include "Engine.h"

#include "MAVLinkProtocol.h"
#include "MAVLinkLib.h"
#include "LinkInterface.h"

#include <QtCore/QDebug>
#include "QGCLoggingCategory.h"

#include <QDir>
#include <QtGlobal>
#include <QDateTime>
#include <QIODevice>
#include <QTextStream>
#include <cmath>
#include <cstring>

EngineStatusController::EngineStatusController(QObject *parent)
    : QObject(parent)
{
    // Connect to MAVLinkProtocol so we receive incoming MAVLink messages
    MAVLinkProtocol *const mavlinkProtocol = MAVLinkProtocol::instance();
    (void) connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &EngineStatusController::_receiveMessage);

    _engineDataLogTimer.setInterval(ENGINE_LOG_INTERVAL_MS);
    _engineDataLogTimer.setSingleShot(false);
    (void) connect(&_engineDataLogTimer, &QTimer::timeout, this, &EngineStatusController::_engineLogTimerTick);

    _engineConnectionStatusTimer.setInterval(100);
    _engineConnectionStatusTimer.setSingleShot(false);
    (void) connect(&_engineConnectionStatusTimer, &QTimer::timeout, this, &EngineStatusController::_engineConnectionStatusTimerTick);
    _engineConnectionStatusTimer.start();
}

EngineStatusController::~EngineStatusController()
{
    _stopEngineDataLog();
}

static qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

bool EngineStatusController::hasField(const QString &key) const
{
    return _lastFieldUpdateTime.contains(key) && _lastFieldUpdateTime.value(key) > 0;
}

QString EngineStatusController::_engineDataLogDirectory() const
{
    return QStringLiteral("E:/QGC/qgroundcontrol_lab/engine_data");
}

QString EngineStatusController::_uniqueEngineDataLogFilePath(qint64 timestampMs) const
{
    const QDir dir(_engineDataLogDirectory());
    const QString baseName = QStringLiteral("EngineSummary_%1")
        .arg(QDateTime::fromMSecsSinceEpoch(timestampMs).toString(QStringLiteral("yyyy-MM-dd_hh-mm-ss")));

    QString fileName = baseName + QStringLiteral(".csv");
    int duplicateIndex = 1;
    while (dir.exists(fileName)) {
        fileName = QStringLiteral("%1.%2.csv").arg(baseName).arg(duplicateIndex++);
    }

    return dir.absoluteFilePath(fileName);
}

static QString csvDouble(double value)
{
    return QString::number(value, 'f', 3);
}

static int twoBitState(quint32 value, int shift)
{
    return static_cast<int>((value >> shift) & 0x03);
}

static QString faultStateText(int state)
{
    switch (state) {
    case 1:
        return QStringLiteral("短路");
    case 2:
        return QStringLiteral("断路");
    default:
        return QString();
    }
}

void EngineStatusController::_updateFaultState(const QString &label, int state)
{
    state &= 0x03;
    if (_engineFaultStates.value(label, 0) == state) {
        return;
    }

    if (state == 0) {
        _engineFaultStates.remove(label);
    } else {
        _engineFaultStates.insert(label, state);
    }
}

void EngineStatusController::_rebuildEngineFaultMessages()
{
    QStringList messages;
    for (auto it = _engineFaultStates.constBegin(); it != _engineFaultStates.constEnd(); ++it) {
        const QString stateText = faultStateText(it.value());
        if (!stateText.isEmpty()) {
            messages.append(QStringLiteral("%1：%2").arg(it.key(), stateText));
        }
    }

    if (messages == _engineFaultMessages) {
        return;
    }

    _engineFaultMessages = messages;
    QMetaObject::invokeMethod(this, [this]() { emit engineFaultsChanged(); }, Qt::QueuedConnection);
}

void EngineStatusController::_startEngineDataLog(qint64 timestampMs)
{
    if (_engineDataLogFile.isOpen()) {
        if (!_engineDataLogTimer.isActive()) {
            _engineDataLogTimer.start();
        }
        return;
    }

    const QString logDirPath = _engineDataLogDirectory();
    QDir logDir(logDirPath);
    if (!logDir.exists() && !QDir().mkpath(logDirPath)) {
        qWarning() << "Unable to create engine data log directory:" << logDirPath;
        return;
    }

    _engineDataLogFile.setFileName(_uniqueEngineDataLogFilePath(timestampMs));
    if (!_engineDataLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Unable to open engine data log file:" << _engineDataLogFile.fileName() << _engineDataLogFile.errorString();
        return;
    }

    QTextStream out(&_engineDataLogFile);
    out << "timestamp_iso,timestamp_ms,data_class,last_message_type,last_array_id,"
        << "hun_msg_count,ten_msg_count,"
        << "speed_rpm,"
        << "throttle_opening_pct,throttle_position_pct,"
        << "electrical_voltage_v,"
        << "temperature_oil_c,temperature_intake_c,"
        << "temperature_exhaust_1_c,temperature_exhaust_2_c,temperature_exhaust_3_c,temperature_exhaust_4_c,"
        << "temperature_coolant_1_c,temperature_coolant_2_c,temperature_coolant_3_c,temperature_coolant_4_c,"
        << "pressure_oil_bar,pressure_manifold_hpa,pressure_fuel_bar\n";
    (void) _engineDataLogFile.flush();
    _engineDataLogTimer.start();
}

void EngineStatusController::_stopEngineDataLog()
{
    if (_engineDataLogTimer.isActive()) {
        _engineDataLogTimer.stop();
    }

    if (_engineDataLogFile.isOpen()) {
        (void) _engineDataLogFile.flush();
        _engineDataLogFile.close();
    }
}

void EngineStatusController::_noteEngineDataReceived(const QString &messageType, int arrayId, qint64 timestampMs)
{
    _lastEngineDataTimeMs = timestampMs;
    _lastEngineMessageType = messageType;
    _lastEngineArrayId = arrayId;
    if (!_engineDataConnected) {
        _engineDataConnected = true;
        QMetaObject::invokeMethod(this, [this]() { emit engineDataConnectedChanged(); }, Qt::QueuedConnection);
    }
    _startEngineDataLog(timestampMs);
}

void EngineStatusController::_writeEngineDataLogRow(qint64 timestampMs)
{
    if (!_engineDataLogFile.isOpen()) {
        return;
    }

    QTextStream out(&_engineDataLogFile);
    out << QDateTime::fromMSecsSinceEpoch(timestampMs).toString(Qt::ISODateWithMs) << ','
        << timestampMs << ','
        << "engine_summary_1hz" << ','
        << _lastEngineMessageType << ','
        << _lastEngineArrayId << ','
        << _hunMsgCount << ','
        << _tenMsgCount << ','
        << csvDouble(_rpm) << ','
        << csvDouble(_throttleOpeningSendVal) << ','
        << csvDouble(_throttlePosA) << ','
        << csvDouble(_voltage) << ','
        << csvDouble(_temperature) << ','
        << csvDouble(_intakeTemp) << ','
        << csvDouble(_exhaustTemp1) << ','
        << csvDouble(_exhaustTemp2) << ','
        << csvDouble(_exhaustTemp3) << ','
        << csvDouble(_exhaustTemp4) << ','
        << csvDouble(_coolantTemp1) << ','
        << csvDouble(_coolantTemp2) << ','
        << csvDouble(_coolantTemp3) << ','
        << csvDouble(_coolantTemp4) << ','
        << csvDouble(_oilPressure) << ','
        << csvDouble(_manifoldPreA) << ','
        << csvDouble(_fuelPressure) << '\n';
    (void) _engineDataLogFile.flush();
}

void EngineStatusController::_engineLogTimerTick()
{
    const qint64 timestampMs = nowMs();
    if ((_lastEngineDataTimeMs == 0) || ((timestampMs - _lastEngineDataTimeMs) > ENGINE_DATA_STOP_TIMEOUT_MS)) {
        _stopEngineDataLog();
        return;
    }

    _writeEngineDataLogRow(timestampMs);
}

void EngineStatusController::_engineConnectionStatusTimerTick()
{
    const qint64 timestampMs = nowMs();
    const bool connected = (_lastEngineDataTimeMs != 0) && ((timestampMs - _lastEngineDataTimeMs) <= ENGINE_DATA_CONNECTION_TIMEOUT_MS);
    if (_engineDataConnected != connected) {
        _engineDataConnected = connected;
        QMetaObject::invokeMethod(this, [this]() { emit engineDataConnectedChanged(); }, Qt::QueuedConnection);
    }
    if (!connected && !_engineFaultMessages.isEmpty()) {
        _engineFaultStates.clear();
        _engineFaultMessages.clear();
        QMetaObject::invokeMethod(this, [this]() { emit engineFaultsChanged(); }, Qt::QueuedConnection);
    }
}



static float safeRead(const mavlink_debug_float_array_t &m, int idx)
{
    // mavlink DEBUG_FLOAT_ARRAY has data up to 56 entries
    if (idx < 0 || idx >= (int)MAVLINK_MSG_DEBUG_FLOAT_ARRAY_FIELD_DATA_LEN) {
        return 0.0f;
    }
    return m.data[idx];
}

// Member helpers to update fields with hysteresis and signal emission
void EngineStatusController::_updateIntField(const QString &key, int &field, int newVal, void (EngineStatusController::*signal)())
{
    if (field == newVal) return;

    qint64 t = nowMs();
    field = newVal;
    _lastFieldUpdateTime.insert(key, t);
    if (newVal != 0) {
        _lastFieldNonZeroTime.insert(key, t);
    }
    _lastFieldValue.insert(key, (double)newVal);
    QMetaObject::invokeMethod(this, [this, signal]() { (this->*signal)(); }, Qt::QueuedConnection);
}

void EngineStatusController::_updateDoubleField(const QString &key, double &field, double newVal, void (EngineStatusController::*signal)())
{
    // float precision check to prevent continuous UI updates
    if (std::abs(field - newVal) < 0.001) return;

    qint64 t = nowMs();
    field = newVal;
    _lastFieldUpdateTime.insert(key, t);
    if (newVal != 0.0) {
        _lastFieldNonZeroTime.insert(key, t);
    }
    _lastFieldValue.insert(key, newVal);
    QMetaObject::invokeMethod(this, [this, signal]() { (this->*signal)(); }, Qt::QueuedConnection);
}

void EngineStatusController:: _receiveMessage(const LinkInterface* /*link*/, const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY) {
        return;
    }

    mavlink_debug_float_array_t dbg{};
    mavlink_msg_debug_float_array_decode(&message, &dbg);

    // message name may be shorter; compare with prefix
    const char *name = dbg.name;
    int array_id = dbg.array_id;
    qint64 t = nowMs();

    // compare names
    if (strncmp(name, "eng_hun", sizeof(dbg.name)) == 0) {
        _noteEngineDataReceived(QStringLiteral("eng_hun"), array_id, t);

        // record HUN receive time for this source
        _lastHunReceiveTime.insert(array_id, t);
        // diagnostic counter
        _hunMsgCount++;
        QMetaObject::invokeMethod(this, [this]() { emit hunMsgCountChanged(); }, Qt::QueuedConnection);

        // parse according to UAVCAN_ENGINE_STATUS(1).hpp mapping
    // data[2] = manifold_temp_a_c -> intakeTemp
    _updateDoubleField("intakeTemp", _intakeTemp, (double)safeRead(dbg, 2), &EngineStatusController::intakeTempChanged);
    // data[4] = manifold_pre_a_kpa -> manifoldPreA
    _updateDoubleField("manifoldPreA", _manifoldPreA, (double)safeRead(dbg, 4), &EngineStatusController::manifoldPreAChanged);

        // cyl coolant temps 6..9 -> coolantTemp1..4
        _updateDoubleField("coolantTemp1", _coolantTemp1, (double)safeRead(dbg, 6), &EngineStatusController::coolantTemp1Changed);
        _updateDoubleField("coolantTemp2", _coolantTemp2, (double)safeRead(dbg, 7), &EngineStatusController::coolantTemp2Changed);
        _updateDoubleField("coolantTemp3", _coolantTemp3, (double)safeRead(dbg, 8), &EngineStatusController::coolantTemp3Changed);
        _updateDoubleField("coolantTemp4", _coolantTemp4, (double)safeRead(dbg, 9), &EngineStatusController::coolantTemp4Changed);

        // engine speeds
        double rpmA = (double)safeRead(dbg, 10);
        double rpmB = (double)safeRead(dbg, 11);
        // prefer A over B
        if (rpmA != 0.0) {
            // rpm hysteresis: update if change exceeds threshold or previously zero
            if (std::abs(_rpm - rpmA) >= EngineStatusController::RPM_UPDATE_THRESHOLD || _rpm == 0.0) {
                _updateDoubleField("rpm", _rpm, rpmA, &EngineStatusController::rpmChanged);
            } else {
                _lastFieldUpdateTime.insert("rpm", t);
            }
        } else if (rpmB != 0.0) {
            if (std::abs(_rpm - rpmB) >= EngineStatusController::RPM_UPDATE_THRESHOLD || _rpm == 0.0) {
                _updateDoubleField("rpm", _rpm, rpmB, &EngineStatusController::rpmChanged);
            } else {
                _lastFieldUpdateTime.insert("rpm", t);
            }
        }

        // throttle pos sens a/b
    _updateDoubleField("throttlePosA", _throttlePosA, (double)safeRead(dbg, 12), &EngineStatusController::throttlePosAChanged);

        // data[16] = engine mode feedback
        _updateIntField("engineMode", _engineMode, static_cast<int>(safeRead(dbg, 16)), &EngineStatusController::engineModeChanged);

        const quint32 sensStateManifold = static_cast<quint32>(safeRead(dbg, 22)) & 0xff;
        _updateFaultState(QStringLiteral("歧管压力传感器AECU"), twoBitState(sensStateManifold, 0));
        _updateFaultState(QStringLiteral("歧管压力传感器BECU"), twoBitState(sensStateManifold, 2));
        _updateFaultState(QStringLiteral("歧管温度传感器AECU"), twoBitState(sensStateManifold, 4));
        _updateFaultState(QStringLiteral("歧管温度传感器BECU"), twoBitState(sensStateManifold, 6));

        const quint32 sensStateCoolant = static_cast<quint32>(safeRead(dbg, 23)) & 0xff;
        _updateFaultState(QStringLiteral("1缸冷却液温度传感器"), twoBitState(sensStateCoolant, 0));
        _updateFaultState(QStringLiteral("2缸冷却液温度传感器"), twoBitState(sensStateCoolant, 2));
        _updateFaultState(QStringLiteral("3缸冷却液温度传感器"), twoBitState(sensStateCoolant, 4));
        _updateFaultState(QStringLiteral("4缸冷却液温度传感器"), twoBitState(sensStateCoolant, 6));

        const quint32 sensStateThrottle = static_cast<quint32>(safeRead(dbg, 24)) & 0xff;
        _updateFaultState(QStringLiteral("节气门A位置传感器"), twoBitState(sensStateThrottle, 0));
        _updateFaultState(QStringLiteral("节气门电机"), twoBitState(sensStateThrottle, 2));
        _updateFaultState(QStringLiteral("节气门B位置传感器"), twoBitState(sensStateThrottle, 4));

        const quint32 sensStateCurvedKnock = static_cast<quint32>(safeRead(dbg, 25)) & 0xff;
        _updateFaultState(QStringLiteral("曲位传感器A"), twoBitState(sensStateCurvedKnock, 0));
        _updateFaultState(QStringLiteral("爆震传感器"), twoBitState(sensStateCurvedKnock, 2));
        _updateFaultState(QStringLiteral("曲位传感器B"), twoBitState(sensStateCurvedKnock, 4));
        _updateFaultState(QStringLiteral("爆震发生状态"), twoBitState(sensStateCurvedKnock, 6));

        const quint32 sensStateInjA = static_cast<quint32>(safeRead(dbg, 26)) & 0xff;
        _updateFaultState(QStringLiteral("喷油器A2"), twoBitState(sensStateInjA, 0));
        _updateFaultState(QStringLiteral("喷油器A1"), twoBitState(sensStateInjA, 2));
        _updateFaultState(QStringLiteral("喷油器A4"), twoBitState(sensStateInjA, 4));
        _updateFaultState(QStringLiteral("喷油器A3"), twoBitState(sensStateInjA, 6));

        const quint32 sensStateInjB = static_cast<quint32>(safeRead(dbg, 27)) & 0xff;
        _updateFaultState(QStringLiteral("喷油器B2"), twoBitState(sensStateInjB, 0));
        _updateFaultState(QStringLiteral("喷油器B1"), twoBitState(sensStateInjB, 2));
        _updateFaultState(QStringLiteral("喷油器B4"), twoBitState(sensStateInjB, 4));
        _updateFaultState(QStringLiteral("喷油器B3"), twoBitState(sensStateInjB, 6));

        const quint32 sensStateIgnCoil = static_cast<quint32>(safeRead(dbg, 28)) & 0xff;
        _updateFaultState(QStringLiteral("点火线圈A34"), twoBitState(sensStateIgnCoil, 0));
        _updateFaultState(QStringLiteral("点火线圈A12"), twoBitState(sensStateIgnCoil, 2));
        _updateFaultState(QStringLiteral("点火线圈B34"), twoBitState(sensStateIgnCoil, 4));
        _updateFaultState(QStringLiteral("点火线圈B12"), twoBitState(sensStateIgnCoil, 6));

        const quint32 sensStatePwm = static_cast<quint32>(safeRead(dbg, 29)) & 0xff;
        _updateFaultState(QStringLiteral("主喷油器组判定状态"), twoBitState(sensStatePwm, 0));
        _updateFaultState(QStringLiteral("系统切换线PWM输出"), twoBitState(sensStatePwm, 2));
        _updateFaultState(QStringLiteral("PWM信号线诊断状态ECUB"), twoBitState(sensStatePwm, 4));
        _updateFaultState(QStringLiteral("PWM输出ECUB"), twoBitState(sensStatePwm, 6));

        _rebuildEngineFaultMessages();

        return;
    }

    // TEN messages
    if (strncmp(name, "eng_ten", sizeof(dbg.name)) == 0) {
        _noteEngineDataReceived(QStringLiteral("eng_ten"), array_id, t);

        _lastTenReceiveTime.insert(array_id, t);
        // diagnostic counter
        _tenMsgCount++;
        QMetaObject::invokeMethod(this, [this]() { emit tenMsgCountChanged(); }, Qt::QueuedConnection);

        // If we have a recent HUN for this source, prefer HUN and skip updating fields that HUN covers.
        qint64 lastHun = _lastHunReceiveTime.value(array_id, 0);
        bool hunRecent = (lastHun != 0) && ((t - lastHun) < HUN_PREFERRED_MS);

        _updateDoubleField("fuelPressure", _fuelPressure, (double)safeRead(dbg, 8), &EngineStatusController::fuelPressureChanged);
        _updateDoubleField("oilPressure", _oilPressure, (double)safeRead(dbg, 9), &EngineStatusController::oilPressureChanged);
        _updateDoubleField("temperature", _temperature, (double)safeRead(dbg, 10), &EngineStatusController::temperatureChanged);
        _updateDoubleField("throttleOpeningSendVal", _throttleOpeningSendVal, (double)safeRead(dbg, 14), &EngineStatusController::throttleOpeningSendValChanged);

        double tenVoltage = (double)safeRead(dbg, 18);
        _updateDoubleField("voltage", _voltage, tenVoltage, &EngineStatusController::voltageChanged);

        _updateDoubleField("throttleRequestFeedback", _throttleRequestFeedback, (double)safeRead(dbg, 29), &EngineStatusController::throttleRequestFeedbackChanged);
        _updateDoubleField("rpmRequestFeedback", _rpmRequestFeedback, (double)safeRead(dbg, 31), &EngineStatusController::rpmRequestFeedbackChanged);

        _updateDoubleField("exhaustTemp1", _exhaustTemp1, (double)safeRead(dbg, 20), &EngineStatusController::exhaustTemp1Changed);
        _updateDoubleField("exhaustTemp2", _exhaustTemp2, (double)safeRead(dbg, 21), &EngineStatusController::exhaustTemp2Changed);
        _updateDoubleField("exhaustTemp3", _exhaustTemp3, (double)safeRead(dbg, 22), &EngineStatusController::exhaustTemp3Changed);
        _updateDoubleField("exhaustTemp4", _exhaustTemp4, (double)safeRead(dbg, 23), &EngineStatusController::exhaustTemp4Changed);

        _updateIntField("fanStatusBits", _fanStatusBits, static_cast<int>(safeRead(dbg, 45)), &EngineStatusController::fanStatusBitsChanged);

        const quint32 sensStateOxygen = static_cast<quint32>(safeRead(dbg, 39)) & 0xff;
        _updateFaultState(QStringLiteral("氧传感器A"), twoBitState(sensStateOxygen, 0));
        _updateFaultState(QStringLiteral("氧传感器B"), twoBitState(sensStateOxygen, 2));
        _updateFaultState(QStringLiteral("滑油液位传感器"), twoBitState(sensStateOxygen, 4));

        const quint32 sensStateCylExhTemp = static_cast<quint32>(safeRead(dbg, 40)) & 0xff;
        _updateFaultState(QStringLiteral("1缸排温传感器"), twoBitState(sensStateCylExhTemp, 0));
        _updateFaultState(QStringLiteral("2缸排温传感器"), twoBitState(sensStateCylExhTemp, 2));
        _updateFaultState(QStringLiteral("3缸排温传感器"), twoBitState(sensStateCylExhTemp, 4));
        _updateFaultState(QStringLiteral("4缸排温传感器"), twoBitState(sensStateCylExhTemp, 6));

        const quint32 sensStateOilFuel = static_cast<quint32>(safeRead(dbg, 41)) & 0xff;
        _updateFaultState(QStringLiteral("机油压力传感器"), twoBitState(sensStateOilFuel, 0));
        _updateFaultState(QStringLiteral("燃油压力传感器"), twoBitState(sensStateOilFuel, 2));
        _updateFaultState(QStringLiteral("机油温度传感器"), twoBitState(sensStateOilFuel, 4));

        const quint32 sensStateBoostExh = static_cast<quint32>(safeRead(dbg, 42)) & 0xff;
        _updateFaultState(QStringLiteral("增压压力传感器"), twoBitState(sensStateBoostExh, 0));
        _updateFaultState(QStringLiteral("增压温度传感器"), twoBitState(sensStateBoostExh, 2));
        _updateFaultState(QStringLiteral("废气阀电机位置传感器"), twoBitState(sensStateBoostExh, 4));
        _updateFaultState(QStringLiteral("废气阀电机"), twoBitState(sensStateBoostExh, 6));

        const quint32 sensStateAtmosphericTemp = static_cast<quint32>(safeRead(dbg, 43)) & 0xff;
        _updateFaultState(QStringLiteral("A大气压力传感器"), twoBitState(sensStateAtmosphericTemp, 0));
        _updateFaultState(QStringLiteral("B大气压力传感器"), twoBitState(sensStateAtmosphericTemp, 2));
        _updateFaultState(QStringLiteral("环境温度传感器"), twoBitState(sensStateAtmosphericTemp, 4));

        const quint32 msgStateCan = static_cast<quint32>(safeRead(dbg, 44)) & 0xff;
        _updateFaultState(QStringLiteral("CAN通信1 ECUA"), twoBitState(msgStateCan, 0));
        _updateFaultState(QStringLiteral("CAN通信2 ECUA"), twoBitState(msgStateCan, 2));
        _updateFaultState(QStringLiteral("CAN通信1 ECUB"), twoBitState(msgStateCan, 4));
        _updateFaultState(QStringLiteral("CAN通信2 ECUB"), twoBitState(msgStateCan, 6));

        const quint32 msgVoltState422Volt = static_cast<quint32>(safeRead(dbg, 45)) & 0xff;
        _updateFaultState(QStringLiteral("RS422通信 ECUA"), twoBitState(msgVoltState422Volt, 0));
        _updateFaultState(QStringLiteral("电压 ECUA"), twoBitState(msgVoltState422Volt, 2));
        _updateFaultState(QStringLiteral("RS422通信 ECUB"), twoBitState(msgVoltState422Volt, 4));
        _updateFaultState(QStringLiteral("电压 ECUB"), twoBitState(msgVoltState422Volt, 6));

        _rebuildEngineFaultMessages();

        // TEN rpm at data[49] - only use if we don't have a recent HUN for this source
        if (!hunRecent) {
            double tenRpm = (double)safeRead(dbg, 49);
            if (tenRpm != 0.0) {
                if (std::abs(_rpm - tenRpm) >= EngineStatusController::RPM_UPDATE_THRESHOLD || _rpm == 0.0) {
                    _updateDoubleField("rpm", _rpm, tenRpm, &EngineStatusController::rpmChanged);
                } else {
                    _lastFieldUpdateTime.insert("rpm", t);
                }
            }
        } else {
            // If HUN is recent, still refresh the rpm timestamp so hasField remains recent
            _lastFieldUpdateTime.insert("rpm", t);
        }
    }
}
