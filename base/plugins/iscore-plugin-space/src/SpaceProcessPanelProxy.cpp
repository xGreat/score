#include "SpaceProcessPanelProxy.hpp"

SpaceProcessPanelProxy::SpaceProcessPanelProxy(
        const SpaceLayerModel &vm,
        QObject *parent):
    LayerModelPanelProxy{parent}
{
    m_layer = new SpaceProcessProxyLayerModel(id_type<LayerModel>(), vm, this);
}

const SpaceProcessProxyLayerModel& SpaceProcessPanelProxy::layer()
{
    return *m_layer;
}
