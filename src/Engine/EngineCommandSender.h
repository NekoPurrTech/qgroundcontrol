/*
 * Minimal QML-facing wrapper to send MAVLink COMMAND_LONG (and related) from QML.
 */
#pragma once

#include <QObject>

class EngineCommandSender : public QObject
{
    Q_OBJECT

public:
    explicit EngineCommandSender(QObject* parent = nullptr);

    // Send a command to the active vehicle. Parameters correspond to Vehicle::sendCommand
    Q_INVOKABLE void sendEngineCommand(int compId, int command, bool showError = true,
                                       double p1 = 0, double p2 = 0, double p3 = 0,
                                       double p4 = 0, double p5 = 0, double p6 = 0,
                                       double p7 = 0);
};
