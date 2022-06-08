#include <iostream>
#include <cmath>

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
#include "../binding/Binding.hpp"
#include "../binding/LifetimeAnalysis.hpp"
#include "../binding/BitpackRegBinding.hpp"
#include "RtlGenerator.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlGenerator::RtlGenerator(llvm::Function& function,
                                Fsm& fsm,
                                binding::LifetimeAnalysis& lva,
                                binding::Binding& binding)
    : function(function),
      fsm(fsm),
      lva(lva),
      binding(binding),
      module(function.getName().str()) {}

rtl::RtlModule& rtl::RtlGenerator::generate() {
    generateDeclaration();

    performBitpackRegBinding();

    addInstructionsSignals();

    connectRegistersToWires();

    performBinding();

    createBindingSignals();

    generateDatapath();

    // shareOperationRegisters();

// #ifndef NDEBUG
//     module.printSignals();
// #endif

    return module;
}

void rtl::RtlGenerator::generateDeclaration() {
    ZERO = module.addConstant("0");
    ONE = module.addConstant("1");

    auto* wait_state = *fsm.states().begin();

    addDefaultPorts();

    setUpFsmControllerStates();

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

void rtl::RtlGenerator::setUpFsmControllerStates() {
    unsigned int n_states = fsm.getStatesNum();
    unsigned int state_width =
        static_cast<unsigned int>(std::ceil(std::log2(n_states)));

    module.addReg("cur_state", RtlWidth(state_width));

    unsigned int state_count = 0;
    for (auto* state : fsm.states()) {
        auto* state_param = module.addParam(state->getName(), std::to_string(state_count));
        state_param->setWidth(RtlWidth(state_width));

        state_signals[state] = state_param;
        state_count++;
    }
}

void rtl::RtlGenerator::addDefaultPorts() {
    module.addInputWire("clk");
    module.addInputWire("reset");
    module.addInputWire("start");
    module.addOutputReg("finish");
}

void rtl::RtlGenerator::performBitpackRegBinding() {
    binding::BitpackRegBinding bp_binding(function, fsm);

    bp_binding.bindRegisters();

    bp_regs.assign(
        bp_binding.getRegisters().begin(),
        bp_binding.getRegisters().end()
    );

    bp_reg_binding_map.insert(
        bp_binding.getRegisterMapping().begin(),
        bp_binding.getRegisterMapping().end()
    );
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
                module.addWire(wire, RtlWidth(&instr));
            }

             // FIXME add wire width
             // FIXME add reg width
             // Почему то до сих пор не убрал. А почему ммммм?
            if (bp_reg_binding_map.count(&instr) != 0) {
                auto& sub_reg = bp_reg_binding_map[&instr]; 
                auto& bp_reg = sub_reg.reg;

                auto bp_reg_name = utility::getBpRegVarilogName(bp_reg);

                if (!module.exists(bp_reg_name)) {
                    module.addReg(bp_reg_name, RtlWidth(bp_reg.width));
                }

                if (!module.exists(reg)) {
                    /* Bitpack Register field proxy */
                    module.addWire(reg, RtlWidth(&instr));
                }
            } else {
                if (!module.exists(reg)) {
                    /* Normal register */
                    module.addReg(reg, RtlWidth(&instr));
                }
            }
        }
    }
}

void rtl::RtlGenerator::connectRegistersToWires() {
    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            if (shouldIgnoreInstruction(instr) || isa<PHINode>(instr)) {
                continue;
            }

            auto wire = utility::getVerilogName(&instr);
            auto reg = wire + "_reg";

            auto* wire_signal = module.find(wire);
            auto* reg_signal = module.find(reg);

            if (bp_reg_binding_map.count(&instr) != 0) {
                auto& sub_reg = bp_reg_binding_map[&instr]; 
                auto& reg = sub_reg.reg;

                auto bp_reg = utility::getBpRegVarilogName(reg);
                auto* bp_reg_signal = module.find(bp_reg);

                assert(bp_reg_signal != nullptr);

                RtlWidth bitfield_width(sub_reg.bitfield.first, sub_reg.bitfield.second, false);

                driveSignalInState(
                    bp_reg_signal,
                    wire_signal,
                    fsm.getEndState(&instr),
                    &instr,
                    wire_signal->getWidth(),
                    bitfield_width
                );

                reg_signal->setExclDriver(bp_reg_signal, &instr, bitfield_width);
            } else {
                driveSignalInState(
                    module.find(reg),
                    module.find(wire),
                    fsm.getEndState(&instr),
                    &instr
                );
            }
        }
    }
}

void rtl::RtlGenerator::performBinding() {
    binding.assignInstructions();
    
    for (auto& bind_map : binding.iter_binding()) {
        fu_binding_map[bind_map.second].insert(bind_map.first); 
    }
}

void rtl::RtlGenerator::createBindingSignals() {
    for (auto& instr_fu : binding.iter_binding()) {
        auto* instr = instr_fu.first;
        auto& fu_inst = instr_fu.second;

        auto fu_signal = utility::getFuInstVerilogName(instr, fu_inst.second);

        if (module.exists(fu_signal)) {
            continue;
        }

        RtlSignal* fu_operation = nullptr;

        auto* op0 =
            module.addWire(fu_signal + "_op0", RtlWidth(instr->getOperand(0)));
        
        if (instr->isBinaryOp()) {
            auto* op1 =
                module.addWire(fu_signal + "_op1", RtlWidth(instr->getOperand(1)));
            fu_operation = createFu(instr, op0, op1);
        } else {
            fu_operation = createFu(instr, op0, nullptr);
        }

        auto* fu = module.addWire(fu_signal, fu_operation->getWidth());
        fu->setExclDriver(fu_operation);
    }

    /* Create bitpack registers */
    for (auto& reg : bp_regs) {
        module.addReg(utility::getBpRegVarilogName(reg), RtlWidth(reg.width));
    }
}

void rtl::RtlGenerator::generateDatapath() {
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

void rtl::RtlGenerator::shareOperationRegisters() {
    for (auto& fu_bind_set : fu_binding_map) {
        auto& fu_inst = fu_bind_set.first;
        auto& instructions = fu_bind_set.second;

        std::map<Instruction*, std::set<Instruction*>> independant_instructions;

        binding.findIndependantInstructions(
            instructions,
            independant_instructions
        );

        shareRegistersForFu(instructions, independant_instructions);
    }
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
        true_branch->setOperand(0, getTransitionOperand(state));
        true_branch->setOperand(1, ONE);

        auto* true_condition = module.addOperation(RtlOperation::And);
        true_condition->setOperand(0, condition);
        true_condition->setOperand(1, true_branch);

        // TODO PHI Copie
        cur_state->addCondition(true_condition, state_signals[state->getTransitionState(0)]);

        auto* false_branch = module.addOperation(RtlOperation::Eq);
        false_branch->setOperand(0, getTransitionOperand(state));
        false_branch->setOperand(1, ZERO);

        auto* false_condition = module.addOperation(RtlOperation::And);
        false_condition->setOperand(0, condition);
        false_condition->setOperand(1, false_branch);

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
                                           Instruction* instr,
                                           std::optional<RtlWidth> src_bits,
                                           std::optional<RtlWidth> dets_bits)
{
    assert(signal != nullptr);
    assert(driver != nullptr);
    assert(state != nullptr);

    RtlOperation* condition = addStateCheckOperation(state);
    signal->addCondition(condition, driver, instr, src_bits, dets_bits);
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

void rtl::RtlGenerator::shareRegistersForFu(std::set<Instruction*> instructions,
                                            std::map<Instruction*, std::set<Instruction*>> independant_instructions)
{
	std::map<Instruction*, std::set<Instruction*>> shared_reg_instr_map;

	// loop over every instruction assigned to this functional unit
	for (auto* instr : instructions) {
		// if it is a store, the verilogName(*inst) couldn't get its reg name
		if (isa<StoreInst>(instr) || isa<LoadInst>(instr)) {
			continue;
		}

		bool is_sharable = false;
		for (auto shared_reg_instr_set : shared_reg_instr_map) {
            auto* shared_reg_instr = shared_reg_instr_set.first;
			auto& assigned_instr_set = shared_reg_instr_set.second;

			assert(shared_reg_instr != instr);

			bool independent = true;
			for (auto* assigned_instr : assigned_instr_set) {
				if (independant_instructions[assigned_instr].count(instr) == 0) {
					independent = false;
				}
			}

			if (independent) {
				is_sharable = true;

				auto* shared_reg = module.find(utility::getVerilogName(shared_reg_instr) + "_reg");

                auto* old_reg = module.find(utility::getVerilogName(instr) + "_reg");

				//determine whether new register has a signed value
                bool is_new_signed = shared_reg->getWidth().isSigned();
                bool is_old_signed = old_reg->getWidth().isSigned();

				bool is_signed = is_new_signed || is_old_signed;

				unsigned int new_width = shared_reg->getWidth().getBitwidth();
				unsigned int old_width = old_reg->getWidth().getBitwidth();

				if (is_signed) {
					if (!is_new_signed) {
                        new_width += 1;
                    } else if (!is_old_signed) {
                        old_width += 1;
				    }
                }

				if (new_width < old_width) {
                    new_width = old_width;
                }
				
                unsigned int reg_instr_size = shared_reg_instr->getType()->getPrimitiveSizeInBits();
				new_width = std::min(new_width, reg_instr_size);

				shared_reg->setWidth(RtlWidth(new_width, is_signed));

                // now make sure the shared register is active at the
                // correct times
                auto* state = fsm.getEndState(instr);
                driveSignalInState(
                    shared_reg,
                    module.find(utility::getVerilogName(instr)),
                    state,
                    instr
                );

                // convert the old register into a wire and drive it by the
                // shared register
                old_reg->setType("wire");
				old_reg->setExclDriver(shared_reg, instr);
				shared_reg_instr_map[shared_reg_instr].insert(instr);
				break;
			}
		}

		if (!is_sharable) {
			shared_reg_instr_map[instr].insert(instr);
		}
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

            const_val += const_int_val_str;
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
            return module.find(reg); // FIXME Пока что всегда привязывается регистр
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

rtl::RtlSignal* rtl::RtlGenerator::getTransitionOperand(FsmState* state) {
    auto* operation = state->getTransitionSignal();

    if (operation != nullptr) {
        return operation;
    } else {
        return getOperandSignal(state, state->getTransitionVariable());
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

rtl::RtlSignal* rtl::RtlGenerator::createBindedFuUse(Instruction* instr, RtlSignal* op_0, RtlSignal* op_1) {
    auto fu_inst = binding.getBindedFu(instr);
    auto fu_signal = utility::getFuInstVerilogName(instr, fu_inst.second);

    driveSignalInState(
        module.find(fu_signal + "_op0"),
        op_0,
        visit_state,
        instr
    );

    if (instr->isBinaryOp()) {
        driveSignalInState(
            module.find(fu_signal + "_op1"),
            op_1,
            visit_state,
            instr
        );
    }

    auto* fu = module.find(fu_signal);
    return fu;
}

/* VISITING FUNCTIONS BEGIN --------------------------------------------------*/

void rtl::RtlGenerator::visitReturnInst(ReturnInst &I) {
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
    auto* instr = &I;

    // if (binded_instructions.count(instr) != 0) {
    //     return;
    // }

    auto* instr_wire = getInstructionLhsSignal(instr);
    auto* op_0 = getOperandSignal(visit_state, instr->getOperand(0));
    auto* op_1 = getOperandSignal(visit_state, instr->getOperand(1));

    // std::cout << "Instr wire: " << instr_wire->getName().value_or("NONAME") << std::endl;
    // std::cout << "Op 0: " << op_0->getName().value_or("NONAME") << std::endl;
    // std::cout << "Op 1: " << op_1->getName().value_or("NONAME") << std::endl;

    RtlSignal* fu = nullptr;
    
    if (binding.exists(instr)) {
        fu = createBindedFuUse(instr, op_0, op_1);
    } else {
        fu = createFu(instr, op_0, op_1);
    }

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
    auto* instr = &I;

    // if (binded_instructions.count(instr) != 0) {
    //     return;
    // }

    auto* instr_wire = getInstructionLhsSignal(instr);
    auto* op_0 = getOperandSignal(visit_state, instr->getOperand(0));

    RtlSignal* fu = nullptr;
    if (binding.exists(instr)) {
        fu = createBindedFuUse(instr, op_0, nullptr); // TODO избавиться от нулей через optional
    } else {
        fu = createFu(instr, op_0, nullptr);
    }

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
    visitBinaryOperator(I);
}

void rtl::RtlGenerator::visitCastInst(CastInst &I) {
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
    // TODO Implement method
}

void rtl::RtlGenerator::visitInvokeInst(InvokeInst &I) {}

void rtl::RtlGenerator::visitUnreachableInst(UnreachableInst &I) {}

/* VISITING FUNCTIONS END ----------------------------------------------------*/
