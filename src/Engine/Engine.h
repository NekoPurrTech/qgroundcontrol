// Minimal Engine status controller exposed to QML for displaying engine sensor data
#pragma once

#include <QObject>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"
#include <QMap>
#include <QtGlobal>

class LinkInterface;

class EngineStatusController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(double rpm READ rpm NOTIFY rpmChanged)
	Q_PROPERTY(double oilPressure READ oilPressure NOTIFY oilPressureChanged)
	Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
	Q_PROPERTY(double voltage READ voltage NOTIFY voltageChanged)
	Q_PROPERTY(double intakeTemp READ intakeTemp NOTIFY intakeTempChanged)
	Q_PROPERTY(double exhaustTemp1 READ exhaustTemp1 NOTIFY exhaustTemp1Changed)
	Q_PROPERTY(double exhaustTemp2 READ exhaustTemp2 NOTIFY exhaustTemp2Changed)
	Q_PROPERTY(double exhaustTemp3 READ exhaustTemp3 NOTIFY exhaustTemp3Changed)
	Q_PROPERTY(double exhaustTemp4 READ exhaustTemp4 NOTIFY exhaustTemp4Changed)
	Q_PROPERTY(double coolantTemp1 READ coolantTemp1 NOTIFY coolantTemp1Changed)
	Q_PROPERTY(double coolantTemp2 READ coolantTemp2 NOTIFY coolantTemp2Changed)
	Q_PROPERTY(double coolantTemp3 READ coolantTemp3 NOTIFY coolantTemp3Changed)
	Q_PROPERTY(double coolantTemp4 READ coolantTemp4 NOTIFY coolantTemp4Changed)
	Q_PROPERTY(double fuelPressure READ fuelPressure NOTIFY fuelPressureChanged)
	Q_PROPERTY(double manifoldPreA READ manifoldPreA NOTIFY manifoldPreAChanged)
	Q_PROPERTY(double throttlePosA READ throttlePosA NOTIFY throttlePosAChanged)
	Q_PROPERTY(double throttleOpeningSendVal READ throttleOpeningSendVal NOTIFY throttleOpeningSendValChanged)
	Q_PROPERTY(int hunMsgCount READ hunMsgCount NOTIFY hunMsgCountChanged)
	Q_PROPERTY(int tenMsgCount READ tenMsgCount NOTIFY tenMsgCountChanged)
	Q_PROPERTY(bool engineDataConnected READ engineDataConnected NOTIFY engineDataConnectedChanged)
	Q_PROPERTY(int engineMode READ engineMode NOTIFY engineModeChanged)
	Q_PROPERTY(double throttleRequestFeedback READ throttleRequestFeedback NOTIFY throttleRequestFeedbackChanged)
	Q_PROPERTY(double rpmRequestFeedback READ rpmRequestFeedback NOTIFY rpmRequestFeedbackChanged)
	Q_PROPERTY(int fanStatusBits READ fanStatusBits NOTIFY fanStatusBitsChanged)
	Q_PROPERTY(QStringList engineFaults READ engineFaults NOTIFY engineFaultsChanged)

public:
	explicit EngineStatusController(QObject *parent = nullptr);
	~EngineStatusController() override;

	Q_INVOKABLE bool hasField(const QString &key) const;


	double rpm() const { return _rpm; }
	double oilPressure() const { return _oilPressure; }
	double temperature() const { return _temperature; }
	double voltage() const { return _voltage; }
	double intakeTemp() const { return _intakeTemp; }
	double exhaustTemp1() const { return _exhaustTemp1; }
	double exhaustTemp2() const { return _exhaustTemp2; }
	double exhaustTemp3() const { return _exhaustTemp3; }
	double exhaustTemp4() const { return _exhaustTemp4; }
	double coolantTemp1() const { return _coolantTemp1; }
	double coolantTemp2() const { return _coolantTemp2; }
	double coolantTemp3() const { return _coolantTemp3; }
	double coolantTemp4() const { return _coolantTemp4; }
	double fuelPressure() const { return _fuelPressure; }
	double manifoldPreA() const { return _manifoldPreA; }
	double throttlePosA() const { return _throttlePosA; }
	double throttleOpeningSendVal() const { return _throttleOpeningSendVal; }
	bool engineDataConnected() const { return _engineDataConnected; }
	int engineMode() const { return _engineMode; }
	double throttleRequestFeedback() const { return _throttleRequestFeedback; }
	double rpmRequestFeedback() const { return _rpmRequestFeedback; }
	int fanStatusBits() const { return _fanStatusBits; }
	QStringList engineFaults() const { return _engineFaultMessages; }

public slots:
	void _receiveMessage(const LinkInterface* link, const mavlink_message_t &message);

private slots:
	void _engineLogTimerTick();
	void _engineConnectionStatusTimerTick();

public:
	int hunMsgCount() const { return _hunMsgCount; }
	int tenMsgCount() const { return _tenMsgCount; }

signals:
	void rpmChanged();
	void oilPressureChanged();
	void temperatureChanged();
	void voltageChanged();
	void intakeTempChanged();
	void exhaustTemp1Changed();
	void exhaustTemp2Changed();
	void exhaustTemp3Changed();
	void exhaustTemp4Changed();
	void coolantTemp1Changed();
	void coolantTemp2Changed();
	void coolantTemp3Changed();
	void coolantTemp4Changed();
	void fuelPressureChanged();
	void manifoldPreAChanged();
	void throttlePosAChanged();
	void throttleOpeningSendValChanged();

	void hunMsgCountChanged();
	void tenMsgCountChanged();
	void engineDataConnectedChanged();
	void engineModeChanged();
	void throttleRequestFeedbackChanged();
	void rpmRequestFeedbackChanged();
	void fanStatusBitsChanged();
	void engineFaultsChanged();

private:
	double _rpm = 0.0;
	double _oilPressure = 0.0;
	double _temperature = 0.0;
	double _voltage = 0.0;
	double _intakeTemp = 0.0;
	double _exhaustTemp1 = 0.0;
	double _exhaustTemp2 = 0.0;
	double _exhaustTemp3 = 0.0;
	double _exhaustTemp4 = 0.0;
	double _coolantTemp1 = 0.0;
	double _coolantTemp2 = 0.0;
	double _coolantTemp3 = 0.0;
	double _coolantTemp4 = 0.0;
	double _fuelPressure = 0.0;
	// additional HUN fields
	double _manifoldPreA = 0.0;
	double _throttlePosA = 0.0;
	// additional HUN tail fields
	double _throttleOpeningSendVal = 0.0;
	int _engineMode = 0;
	double _throttleRequestFeedback = 0.0;
	double _rpmRequestFeedback = 0.0;
	int _fanStatusBits = 0;
	QStringList _engineFaultMessages;
	QMap<QString, int> _engineFaultStates;
	// Track last-received timestamps per array_id (source) so we can prefer HUN data
	// and only use TEN when HUN hasn't been received recently.
	QMap<int, qint64> _lastHunReceiveTime;
	QMap<int, qint64> _lastTenReceiveTime;
	// How long to prefer HUN data (ms) before falling back to TEN
	static const qint64 HUN_PREFERRED_MS = 2000;
	// Per-field hysteresis to avoid blinking between valid values and transient zeros
	QMap<QString, double> _lastFieldValue;
	QMap<QString, qint64> _lastFieldUpdateTime;
	QMap<QString, qint64> _lastFieldNonZeroTime;
	static const qint64 ZERO_SUSTAIN_MS = 2000; // accept zero only after sustained zeros
	static constexpr double RPM_UPDATE_THRESHOLD = 25.0; // rpm change threshold

	// Non-logging counters for diagnostics
	int _hunMsgCount = 0;
	int _tenMsgCount = 0;

    // Internal helpers implemented in Engine.cc. Declared here so implementations
    // can access private members safely.
    void _updateIntField(const QString &key, int &field, int newVal, void (EngineStatusController::*signal)());
    void _updateDoubleField(const QString &key, double &field, double newVal, void (EngineStatusController::*signal)());
    void _noteEngineDataReceived(const QString &messageType, int arrayId, qint64 timestampMs);
    void _startEngineDataLog(qint64 timestampMs);
    void _stopEngineDataLog();
    void _writeEngineDataLogRow(qint64 timestampMs);
    QString _engineDataLogDirectory() const;
    QString _uniqueEngineDataLogFilePath(qint64 timestampMs) const;
    void _updateFaultState(const QString &label, int state);
    void _rebuildEngineFaultMessages();

    QFile _engineDataLogFile;
    QTimer _engineDataLogTimer;
    QTimer _engineConnectionStatusTimer;
    qint64 _lastEngineDataTimeMs = 0;
    QString _lastEngineMessageType;
    int _lastEngineArrayId = -1;
    bool _engineDataConnected = false;
    static const int ENGINE_LOG_INTERVAL_MS = 1000;
    static const qint64 ENGINE_DATA_STOP_TIMEOUT_MS = 2500;
    static const qint64 ENGINE_DATA_CONNECTION_TIMEOUT_MS = 5000;
};
