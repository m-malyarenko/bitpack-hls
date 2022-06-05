#ifndef __BINDING_LIFETIME_ANALYSIS_HPP__
#define __BINDING_LIFETIME_ANALYSIS_HPP__

#include <map>

#include <llvm/IR/Function.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/iterator_range.h>

namespace llvm {
    namespace bphls {
        namespace binding {

class LifetimeAnalysis {
public:
    LifetimeAnalysis(Function& function)
        : function(function) {}

    ~LifetimeAnalysis();

    struct BasicBlockLifetimeInfo {
        BitVector def;
        BitVector use;
        BitVector in;
        BitVector out;

        BasicBlock* basic_block;

        BasicBlockLifetimeInfo(BasicBlock* basic_block, unsigned int def_count)
            : def(def_count),
              use(def_count),
              in(def_count),
              out(def_count),
              basic_block(basic_block) {}
    };

    void analize();

    unsigned int getBitPosition(Value* val);

    BasicBlockLifetimeInfo* getInfo(BasicBlock* basic_block);

    typedef std::map<BasicBlock*, BasicBlockLifetimeInfo*>::iterator BbInfoIterator;
    iterator_range<BbInfoIterator> iter_bb_info() {
        return make_range(bb_info_lookup.begin(), bb_info_lookup.end());
    }
    
private:
    Function& function;

    std::map<Value*, unsigned int> value_bit_pos_lookup;
    std::map<BasicBlock*, BasicBlockLifetimeInfo*> bb_info_lookup;

    typedef std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector*> FlowMask;

    void markUses(Value* def_value);

    void initFlowMask(FlowMask& flow_mask);

    void initPhiFlowMask(BitVector& basic_block_mask, BasicBlock& basic_block);
};

        } /* namespace binding */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __BINDING_LIFETIME_ANALYSIS_HPP__ */

