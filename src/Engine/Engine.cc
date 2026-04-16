#include "Engine.h"

#include "MAVLinkProtocol.h"
#include "MAVLinkLib.h"
#include "LinkInterface.h"

#include <QtCore/QDebug>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(EngineLog, "qgc.engine")

#include <QtCore/QByteArray>
#include <algorithm>
#include <cstring>

EngineStatusController::EngineStatusController(QObject *parent)
	: QObject(parent)
{
	// Subscribe to global MAVLink messages
	if (MAVLinkProtocol::instance()) {
		(void) connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived,
					   this, &EngineStatusController::_receiveMessage);
	}
}

void EngineStatusController::_receiveMessage(const LinkInterface* link, const mavlink_message_t &message)
{
	Q_UNUSED(link);

	// Try to identify engine-related messages by name from the mavlink metadata
	const mavlink_message_info_t *msgInfo = mavlink_get_message_info(&message);
	if (!msgInfo) {
		qCDebug(EngineLog) << "EngineStatusController: NULL msgInfo for msgid" << message.msgid;
		return;
	}

	// Pointer to raw payload bytes for direct offset reads
	const uint8_t *msg = reinterpret_cast<const uint8_t*>(&message.payload64[0]);

	const QString msgName = QString::fromLatin1(msgInfo->name).toLower();
	qCDebug(EngineLog) << "EngineStatusController: received msgid" << message.msgid << "name" << msgName << "fields" << msgInfo->num_fields;
	// Special-case: PX4 mavlink stream packs UAVCAN engine status into DEBUG_FLOAT_ARRAY
	if (msgName == QStringLiteral("debug_float_array") || message.msgid == MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY) {
		// Find the name field (char array) and the data float array offset
		const unsigned int numFields = msgInfo->num_fields;
		const char *embeddedName = nullptr;
		unsigned int nameLen = 0;
		unsigned int dataOffset = 0;
		unsigned int dataCount = 0;
		for (unsigned int i = 0; i < numFields; ++i) {
			const auto &f = msgInfo->fields[i];
			if (f.type == MAVLINK_TYPE_CHAR && f.array_length > 0) {
				embeddedName = reinterpret_cast<const char*>(msg + f.wire_offset);
				nameLen = f.array_length;
			}
			if ((f.type == MAVLINK_TYPE_FLOAT) && f.array_length > 0 && QString::fromLatin1(f.name) == QStringLiteral("data")) {
				dataOffset = f.wire_offset;
				dataCount = f.array_length;
			}
		}

		QString eName;
		if (embeddedName) {
			eName = QString::fromLatin1(embeddedName, static_cast<int>(nameLen)).trimmed().toLower();
			qCDebug(EngineLog) << "EngineStatusController: embedded name" << eName;
		}

		if (eName.contains("eng_ten") || eName.contains("eng_hun") || eName.contains("eng")) {
			// Parse float array starting at dataOffset
			if (dataCount == 0) {
				qCDebug(EngineLog) << "EngineStatusController: debug_float_array has no data field info";
			} else {
				// Helper to read float at index
				auto readFloatIndex = [&](unsigned int idx)->double {
					if (idx >= dataCount) return 0.0;
					float v = 0.0f;
					(void) memcpy(&v, msg + dataOffset + idx * sizeof(float), sizeof(float));
					return static_cast<double>(v);
				};

				if (eName.contains("eng_ten")) {
					// Map based on uavcan_engine_status_ten mapping in sending code
					double oilPressure = readFloatIndex(7); // oil_pressure_kpa
					double oilTemp = readFloatIndex(8); // oil_temperature_c
					double voltage_v = readFloatIndex(44); // voltage_v
					double rpmVal = readFloatIndex(47); // rpm

					// Additional fields
					double intake = readFloatIndex(13); // ambient_temp
					// exhaust: take max of cyl_exh_temp_1..4 indices 18-21
					double ex1 = readFloatIndex(18);
					double ex2 = readFloatIndex(19);
					double ex3 = readFloatIndex(20);
					double ex4 = readFloatIndex(21);
					double exhaustMax = std::max(std::max(ex1, ex2), std::max(ex3, ex4));
					double fuelPres = readFloatIndex(6); // fuel_pressure_kpa
					double supplyV = readFloatIndex(16); // supply_voltage_a

					if (!qFuzzyCompare(oilPressure + 1.0, _oilPressure + 1.0)) {
						_oilPressure = oilPressure;
						emit oilPressureChanged();
					}
					if (!qFuzzyCompare(oilTemp + 1.0, _temperature + 1.0)) {
						_temperature = oilTemp;
						emit temperatureChanged();
					}
					if (!qFuzzyCompare(voltage_v + 1.0, _voltage + 1.0)) {
						_voltage = voltage_v;
						emit voltageChanged();
					}
					if (!qFuzzyCompare(intake + 1.0, _intakeTemp + 1.0)) {
						_intakeTemp = intake;
						emit intakeTempChanged();
					}
					if (!qFuzzyCompare(exhaustMax + 1.0, _exhaustTemp + 1.0)) {
						_exhaustTemp = exhaustMax;
						emit exhaustTempChanged();
					}
					if (!qFuzzyCompare(fuelPres + 1.0, _fuelPressure + 1.0)) {
						_fuelPressure = fuelPres;
						emit fuelPressureChanged();
					}
					if (!qFuzzyCompare(supplyV + 1.0, _supplyVoltage + 1.0)) {
						_supplyVoltage = supplyV;
						emit supplyVoltageChanged();
					}
					if (_rpm != static_cast<int>(rpmVal)) {
						_rpm = static_cast<int>(rpmVal);
						emit rpmChanged();
					}

					qCDebug(EngineLog) << "EngineStatusController: ENG_TEN parsed rpm" << _rpm << "oilPressure" << _oilPressure << "temp" << _temperature << "voltage" << _voltage;
				} else if (eName.contains("eng_hun")) {
					// Map based on uavcan_engine_status_hun mapping
					double rpmA = readFloatIndex(8); // engine_speed_a_rpm
					double rpmB = readFloatIndex(9); // engine_speed_b_rpm
					double rpmVal = rpmA != 0.0 ? rpmA : rpmB;
					double oilTemp = readFloatIndex(4); // cyl_coolant_temp_1_c as proxy
					double intake = readFloatIndex(0); // manifold_temp_a_c
					double supplyV = 0.0; // ENG_HUN does not include voltage in this mapping

					if (_rpm != static_cast<int>(rpmVal)) {
						_rpm = static_cast<int>(rpmVal);
						emit rpmChanged();
					}
					if (!qFuzzyCompare(oilTemp + 1.0, _temperature + 1.0)) {
						_temperature = oilTemp;
						emit temperatureChanged();
					}
					if (!qFuzzyCompare(intake + 1.0, _intakeTemp + 1.0)) {
						_intakeTemp = intake;
						emit intakeTempChanged();
					}

					qCDebug(EngineLog) << "EngineStatusController: ENG_HUN parsed rpm" << _rpm << "temp" << _temperature;
				}
			}
		}
		return;
	}

	if (!msgName.contains("engine") && !msgName.contains("uavcan")) {
		// not an engine/uavcan message - but do a payload scan for a fallback to detect uavcan text
		// not an engine/uavcan message - but do a payload scan for a fallback to detect uavcan text
		const uint8_t *raw = reinterpret_cast<const uint8_t*>(&(message.payload64[0]));
		const char *needle = "uavcan";
		const size_t needleLen = 6;
		bool found = false;
		for (unsigned int p = 0; p + needleLen <= MAVLINK_MAX_PAYLOAD_LEN; ++p) {
			bool match = true;
			for (size_t k = 0; k < needleLen; ++k) {
				if (raw[p + k] != static_cast<uint8_t>(needle[k])) { match = false; break; }
			}
			if (match) { found = true; qCDebug(EngineLog) << "EngineStatusController: payload contains 'uavcan' at offset" << p; break; }
		}
	if (found) {
			// Dump a short hex preview to help identify message layout
			QByteArray hexPreview;
			const unsigned int dumpLen = std::min<unsigned int>(32, MAVLINK_MAX_PAYLOAD_LEN);
			for (unsigned int i = 0; i < dumpLen; ++i) {
				char buf[4];
				qsnprintf(buf, sizeof(buf), "%02X", raw[i]);
				hexPreview.append(buf);
				if (i < dumpLen - 1) hexPreview.append(' ');
			}
			qCDebug(EngineLog) << "EngineStatusController: msgid" << message.msgid << "hexPreview:" << hexPreview;
		}
		return;
	}

	// Look for common field names and extract basic numeric values

	for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
		const char *fieldName = msgInfo->fields[i].name;
		const QString field = QString::fromLatin1(fieldName).toLower();
		const unsigned int offset = msgInfo->fields[i].wire_offset;
		const unsigned int array_length = msgInfo->fields[i].array_length;

		Q_UNUSED(array_length);

		switch (msgInfo->fields[i].type) {
		case MAVLINK_TYPE_INT32_T: {
			int32_t v = 0;
			(void) memcpy(&v, msg + offset, sizeof(v));
			qCDebug(EngineLog) << " field" << fieldName << "INT32" << v << "offset" << offset;
			if (field == "rpm") {
				if (_rpm != static_cast<int>(v)) {
					_rpm = static_cast<int>(v);
					emit rpmChanged();
				}
			}
			break;
		}
		case MAVLINK_TYPE_UINT32_T: {
			uint32_t v = 0;
			(void) memcpy(&v, msg + offset, sizeof(v));
			qCDebug(EngineLog) << " field" << fieldName << "UINT32" << v << "offset" << offset;
			if (field == "rpm") {
				if (_rpm != static_cast<int>(v)) {
					_rpm = static_cast<int>(v);
					emit rpmChanged();
				}
			}
			break;
		}
		case MAVLINK_TYPE_INT16_T: {
			int16_t v = 0;
			(void) memcpy(&v, msg + offset, sizeof(v));
			qCDebug(EngineLog) << " field" << fieldName << "INT16" << v << "offset" << offset;
			if (field.contains("temp") || field.contains("temperature") || field.contains("oil_temp")) {
				double dv = static_cast<double>(v);
				if (!qFuzzyCompare(dv + 1.0, _temperature + 1.0)) {
					_temperature = dv;
					emit temperatureChanged();
				}
			}
			break;
		}
		case MAVLINK_TYPE_FLOAT: {
			float fv = 0.0f;
			(void) memcpy(&fv, msg + offset, sizeof(fv));
			qCDebug(EngineLog) << " field" << fieldName << "FLOAT" << fv << "offset" << offset;
			double dv = static_cast<double>(fv);
			if (field.contains("oil_pressure") || field.contains("pressure")) {
				if (!qFuzzyCompare(dv + 1.0, _oilPressure + 1.0)) {
					_oilPressure = dv;
					emit oilPressureChanged();
				}
			} else if (field.contains("temp") || field.contains("temperature")) {
				if (!qFuzzyCompare(dv + 1.0, _temperature + 1.0)) {
					_temperature = dv;
					emit temperatureChanged();
				}
			} else if (field.contains("voltage") || field.contains("supply_voltage") || field.contains("volt")) {
				if (!qFuzzyCompare(dv + 1.0, _voltage + 1.0)) {
					_voltage = dv;
					emit voltageChanged();
				}
			}
			break;
		}
		default:
			break;
		}
	}
}

