#ifndef __BINDING_BITPACK_REGISTER_BINDING_HPP__
#define __BINDING_BITPACK_REGISTER_BINDING_HPP__

#include <map>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include "../scheduling/fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {
        namespace binding {

class BitpackRegBinding {
public:
    BitpackRegBinding(Function& function, Fsm& fsm);

    ~BitpackRegBinding();

    typedef unsigned int RegId;

    /** Binding regiter representation */
    struct Reg {
        RegId id;
        unsigned int width;

        Reg();

        Reg(RegId id, unsigned int width);
    };
    
    /** Binding register bitfield */
    struct RegBitfield {
        Reg reg;
        std::pair<unsigned char /*msb*/, unsigned char /*lsb*/> bitfield;

        RegBitfield();

        RegBitfield(Reg reg);

        RegBitfield(Reg reg, unsigned char msb, unsigned char lsb);

        RegBitfield operator + (RegBitfield& other);
    };

    void bindRegisters();

    std::map<Value*, RegBitfield>& getRegisterMapping();

    std::vector<Reg>& getRegisters();

private:
    Function& function;
    Fsm& fsm;

    /** Variable lifetime interval */
    struct LifetimeInterval {
        unsigned int def;
        unsigned int use;
        unsigned int weight;

        LifetimeInterval();

        LifetimeInterval(unsigned int weight);

        LifetimeInterval(unsigned int def, unsigned int use, unsigned int weight);

        bool overlap(LifetimeInterval& other);
    };
    
    /** Variable lifetime interval chunk with the given bitwidth */
    struct LifetimeWidthPool {
        unsigned int id;
        Value* var;
        LifetimeInterval* lt;

        LifetimeWidthPool(Value* var, LifetimeInterval* lt, unsigned int id);
    };

    unsigned int n_stages = 0;
    unsigned int n_sub_regs = 0;
    unsigned int n_regs = 0;
    typedef unsigned int SubRegId;

    std::map<Value*, LifetimeInterval*> lt_table;
    std::vector<LifetimeWidthPool*> lt_pool_table;
    std::map<Value*, std::vector<LifetimeWidthPool*>> lt_pool_map;
    std::vector<std::pair<Value*, std::set<SubRegId>>> var_sub_regs;
    std::vector<Reg> registers;
    std::vector<RegBitfield> bitfields;

    std::map<LifetimeWidthPool*, SubRegId> sub_binding_map;
    std::map<Value*, RegBitfield> binding_map;

    static bool defAfter(LifetimeWidthPool* first, LifetimeWidthPool* second);

    static bool defBefore(LifetimeWidthPool* first, LifetimeWidthPool* second);

    static bool subBindingPredicate(std::pair<Value*, std::set<SubRegId>>& first,
                                    std::pair<Value*, std::set<SubRegId>>& second);

    void analyzeLifetime();

    void printLifetimeTable();

    unsigned int splitTableIntoEqWidthPools();

    void performLeftEdge();

    void mergeSubRegisters(unsigned int pool_width);

    void mapVariablesToBitfields();
};

        } /* namespace binding */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __BINDING_BITPACK_REGISTER_BINDING_HPP__ */
