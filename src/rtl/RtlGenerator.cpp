#include <iostream>

#include <string>
#include <map>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include "llvm/IR/InstVisitor.h"
// #include <llvm/IR/InstIterator.h>

#include "../utility/verilog_utility.hpp"
#include "../utility/instruction_utility.hpp"
#include "../scheduling/Scheduler.hpp"
#include "../scheduling/fsm/Fsm.hpp"
#include "RtlGenerator.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlGenerator::RtlGenerator(Function& function, Fsm& fsm)
    : function(function),
      fsm(fsm),
      module(function.getName().str()) {}

rtl::RtlModule& rtl::RtlGenerator::generate() {
    generateDeclaration();

    addInstructionsSignals();

    connectRegistersToWires();

#ifndef NDEBUG
    module.printSignals();
#endif

    // TODO Binding

    generateDatapath();

    return module;
}

void rtl::RtlGenerator::generateDeclaration() {
    ZERO = module.addConstant("0");
    ONE = module.addConstant("1");

    for (auto* state : fsm.states()) {
        state_signals[state] = module.addParam("state_ph", "ph");
    }

    auto* wait_state = *fsm.states().begin();

    module.addReg("cur_state");

    addDefaultPorts();

    /* Function return value - RTL module output register */
    auto* ret_type = function.getReturnType();
    if (!ret_type->isVoidTy()) {
        auto* ret_reg = module.addOutputReg("return_val", RtlWidth(ret_type));
        auto* ret_reg_reset_signal = module.addConstant("0", ret_reg->getWidth());

        driveSignalInState(ret_reg, ret_reg_reset_signal, wait_state);
    }

    /* Function arguments - RTL module input wires */
    for (auto& arg : function.args()) {
        auto arg_name = utility::getVerilogName(&arg);
        module.addInputWire(arg_name, RtlWidth(arg.getType()));
    }

    /* Start execution from wait state on "start" signal */
    wait_state->setTransitionSignal(module.find("start"));

    /* Drive output "finish" signal with ZERO in wait state */
    driveSignalInState(module.find("finish"), ZERO, wait_state);
}

void rtl::RtlGenerator::addDefaultPorts() {
    module.addInputWire("clk");
    module.addInputWire("reset");
    module.addInputWire("start");
    module.addOutputReg("finish");
}

void rtl::RtlGenerator::addInstructionsSignals() {
    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            if (shouldIgnoreInstruction(instr)) {
                continue;
            }

            auto wire = utility::getVerilogName(&instr);
            auto reg = wire + "_reg";

            if (!module.exists(wire)) {
                module.addWire(wire); // FIXME add wire width
            }

            if (!module.exists(reg)) {
                module.addReg(reg); // FIXME add reg width
            }
        }
    }

    // TODO Здесь как раз можно вызвать функцию которая подсоеденит управляющеи сигналы кластеров
}

void rtl::RtlGenerator::connectRegistersToWires() {
    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            if (shouldIgnoreInstruction(instr) || isa<PHINode>(instr)) {
                continue;
            }

            auto wire = utility::getVerilogName(&instr);
            auto reg = wire + "_reg";

            driveSignalInState(
                module.find(reg),
                module.find(wire),
                fsm.getEndState(&instr),
                &instr
            );
        }
    }
}

void rtl::RtlGenerator::generateDatapath() {
    char indent = '\t';

    auto* cur_state = module.find("cur_state");
    assert(cur_state != nullptr);

    for (auto* state : fsm.states()) {
        assert(state_signals.count(state) != 0);

        auto* transition_state = module.addOperation(RtlOperation::Eq);
        transition_state->setOperand(0, cur_state);
        transition_state->setOperand(1, state_signals[state]);

        for (auto* instr : state->instructions()) {
            visit_state = state;

            visit(instr);
            // TODO Update usage estimation
        }

        generateStateTransition(transition_state, state);
    }

    auto* reset_state = module.addOperation(RtlOperation::Eq);
    reset_state->setOperand(0, module.find("reset"));
    reset_state->setOperand(1, ONE);

    cur_state->addCondition(reset_state, module.addConstant("0", cur_state->getWidth()));
}

void rtl::RtlGenerator::generateStateTransition(RtlSignal* condition, FsmState* state) {
    assert(state->getDefaultTransition() != nullptr);

    auto* cur_state = module.find("cur_state");

    /* Unconditional branch */
    if (state->getTransitionsNum() == 1) {
        // TODO PHI Copie line 5608 GenerateRTL.cpp
        cur_state->addCondition(condition, state_signals[state->getDefaultTransition()]);
        return;
    }

    /* Conditional branch */
    if (state->getTransitionsNum() == 2) {
        auto* true_branch = module.addOperation(RtlOperation::Eq);
        true_branch->setOperand(0, nullptr); // FIXME
        true_branch->setOperand(1, ONE);

        auto* true_condition = module.addOperation(RtlOperation::And);
        true_branch->setOperand(0, condition); // FIXME
        true_branch->setOperand(1, true_branch);

        // TODO PHI Copie
        cur_state->addCondition(true_condition, state_signals[state->getTransitionState(0)]);

        auto* false_branch = module.addOperation(RtlOperation::Eq);
        true_branch->setOperand(0, nullptr); // FIXME
        true_branch->setOperand(1, ZERO);

        auto* false_condition = module.addOperation(RtlOperation::And);
        true_branch->setOperand(0, condition); // FIXME
        true_branch->setOperand(1, false_branch);

        // TODO PHI Copie
        cur_state->addCondition(false_condition, state_signals[state->getDefaultTransition()]);
    
        return;
    }

    /* TODO Switch branch */
    llvm_unreachable("Switch branch unsupported");
}

rtl::RtlOperation* rtl::RtlGenerator::addStateCheckOperation(FsmState* state) {
    auto* condition_signal = module.addOperation(RtlOperation::Eq);
    condition_signal->setOperand(0, module.find("cur_state"));
    condition_signal->setOperand(1, state_signals[state]);

    return condition_signal;
}

void rtl::RtlGenerator::driveSignalInState(RtlSignal* signal,
                                           RtlSignal* driver,
                                           FsmState* state,
                                           Instruction* instr)
{
    assert(signal != nullptr);
    assert(driver != nullptr);
    assert(state != nullptr);

    RtlOperation* condition = addStateCheckOperation(state);
    signal->addCondition(condition, driver, instr);
}

bool rtl::RtlGenerator::shouldIgnoreInstruction(Instruction& instr) {
    return
        instr.getType()->isVoidTy()
            || isa<AllocaInst>(instr)
            || utility::isDummyCall(instr)
            || instr.users().empty();
}

bool rtl::RtlGenerator::isUsedAcrossStates(Value* val, FsmState* state) {
    for (auto* user : val->users()) {
        auto* instr = dyn_cast<Instruction>(user);

        if (instr != nullptr) {
            return fsm.getEndState(instr) != state;
        }
    }

    return false;
}

bool rtl::RtlGenerator::isLiveAcrossStates(Value* val, FsmState* state) {
    if (isa<PHINode>(val)) {
        return true;
    }

    auto* instr = dyn_cast<Instruction>(val);

    if (instr == nullptr) {
        return false;
    } else {
        return fsm.getEndState(instr) != state;
    }
}

rtl::RtlSignal* rtl::RtlGenerator::getOperandSignal(FsmState* state, Value* op) {
    if (isa<Constant>(op)) {
        /* Constant value */
        std::string const_val;

        if (isa<ConstantInt>(op)) {
            auto* const_int = dyn_cast<ConstantInt>(op);
            auto const_int_val = const_int->getValue();
            auto const_int_val_str = llvm::toString(const_int_val, 10, true);

            if (const_int->isOne() && (const_int->getBitWidth() == 1)) {
                assert(const_int_val_str[0] == '-');
                const_int_val_str.erase(0, 1);
            } else if (const_int_val.isNegative()) {
                assert(const_int_val_str[0] == '-');
                const_int_val_str.erase(0, 1);
                const_val = "-";
            }

            const_val +=
                std::to_string(const_int->getBitWidth()) + "'d" + const_val;
        } else {
            // TODO Add more constant values
            llvm_unreachable("Constant value unsupported");
        }

        return module.addConstant(const_val, RtlWidth(op->getType()));
    } else {
        /* Non-Constant value */
        auto wire = utility::getVerilogName(op);
        auto reg =  wire + "_reg";

        auto* instr = dyn_cast<Instruction>(op);

        if (instr != nullptr) {
            return module.find(wire);
        }

        RtlSignal* signal = nullptr;
        if (isLiveAcrossStates(op, state)) {
            signal = module.find(reg);
        } else {
            signal = module.find(wire);
        }

        // TODO Тут может быть важное место, где выбирается провод или регистр

        return signal;
    }
}

rtl::RtlSignal* rtl::RtlGenerator::getInstructionLhsSignal(Instruction* instr) {
    RtlWidth width(instr->getType());
    auto wire_name = utility::getVerilogName(instr);
    auto* instr_wire = module.addWire(wire_name, width);

    if (!module.exists(wire_name + "_reg")
            && isUsedAcrossStates(instr, visit_state))
    {
        auto* instr_reg = module.addReg(wire_name + "_reg", width);

        driveSignalInState(
            instr_reg,
            instr_wire,
            visit_state,
            instr
        );
    }

    return instr_wire;
}

rtl::RtlSignal* rtl::RtlGenerator::createFu(Instruction* instr, RtlSignal* op_0, RtlSignal* op_1) {
    // FIXME Only add operation (or smthng)

    auto* fu_operation = module.addOperation(*instr);
    fu_operation->setOperand(0, op_0);
    fu_operation->setOperand(1, op_1);

    RtlWidth out_width(instr);
    fu_operation->setWidth(out_width);

    RtlSignal* fu_output = fu_operation;

    unsigned int latency = Scheduler::getInstructionCycles(*instr);
    if (latency > 0) {
        auto enable_wire_name = utility::getVerilogName(instr) + "_en";
        auto* enable_wire = module.addWire(enable_wire_name);

        auto* enable_condition = module.addOperation(RtlOperation::Eq);
        enable_condition->setOperand(0, enable_wire);
        enable_condition->setOperand(1, ONE);

        /* Delay shift register */
        auto* prev_stage_reg = fu_output;
        for (unsigned int i = 0; i < latency; i++) {
            auto stage_reg_name =
                utility::getVerilogName(instr) + "_stage" + std::to_string(i) + "_reg";

            auto* stage_reg = module.addReg(stage_reg_name, out_width);
            auto* driver = prev_stage_reg;

            stage_reg->addCondition(enable_condition, driver, instr);
            prev_stage_reg = stage_reg;
        }

        fu_output = prev_stage_reg;
    }

    assert(fu_output != nullptr);
    return fu_output;
}

/* VISITING FUNCTIONS BEGIN --------------------------------------------------*/

void rtl::RtlGenerator::visitReturnInst(ReturnInst &I) {
    std::cout << "Visit ret instr" << std::endl;
    if (I.getNumOperands() != 0) {
        /* Non-void return */
        auto* ret_signal = module.find("return_val");

        driveSignalInState(
            ret_signal,
            getOperandSignal(visit_state, I.getOperand(0)),
            visit_state,
            &I
        );
    }

    driveSignalInState(
        module.find("finish"),
        ONE,
        visit_state,
        &I
    );
}

void rtl::RtlGenerator::visitBranchInst(BranchInst &I) {}

void rtl::RtlGenerator::visitSwitchInst(SwitchInst &I) {}

void rtl::RtlGenerator::visitPHINode(PHINode &I) {}

void rtl::RtlGenerator::visitBinaryOperator(Instruction &I) {
    std::cout << "Visit binary operator " << I.getOpcodeName() << std::endl;
    auto* instr = &I;

    // Check if operation was already binded
    if (binded_instructions.count(instr) != 0) {
        return;
    }

    auto* instr_wire = getInstructionLhsSignal(instr);
    auto* op_0 = getOperandSignal(visit_state, instr->getOperand(0));
    auto* op_1 = getOperandSignal(visit_state, instr->getOperand(1));

    auto* fu = createFu(instr, op_0, op_1);

    // FIXME Что то хмурое с переподключением сигнала готовнисти FU: create_fu_enable_signals
    unsigned int latency = Scheduler::getInstructionCycles(*instr);
    if (latency > 0) {
        instr_wire->setExclDriver(fu);
        instr_wire->setWidth(fu->getWidth());

        auto* instr_reg = module.find(utility::getVerilogName(instr) + "_reg");
        driveSignalInState(
            instr_reg,
            instr_wire,
            fsm.getEndState(instr),
            instr
        );
    } else {
        driveSignalInState(
            instr_wire,
            fu,
            visit_state,
            instr
        );
    }
}

void rtl::RtlGenerator::visitUnaryOperator(UnaryInstruction &I) {
    std::cout << "Visit unary operator " << I.getOpcodeName() << std::endl;

    auto* instr = &I;

    // Check if operation was already binded
    if (binded_instructions.count(instr) != 0) {
        return;
    }

    auto* instr_wire = getInstructionLhsSignal(instr);
    auto* op_0 = getOperandSignal(visit_state, instr->getOperand(0));

    auto* fu = createFu(instr, op_0, nullptr); // TODO избавиться от нулей через optional

    // FIXME Что то хмурое с переподключением сигнала готовнисти FU: create_fu_enable_signals
    unsigned int latency = Scheduler::getInstructionCycles(*instr);
    assert(latency > 0);

    instr_wire->setExclDriver(fu);
    instr_wire->setWidth(fu->getWidth());

    auto* instr_reg = module.find(utility::getVerilogName(instr) + "_reg");
    driveSignalInState(
        instr_reg,
        instr_wire,
        fsm.getEndState(instr),
        instr
    );
}

void rtl::RtlGenerator::visitICmpInst(ICmpInst &I) {
    std::cout << "Visit cmp instr " << I.getOpcodeName() << std::endl;
    visitBinaryOperator(I);
}

void rtl::RtlGenerator::visitCastInst(CastInst &I) {
    std::cout << "Visit cast instr " << I.getOpcodeName() << std::endl;

    auto* instr = &I;
    auto* instr_wire = getInstructionLhsSignal(instr);
    auto* op_0 = getOperandSignal(visit_state, instr->getOperand(0));

    if (isa<SExtInst>(instr) || isa<ZExtInst>(instr)) {
        auto* ext_operation =
            isa<SExtInst>(instr)
                ? module.addOperation(RtlOperation::SExt)
                : module.addOperation(RtlOperation::ZExt);

        ext_operation->setCastWidth(instr_wire->getWidth());
        ext_operation->setOperand(0, op_0);

        driveSignalInState(
            instr_wire,
            ext_operation,
            visit_state,
            instr
        );
    } else if (isa<BitCastInst>(instr)
                    || isa<PtrToIntInst>(instr)
                    || isa<IntToPtrInst>(instr))
    {
        driveSignalInState(
            instr_wire,
            op_0,
            visit_state,
            instr
        );
    } else {
        llvm_unreachable("Unsupported cast instruction");
    }
}

void rtl::RtlGenerator::visitAllocaInst(AllocaInst &I) {}

void rtl::RtlGenerator::visitLoadInst(LoadInst &I) {
    std::cout << "Visit load instr " << std::endl;

    // auto* instr = &I;
    // auto* addr = instr->getPointerOperand();

    // auto* end_state = fsm.getEndState(instr);
    // assert(end_state != nullptr);

    // RtlWidth width(instr);

    // auto load_wire_name = utility::getVerilogName(instr);
    // auto* load_wire = module.addWire(load_wire_name, width);
    // auto load_reg_name = load_wire_name + "_reg";

    // if (!module.exists(load_reg_name)) {
    //     auto* load_reg = module.addReg(load_reg_name, width);

    //     driveSignalInState(
    //         load_reg,
    //         load_wire,
    //         end_state,
    //         instr
    //     );
    // }
    // TODO Implement method
}

void rtl::RtlGenerator::visitStoreInst(StoreInst &I) {
    std::cout << "Visit store instr " << std::endl;
    // TODO Implement method
}

void rtl::RtlGenerator::visitInvokeInst(InvokeInst &I) {}

void rtl::RtlGenerator::visitUnreachableInst(UnreachableInst &I) {}

/* VISITING FUNCTIONS END ----------------------------------------------------*/
