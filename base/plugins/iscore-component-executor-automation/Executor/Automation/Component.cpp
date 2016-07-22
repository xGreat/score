#include <Editor/Automation.h>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <QDebug>
#include <QString>
#include <algorithm>
#include <vector>

#include "Component.hpp"
#include <Curve/CurveModel.hpp>
#include <Curve/Segment/CurveSegmentData.hpp>
#include <Device/Protocol/DeviceInterface.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <Device/Protocol/DeviceSettings.hpp>
#include "Editor/Curve.h"
#include "Editor/CurveSegment.h"
#include <Editor/Value/Value.h>
#include <Editor/Value/Behavior.h>
#include "Network/Address.h"
#include "Network/Node.h"
#include <OSSIA/Protocols/OSSIADevice.hpp>
#include <OSSIA/Executor/ProcessElement.hpp>
#include <State/Address.hpp>
#include <OSSIA/CurveConversion.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <OSSIA/iscore2OSSIA.hpp>

#include <OSSIA/Executor/ExecutorContext.hpp>
#include <OSSIA/Executor/DocumentPlugin.hpp>
namespace Process { class ProcessModel; }
class QObject;
namespace OSSIA {
class TimeProcess;
}  // namespace OSSIA
namespace RecreateOnPlay {
class ConstraintElement;

}  // namespace RecreateOnPlay



namespace Automation
{
namespace RecreateOnPlay
{
Component::Component(
        ::RecreateOnPlay::ConstraintElement& parentConstraint,
        ::Automation::ProcessModel& element,
        const ::RecreateOnPlay::Context& ctx,
        const Id<iscore::Component>& id,
        QObject *parent):
    ::RecreateOnPlay::ProcessComponent_T<Automation::ProcessModel>{parentConstraint, element, ctx, id, "Executor::Automation::Component", parent},
    m_deviceList{ctx.devices.list()}
{
    recreate();
}

void Component::recreate()
{
    auto addr = process().address();
    std::shared_ptr<OSSIA::Address> address;
    OSSIA::Node* node{};
    Ossia::OSSIADevice* dev{};

    m_ossia_curve.reset(); // It will be remade after.

    // Get the device list to obtain the nodes.
    const auto& devices = m_deviceList.devices();

    // Look for the real node in the device
    auto dev_it = std::find_if(devices.begin(), devices.end(),
                               [&] (Device::DeviceInterface* a_device) {
        return a_device->settings().name == addr.device;
    });

    if(dev_it == devices.end())
        goto curve_cleanup_label;

    dev = dynamic_cast<Ossia::OSSIADevice*>(*dev_it);
    if(!dev)
        goto curve_cleanup_label;

    node = iscore::convert::findNodeFromPath(addr.path, &dev->impl());
    if(!node)
        goto curve_cleanup_label;

    // Add the real address
    address = node->getAddress();
    if(!address)
        goto curve_cleanup_label;

    m_addressType = address->getValueType();


    using namespace OSSIA;
    if(process().tween())
        on_curveChanged(Destination(node->getThis(), {})); // If the type changes we need to rebuild the curve.
    else
        on_curveChanged({});
    if(!m_ossia_curve)
        goto curve_cleanup_label;

    m_ossia_process = OSSIA::Automation::create(
                address,
                Behavior(m_ossia_curve));


    return;

curve_cleanup_label:
    m_ossia_process.reset(); // Cleanup
    return;
}

template<typename Y_T>
std::shared_ptr<OSSIA::CurveAbstract> Component::on_curveChanged_impl(
        const optional<OSSIA::Destination>& d)
{
    using namespace OSSIA;

    const double min = process().min();
    const double max = process().max();

    auto scale_x = [=] (double val) -> double { return val; };
    auto scale_y = [=] (double val) -> Y_T { return val * (max - min) + min; };

    auto segt_data = process().curve().sortedSegments();
    if(segt_data.size() != 0)
    {
        return iscore::convert::curve<double, Y_T>(scale_x, scale_y, segt_data, d);
    }
    else
    {
        return {};
    }
}

std::shared_ptr<OSSIA::CurveAbstract> Component::on_curveChanged(
        const optional<OSSIA::Destination>& d)
{
    m_ossia_curve.reset();
    switch(m_addressType)
    {
        case OSSIA::Type::INT:
            m_ossia_curve = on_curveChanged_impl<int>(d);
            break;
        case OSSIA::Type::FLOAT:
            m_ossia_curve = on_curveChanged_impl<float>(d);
            break;
        default:
            qDebug() << "Unsupported curve type: " << (int)m_addressType;
            ISCORE_TODO;
    }

    return m_ossia_curve;
}
}
}
