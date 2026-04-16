// Minimal Engine status controller exposed to QML for displaying engine sensor data
#pragma once

#include <QObject>

#include "MAVLinkLib.h"

class LinkInterface;

class EngineStatusController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int rpm READ rpm NOTIFY rpmChanged)
	Q_PROPERTY(double oilPressure READ oilPressure NOTIFY oilPressureChanged)
	Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
	Q_PROPERTY(double voltage READ voltage NOTIFY voltageChanged)
	Q_PROPERTY(double intakeTemp READ intakeTemp NOTIFY intakeTempChanged)
	Q_PROPERTY(double exhaustTemp READ exhaustTemp NOTIFY exhaustTempChanged)
	Q_PROPERTY(double fuelPressure READ fuelPressure NOTIFY fuelPressureChanged)
	Q_PROPERTY(double supplyVoltage READ supplyVoltage NOTIFY supplyVoltageChanged)

public:
	explicit EngineStatusController(QObject *parent = nullptr);

	int rpm() const { return _rpm; }
	double oilPressure() const { return _oilPressure; }
	double temperature() const { return _temperature; }
	double voltage() const { return _voltage; }
	double intakeTemp() const { return _intakeTemp; }
	double exhaustTemp() const { return _exhaustTemp; }
	double fuelPressure() const { return _fuelPressure; }
	double supplyVoltage() const { return _supplyVoltage; }

public slots:
	void _receiveMessage(const LinkInterface* link, const mavlink_message_t &message);

signals:
	void rpmChanged();
	void oilPressureChanged();
	void temperatureChanged();
	void voltageChanged();
	void intakeTempChanged();
	void exhaustTempChanged();
	void fuelPressureChanged();
	void supplyVoltageChanged();

private:
	int _rpm = 0;
	double _oilPressure = 0.0;
	double _temperature = 0.0;
	double _voltage = 0.0;
	double _intakeTemp = 0.0;
	double _exhaustTemp = 0.0;
	double _fuelPressure = 0.0;
	double _supplyVoltage = 0.0;
};

