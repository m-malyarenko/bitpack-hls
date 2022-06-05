#ifndef __BINDING_BINDING_HPP__
#define __BINDING_BINDING_HPP__

#include <set>
#include <map>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/ADT/iterator_range.h>

#include "../hardware/HardwareConstraints.hpp"
#include "../scheduling/fsm/Fsm.hpp"
#include "LifetimeAnalysis.hpp"

namespace llvm {
    namespace bphls {
        namespace binding {

class Binding {
public:
    Binding(Fsm& fsm, LifetimeAnalysis& lva)
        : fsm(fsm),
          lva(lva) {}

    typedef std::pair<hardware::FunctionalUnit*, unsigned char> FuInstId;
    typedef std::map<Instruction*, FuInstId> BindingMap;

    void assignInstructions();

    void findIndependantInstructions(
        std::set<Instruction*>& instructions,
        std::map<Instruction*, std::set<Instruction*>>& independant_instructions
    );

    bool exists(Instruction* instr);

    FuInstId& getBindedFu(Instruction* instr);

    typedef BindingMap::iterator BindingIterator;
    iterator_range<BindingIterator> iter_binding() {
        return make_range(instr_fu_map.begin(), instr_fu_map.end());
    }

private:
    Fsm& fsm;
    LifetimeAnalysis& lva;

    typedef std::vector<std::vector<int>> Table;

    struct AssignInfo {
        std::map<FuInstId, int> mux_inputs;
        std::map<Instruction*, std::set<Instruction*>> independant_instructions;
        std::map<FuInstId, std::set<Instruction*>> existing_operands;
        std::map<FuInstId, std::set<Instruction*>> existing_instructions;
    };

    BindingMap instr_fu_map;
    std::map<FuInstId, std::vector<Instruction*>> fu_instr_set_map;

    std::map<unsigned int, Instruction*> state_idx_instr_lookup;

    void bindFuInState(FsmState* state,
                       hardware::FunctionalUnit* fu,
                       unsigned int n_available,
                       AssignInfo& assign_info);

    void createWeights(Instruction* instr,
                       unsigned int instr_idx,
                       hardware::FunctionalUnit* fu,
                       unsigned int n_available,
                       AssignInfo& assign_info,
                       Table& weights);

    void solveBwm(Table& weights, Table& assignment);

    void verifyBwm(unsigned int n_share, unsigned int n_available, Table& assignment);

    void updateAssignment(unsigned int n_share,
                          unsigned int n_available,
                          hardware::FunctionalUnit* fu,
                          AssignInfo& assign_info,
                          Table& assignment);

    void findInstructionsLiveBlocks(
        std::set<Instruction*>& instructions,
        std::map<Instruction*, std::set<BasicBlock*>>& live_blocks
    );

    void findIndependantInstructionsInLiveBlocks(
        std::map<Instruction*, std::set<Instruction*>>& independant_instructions,
        std::map<Instruction*, std::set<BasicBlock*>>& live_blocks
    );

    bool isRegOptimized(Instruction* instr, FsmState* state);

    bool areIndependantInStates(Instruction* instr_a,
                                Instruction* instr_b,
                                std::set<BasicBlock*>& intersection);

    bool checkPhiSuccessors(Instruction* instr,
                            FsmState* state,
                            std::set<Instruction*>& phi_nodes);

    void visitTransitionStates(FsmState* state, std::set<Instruction*>& phi_nodes);

    void createStateEncoding(Function* function,
                             std::map<FsmState*, unsigned int>& state_encoding,
                             std::map<BasicBlock*, unsigned int>& bb_last_state);

    void addInstructionDefState(
        BasicBlock* basic_block,
        Instruction* instr_a,
        std::vector<unsigned int>& instr_a_states,
        Instruction* instr_b,
        std::vector<unsigned int>& instr_b_states,
        std::map<FsmState*, unsigned int>& state_encoding
    );

    void addInstructionUseState(
        BasicBlock* basic_block,
        Instruction* instr_a,
        std::vector<unsigned int>& instr_a_states,
        Instruction* instr_b,
        std::vector<unsigned int>& instr_b_states,
        std::map<FsmState*, unsigned int>& state_encoding,
        unsigned int last_state
    );

    void addInstructionLiveInOut(
        BasicBlock* basic_block,
        Instruction* instr_a,
        std::vector<unsigned int>& instr_a_states,
        Instruction* instr_b,
        std::vector<unsigned int>& instr_b_states,
        std::map<FsmState*, unsigned int>& state_encoding,
        unsigned int first_state,
        unsigned int last_state
    );

    int** tableToRawMatrix(Table& table, unsigned int rows, unsigned int cols);
};

        } /* namespace binding */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __BINDING_BINDING_HPP__ */
