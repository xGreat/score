#pragma once

#include <QMap>
#include <QPointer>
#include "source/Process/AbstractScenarioViewModel.hpp"
class ScenarioModel;
class RackModel;
class TemporalConstraintViewModel;
class TemporalScenarioPresenter;

class ConstraintModel;
class ScenarioStateMachine;

class TemporalScenarioLayerModel : public AbstractScenarioLayer
{
        Q_OBJECT
    public:
        using model_type = ScenarioModel;
        using constraint_layer_type = TemporalConstraintViewModel;

        TemporalScenarioLayerModel(const id_type<LayerModel>& id,
                              const QMap<id_type<ConstraintModel>,
                              id_type<ConstraintViewModel>>& constraintIds,
                              ScenarioModel& model,
                              QObject* parent);

        // Copy
        TemporalScenarioLayerModel(const TemporalScenarioLayerModel& source,
                              const id_type<LayerModel>& id,
                              ScenarioModel& model,
                              QObject* parent);

        // Load
        template<typename Impl>
        TemporalScenarioLayerModel(Deserializer<Impl>& vis,
                              ScenarioModel& model,
                              QObject* parent) :
            AbstractScenarioLayer {vis, model, parent}
        {
            vis.writeTo(*this);
        }

        virtual LayerModelPanelProxy* make_panelProxy(QObject* parent) const override;

        virtual ~TemporalScenarioLayerModel() = default;

        virtual void serialize(const VisitorVariant&) const override;

        virtual void makeConstraintViewModel(
                const id_type<ConstraintModel>& constraintModelId,
                const id_type<ConstraintViewModel>& constraintViewModelId) override;

        void addConstraintViewModel(constraint_layer_type* constraint_view_model);

    signals:
        void constraintViewModelCreated(const TemporalConstraintViewModel& constraintViewModelid);

    public slots:
        virtual void on_constraintRemoved(const id_type<ConstraintModel>& constraintId) override;

};
