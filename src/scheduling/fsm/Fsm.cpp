#include <list>
#include <algorithm>
#include <string>

#include "../../utility/DotGraph.hpp"
#include "../../utility/transition_utility.hpp"

#include "Fsm.hpp"

using namespace llvm;
using namespace bphls;

FsmState* Fsm::createState(FsmState* after, std::string name) {
    FsmState* state = new FsmState(this);

    if (after != nullptr) {
        auto after_iter = std::find(states().begin(), states().end(), after);

        assert(after_iter != states().end());

        after_iter++;
        state_list.insert(after_iter, state);
    } else {
        state_list.push_back(state);
    }

    state->setName(name);
    return state;
}

void Fsm::setStartState(Instruction* instr, FsmState* state) {
    start_state_lookup[instr] = state;
}

FsmState* Fsm::getStartState(Instruction* instr) {
    return start_state_lookup[instr];
}

void Fsm::setEndState(Instruction* instr, FsmState* state) {
    end_state_lookup[instr] = state;
}

FsmState* Fsm::getEndState(Instruction* instr) {
    return end_state_lookup[instr];
}

void printNodeLabel(raw_ostream& out, FsmState* state) {
    out << state->getName() << "\\n";

    for (auto* instr : state->instructions()) {
        out << instr->getOpcodeName() << "\\n";
    }
}

void Fsm::exportDot(formatted_raw_ostream& out) {
    dotGraph<FsmState> graph(out, printNodeLabel);
    graph.setLabelLimit(100);

    for (auto* state : this->states()) {

        assert(state->getDefaultTransition());
        const unsigned int trans_num = state->getTransitionsNum(); 

        if (trans_num == 1) {
            graph.connectDot(out, state, state->getDefaultTransition(), "");
        } else if (trans_num == 2) {
            graph.connectDot(
                out,
                state,
                state->getTransitionState(0),
                "label=\"" + utility::getTransitionOperands(state) + "\""
            );

            graph.connectDot(
                out,
                state,
                state->getDefaultTransition(),
                "label=\"~" + utility::getTransitionOperands(state) + "\""
            );
        } else {
            assert(trans_num > 0);

            for (unsigned int i = 0; i != (trans_num - 1); i++) {
                std::string label =
                    "label=\"" + utility::getTransitionOperands(state) + " == " +
                    state->getTransitionValue(i)->getName().str() + "\"";

                graph.connectDot(
                    out,
                    state,
                    state->getTransitionState(i),
                    label
                );
            }

            graph.connectDot(out, state, state->getDefaultTransition(), "Default");
        }
    }
}