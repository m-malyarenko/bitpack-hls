#ifndef __RTL_RTL_GENERATOR_HPP__
#define __RTL_RTL_GENERATOR_HPP__

#include <map>
#include <set>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include "llvm/IR/InstVisitor.h"

#include "../scheduling/fsm/Fsm.hpp"
#include "RtlModule.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlGenerator : public InstVisitor<RtlGenerator> {
public:
    RtlGenerator(Function& function, Fsm& fsm);

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

    RtlModule module;

    FsmState* visit_state;
    std::set<Instruction*> visited_instr;
    std::set<Instruction*> binded_instructions;

    std::map<FsmState*, RtlSignal*> state_signals;

    RtlConstant* ZERO;
    RtlConstant* ONE;

    void generateDeclaration();

    void addDefaultPorts();

    void addInstructionsSignals();

    void connectRegistersToWires();

    void generateDatapath();

    void generateStateTransition(RtlSignal* cond, FsmState* state);

    void driveSignalInState(RtlSignal* signal,
                            RtlSignal* driver,
                            FsmState* state,
                            Instruction* instr = nullptr);

    bool isUsedAcrossStates(Value* val, FsmState* state);

    bool isLiveAcrossStates(Value* val, FsmState* state);

    RtlSignal* createFu(Instruction* instr, RtlSignal* op_0, RtlSignal* op_1);

    RtlSignal* getOperandSignal(FsmState* state, Value* op);

    RtlSignal* getInstructionLhsSignal(Instruction* instr);

    RtlOperation* addStateCheckOperation(FsmState* state);

    bool shouldIgnoreInstruction(Instruction& instr);
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_GENERATOR_HPP__ */
