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
