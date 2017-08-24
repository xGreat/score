#pragma once
#include "ScenarioCreationState.hpp"

#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>

#include <Scenario/Commands/Scenario/Creations/CreateState.hpp>
#include <Scenario/Commands/Scenario/Displacement/MoveNewEvent.hpp>
#include <Scenario/Commands/Scenario/Displacement/MoveNewState.hpp>

#include <Scenario/Commands/Scenario/Creations/CreateConstraint.hpp>

#include <Scenario/Application/ScenarioEditionSettings.hpp>
#include <Scenario/Palette/Transitions/AnythingTransitions.hpp>
#include <Scenario/Palette/Transitions/ConstraintTransitions.hpp>
#include <Scenario/Palette/Transitions/EventTransitions.hpp>
#include <Scenario/Palette/Transitions/NothingTransitions.hpp>
#include <Scenario/Palette/Transitions/StateTransitions.hpp>
#include <Scenario/Palette/Transitions/TimeSyncTransitions.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>

#include <QApplication>
#include <QFinalState>
#include <Scenario/Palette/Tools/ScenarioRollbackStrategy.hpp>

namespace Scenario
{
template <typename Scenario_T, typename ToolPalette_T>
class Creation_FromState final
    : public CreationState<Scenario_T, ToolPalette_T>
{
public:
  Creation_FromState(
      const ToolPalette_T& stateMachine,
      const Scenario_T& scenarioPath,
      const iscore::CommandStackFacade& stack,
      QState* parent)
      : CreationState<Scenario_T, ToolPalette_T>{
            stateMachine, stack, std::move(scenarioPath), parent}
  {
    using namespace Scenario::Command;
    auto finalState = new QFinalState{this};
    QObject::connect(
        finalState, &QState::entered, [&]() { this->clearCreatedIds(); });

    auto mainState = new QState{this};
    {
      auto pressed = new QState{mainState};
      auto released = new QState{mainState};
      auto move_nothing = new StrongQState<MoveOnNothing>{mainState};
      auto move_state = new StrongQState<MoveOnState>{mainState};
      auto move_event = new StrongQState<MoveOnEvent>{mainState};
      auto move_timesync = new StrongQState<MoveOnTimeSync>{mainState};

      // General setup
      mainState->setInitialState(pressed);
      released->addTransition(finalState);

      // Release
      iscore::make_transition<ReleaseOnAnything_Transition>(
          mainState, released);

      // Pressed -> ...
      iscore::make_transition<MoveOnNothing_Transition<Scenario_T>>(
          pressed, move_state, *this);
      iscore::make_transition<MoveOnNothing_Transition<Scenario_T>>(
          pressed, move_nothing, *this);

      /// MoveOnNothing -> ...
      // MoveOnNothing -> MoveOnNothing.
      iscore::make_transition<MoveOnNothing_Transition<Scenario_T>>(
          move_nothing, move_nothing, *this);

      // MoveOnNothing -> MoveOnState.
      this->add_transition(move_nothing, move_state, [&]() {
        this->rollback();
        createToState();
      });

      // MoveOnNothing -> MoveOnEvent.
      this->add_transition(move_nothing, move_event, [&]() {
        this->rollback();
        createToEvent();
      });

      // MoveOnNothing -> MoveOnTimeSync
      this->add_transition(move_nothing, move_timesync, [&]() {
        this->rollback();
        createToTimeSync();
      });

      /// MoveOnState -> ...
      // MoveOnState -> MoveOnNothing
      this->add_transition(move_state, move_nothing, [&]() {
        this->rollback();
        createToNothing();
      });

      // MoveOnState -> MoveOnState
      // We don't do anything, the constraint should not move.

      // MoveOnState -> MoveOnEvent
      this->add_transition(move_state, move_event, [&]() {
        this->rollback();
        createToEvent();
      });

      // MoveOnState -> MoveOnTimeSync
      this->add_transition(move_state, move_timesync, [&]() {
        this->rollback();
        createToTimeSync();
      });

      /// MoveOnEvent -> ...
      // MoveOnEvent -> MoveOnNothing
      this->add_transition(move_event, move_nothing, [&]() {
        this->rollback();
        createToNothing();
      });

      // MoveOnEvent -> MoveOnState
      this->add_transition(move_event, move_state, [&]() {

        if (this->clickedState && this->hoveredState)
        {
          if (this->m_parentSM.model().state(*this->clickedState).eventId()
              != this->m_parentSM.model().state(*this->hoveredState).eventId())
          {
            this->rollback();
            createToState();
          }
        }
      });

      // MoveOnEvent -> MoveOnEvent
      iscore::make_transition<MoveOnEvent_Transition<Scenario_T>>(
          move_event, move_event, *this);

      // MoveOnEvent -> MoveOnTimeSync
      this->add_transition(move_event, move_timesync, [&]() {
        this->rollback();
        createToTimeSync();
      });

      /// MoveOnTimeSync -> ...
      // MoveOnTimeSync -> MoveOnNothing
      this->add_transition(move_timesync, move_nothing, [&]() {
        this->rollback();
        createToNothing();
      });

      // MoveOnTimeSync -> MoveOnState
      this->add_transition(move_timesync, move_state, [&]() {
        this->rollback();
        createToState();
      });

      // MoveOnTimeSync -> MoveOnEvent
      this->add_transition(move_timesync, move_event, [&]() {
        this->rollback();
        createToEvent();
      });

      // MoveOnTimeSync -> MoveOnTimeSync
      iscore::make_transition<MoveOnTimeSync_Transition<Scenario_T>>(
          move_timesync, move_timesync, *this);

      // What happens in each state.
      QObject::connect(pressed, &QState::entered, [&]() {
        this->m_clickedPoint = this->currentPoint;
        createToNothing();
      });

      QObject::connect(move_nothing, &QState::entered, [&]() {
        if (this->createdConstraints.empty() || this->createdEvents.empty())
        {
          this->rollback();
          return;
        }

        if (this->currentPoint.date <= this->m_clickedPoint.date)
        {
          this->currentPoint.date
              = this->m_clickedPoint.date + TimeVal::fromMsecs(10);
          ;
        }

        // Magnetism handling :
        // If we press "sequence"... we're always in sequence.
        //
        // Else, if we're < 0.005, we switch to "sequence"
        // Else, we keep the normal state.
        Scenario::EditionSettings& settings
            = this->m_parentSM.editionSettings();
        bool manual_sequence = qApp->keyboardModifiers() & Qt::ShiftModifier;
        if (!manual_sequence)
        {
          auto sequence = settings.sequence();
          auto magnetism_distance
              = (std::abs(this->currentPoint.y - this->m_clickedPoint.y)
                < 0.02);
          if (!sequence && magnetism_distance)
          {
            settings.setSequence(true);
            this->rollback();
            createToNothing();
            return;
          }
          else if (sequence && !magnetism_distance)
          {
            settings.setSequence(false);
            this->rollback();
            createToNothing();
            return;
          }
        }

        auto sequence = settings.sequence();
        if (sequence)
        {
          if (this->clickedState)
          {
            const auto& st
                = this->m_parentSM.model().state(*this->clickedState);
            this->currentPoint.y = st.heightPercentage();
          }
        }

        this->m_dispatcher.template submitCommand<MoveNewEvent>(
            this->m_scenario,
            this->createdConstraints.last(),
            this->createdEvents.last(),
            this->currentPoint.date,
            this->currentPoint.y,
            sequence);
      });

      QObject::connect(move_event, &QState::entered, [&]() {
        if (this->createdStates.empty())
        {
          this->rollback();
          return;
        }

        if (this->currentPoint.date <= this->m_clickedPoint.date)
        {
          return;
        }

        this->m_dispatcher.template submitCommand<MoveNewState>(
            this->m_scenario,
            this->createdStates.last(),
            this->currentPoint.y);
      });

      QObject::connect(move_timesync, &QState::entered, [&]() {
        if (this->createdStates.empty())
        {
          this->rollback();
          return;
        }

        if (this->currentPoint.date <= this->m_clickedPoint.date)
        {
          return;
        }

        this->m_dispatcher.template submitCommand<MoveNewState>(
            this->m_scenario,
            this->createdStates.last(),
            this->currentPoint.y);
      });

      QObject::connect(
          released, &QState::entered, this, &Creation_FromState::commit);
    }

    auto rollbackState = new QState{this};
    iscore::make_transition<iscore::Cancel_Transition>(
        mainState, rollbackState);
    rollbackState->addTransition(finalState);

    QObject::connect(
        rollbackState, &QState::entered, this, &Creation_FromState::rollback);
    this->setInitialState(mainState);
  }

private:
  template <typename Fun>
  void creationCheck(Fun&& fun)
  {
    const auto& scenar = this->m_parentSM.model();
    if (!this->clickedState)
      return;
    bool sequence = this->m_parentSM.editionSettings().sequence();
    bool new_event = qApp->keyboardModifiers() & Qt::ALT;
    auto& st = scenar.state(*this->clickedState);
    if(new_event && !sequence)
    {
      // Create new event on the timesync
      auto tn = Scenario::parentEvent(st, scenar).timeSync();
      auto cmd = new Scenario::Command::CreateEvent_State{
          this->m_scenario, tn, this->currentPoint.y};
      this->m_dispatcher.submitCommand(cmd);

      this->createdEvents.append(cmd->createdEvent());
      this->createdStates.append(cmd->createdState());
      fun(this->createdStates.first());

    }
    else
    {
      if (!sequence)
      {
        // Create new state on the event
        auto cmd = new Scenario::Command::CreateState{
            this->m_scenario, st.eventId(), this->currentPoint.y};
        this->m_dispatcher.submitCommand(cmd);

        this->createdStates.append(cmd->createdState());
        fun(this->createdStates.first());
      }
      else
      {
        if (!st.nextConstraint()) // TODO & deltaY < deltaX
        {
          this->currentPoint.y = st.heightPercentage();
          fun(*this->clickedState);
        }
        else
        {
          // Create new state on the event
          auto cmd = new Scenario::Command::CreateState{
              this->m_scenario, st.eventId(), this->currentPoint.y};
          this->m_dispatcher.submitCommand(cmd);

          this->createdStates.append(cmd->createdState());
          fun(this->createdStates.first());
          // create a single state on the same event (deltaY > deltaX)
        }
      }
    }
  }

  // Note : clickedEvent is set at startEvent if clicking in the background.
  void createToNothing()
  {
    creationCheck(
        [&](const Id<StateModel>& id) { this->createToNothing_base(id); });
  }

  void createToTimeSync()
  {
    creationCheck(
        [&](const Id<StateModel>& id) { this->createToTimeSync_base(id); });
  }

  void createToEvent()
  {
    if (this->clickedState)
    {
      if (this->hoveredEvent
          == this->m_parentSM.model().state(*this->clickedState).eventId())
      {
        creationCheck([&](const Id<StateModel>& id) {});
      }
      else
      {
        creationCheck(
            [&](const Id<StateModel>& id) { this->createToEvent_base(id); });
      }
    }
  }

  void createToState()
  {
    if (this->hoveredState)
    {
      auto& st = this->m_parentSM.model().states.at(*this->hoveredState);
      if (!st.previousConstraint())
      {
        // No previous constraint -> we create a new constraint and link it to
        // this state
        creationCheck(
            [&](const Id<StateModel>& id) { this->createToState_base(id); });
      }
      else
      {
        // Previous constraint -> we add a new state to the event and link to
        // it.
        this->hoveredEvent = st.eventId();
        creationCheck(
            [&](const Id<StateModel>& id) { this->createToEvent_base(id); });
      }
    }
  }
};
}
