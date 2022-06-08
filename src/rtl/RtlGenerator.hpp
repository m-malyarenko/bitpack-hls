#ifndef __RTL_RTL_GENERATOR_HPP__
#define __RTL_RTL_GENERATOR_HPP__

#include <map>
#include <set>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include "llvm/IR/InstVisitor.h"

#include "../scheduling/fsm/Fsm.hpp"
#include "../binding/LifetimeAnalysis.hpp"
#include "../binding/Binding.hpp"
#include "../binding/BitpackRegBinding.hpp"
#include "RtlModule.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlGenerator : public InstVisitor<RtlGenerator> {
public:
    RtlGenerator(llvm::Function& function,
                 Fsm& fsm,
                 binding::LifetimeAnalysis& lva,
                 binding::Binding& binding);

    RtlModule& generate();

/* VISITING FUNCTIONS BEGIN --------------------------------------------------*/

    void visitReturnInst        (ReturnInst &I);
    void visitBranchInst        (BranchInst &I);
    void visitSwitchInst        (SwitchInst &I);
    void visitPHINode           (PHINode &I);

    void visitBinaryOperator    (Instruction &I);
    void visitUnaryOperator     (UnaryInstruction &I);

    void visitICmpInst          (ICmpInst &I);
    void visitCastInst          (CastInst &I);

    void visitAllocaInst        (AllocaInst &I);
    void visitLoadInst          (LoadInst &I);
    void visitStoreInst         (StoreInst &I);

    void visitInvokeInst        (InvokeInst &I);
    void visitUnreachableInst   (UnreachableInst &I);

    // TODO Add unsupported instructions visits to abort

/* VISITING FUNCTIONS END ----------------------------------------------------*/

private:
    Function& function;
    Fsm& fsm;
    binding::LifetimeAnalysis& lva;
    binding::Binding& binding;

    RtlModule module;

    FsmState* visit_state;
    std::set<Instruction*> visited_instr;

    std::map<binding::Binding::FuInstId, std::set<Instruction*>> fu_binding_map;

    std::vector<binding::BitpackRegBinding::Reg> bp_regs;
    std::map<Value*, binding::BitpackRegBinding::RegBitfield> bp_reg_binding_map;

    std::map<FsmState*, RtlSignal*> state_signals;

    RtlConstant* ZERO;
    RtlConstant* ONE;

    void generateDeclaration();

    void setUpFsmControllerStates();

    void addDefaultPorts();

    void addInstructionsSignals();

    void connectRegistersToWires();

    void performBinding();

    void createBindingSignals();

    void generateDatapath();

    void shareRegisters();

    void generateStateTransition(RtlSignal* cond, FsmState* state);

    void driveSignalInState(RtlSignal* signal,
                            RtlSignal* driver,
                            FsmState* state,
                            Instruction* instr = nullptr);

    bool isUsedAcrossStates(Value* val, FsmState* state);

    bool isLiveAcrossStates(Value* val, FsmState* state);

    void shareRegistersForFu(std::set<Instruction*> instructions,
                             std::map<Instruction*, std::set<Instruction*>> independant_instructions);

    RtlSignal* createFu(Instruction* instr, RtlSignal* op_0, RtlSignal* op_1);

    RtlSignal* createBindedFuUse(Instruction* instr, RtlSignal* op_0, RtlSignal* op_1);

    RtlSignal* getOperandSignal(FsmState* state, Value* op);

    RtlSignal* getTransitionOperand(FsmState* state);

    RtlSignal* getInstructionLhsSignal(Instruction* instr);

    RtlOperation* addStateCheckOperation(FsmState* state);

    bool shouldIgnoreInstruction(Instruction& instr);
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_GENERATOR_HPP__ */
