// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ScenarioInspectorWidgetFactoryWrapper.hpp"

#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

#include <Scenario/Inspector/Constraint/ConstraintInspectorFactory.hpp>
#include <Scenario/Inspector/Summary/SummaryInspectorWidget.hpp>
#include <Scenario/Inspector/TimeSync/TimeSyncInspectorWidget.hpp>
#include <Scenario/Inspector/Event/EventInspectorWidget.hpp>
#include <Scenario/Inspector/State/StateInspectorWidget.hpp>

namespace Scenario
{
QWidget*
ScenarioInspectorWidgetFactoryWrapper::make(
    const QList<const QObject*>& sourceElements,
    const iscore::DocumentContext& doc,
    QWidget* parent) const
{
  std::set<const ConstraintModel*> constraints;
  std::set<const TimeSyncModel*> timesyncs;
  std::set<const EventModel*> events;
  std::set<const StateModel*> states;

  if (sourceElements.empty())
    return nullptr;

  auto scenar = dynamic_cast<ScenarioInterface*>(sourceElements[0]->parent());
  auto abstr = safe_cast<const IdentifiedObjectAbstract*>(sourceElements[0]);
  ISCORE_ASSERT(scenar); // because else, matches should have return false

  for (auto elt : sourceElements)
  {
    if (auto st = dynamic_cast<const StateModel*>(elt))
    {
      if (auto ev = scenar->findEvent(st->eventId()))
      {
        auto tn = scenar->findTimeSync(ev->timeSync());
        if (!tn)
          continue;
        states.insert(st);
        events.insert(ev);
        timesyncs.insert(tn);
      }
    }
    else if (auto ev = dynamic_cast<const EventModel*>(elt))
    {
      auto tn = scenar->findTimeSync(ev->timeSync());
      if (!tn)
        continue;
      events.insert(ev);
      timesyncs.insert(tn);
    }
    else if (auto tn = dynamic_cast<const TimeSyncModel*>(elt))
    {
      timesyncs.insert(tn);
    }
    else if (auto cstr = dynamic_cast<const ConstraintModel*>(elt))
    {
      constraints.insert(cstr);
    }
  }

  if (states.size() == 1 && constraints.empty())
      return new StateInspectorWidget{**states.begin(), doc, parent};
  if (events.size() == 1 && constraints.empty())
      return new EventInspectorWidget{**events.begin(), doc, parent};
  if (timesyncs.size() == 1 && constraints.empty())
    return new TimeSyncInspectorWidget{**timesyncs.begin(), doc, parent};

  if (constraints.size() == 1 && timesyncs.empty())
  {
    return ConstraintInspectorFactory{}.make(
        {*constraints.begin()}, doc, parent);
  }

  return new SummaryInspectorWidget{
      abstr, constraints, timesyncs, events, states, doc, parent}; // the default InspectorWidgetBase need
                                       // an only IdentifiedObject : this will
                                       // be "abstr"
}

bool ScenarioInspectorWidgetFactoryWrapper::matches(
    const QList<const QObject*>& objects) const
{
  return std::any_of(objects.begin(), objects.end(), [](const QObject* obj) {
    return dynamic_cast<const StateModel*>(obj)
           || dynamic_cast<const EventModel*>(obj)
           || dynamic_cast<const TimeSyncModel*>(obj)
           || dynamic_cast<const ConstraintModel*>(obj);
  });
}
}
