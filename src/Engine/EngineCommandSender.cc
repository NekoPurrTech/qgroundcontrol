#include "EngineCommandSender.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

EngineCommandSender::EngineCommandSender(QObject* parent)
    : QObject(parent)
{
}

void EngineCommandSender::sendEngineCommand(int compId, int command, bool showError,
                                            double p1, double p2, double p3,
                                            double p4, double p5, double p6,
                                            double p7)
{
    Vehicle* v = MultiVehicleManager::instance()->activeVehicle();
    if (!v) {
        return;
    }

    // Vehicle::sendCommand expects integers for command and component id and doubles for params
    v->sendCommand(compId, command, showError, p1, p2, p3, p4, p5, p6, p7);
}
