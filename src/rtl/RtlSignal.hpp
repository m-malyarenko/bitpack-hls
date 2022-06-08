#ifndef __RTL_RTL_SIGNAL_HPP__
#define __RTL_RTL_SIGNAL_HPP__

#include <optional>
#include <string>
#include <vector>
#include <utility>

#include <llvm/IR/Instruction.h>

#include "RtlWidth.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlSignal {
public:
    struct RtlSignalDriver {
        RtlSignal* signal;
        RtlWidth src_bits;
        RtlWidth dest_bits;

        RtlSignalDriver(RtlSignal* driver, RtlWidth dest_bits);

        RtlSignalDriver(RtlSignal* driver, RtlWidth dest_bits, RtlWidth src_bits);
    };

    RtlSignal();

    RtlSignal(std::optional<std::string> name,
              std::optional<std::string> value,
              std::optional<std::string> type,
              RtlWidth bitwidth = RtlWidth());

    virtual ~RtlSignal() {};

    virtual bool isConstant() const { return false; }

    virtual bool isOperation() const { return false; }

    bool isRegister();

    void setName(std::string name);

    std::optional<std::string>& getName();

    void setType(std::string type);

    std::optional<std::string>& getType();

    void setValue(std::string value);

    std::optional<std::string>& getValue();

    void setWidth(RtlWidth width);

    RtlWidth getWidth();

    void setDriver(unsigned int i,
                   RtlSignal* driver,
                   Instruction* instr = nullptr,
                   std::optional<RtlWidth> src_bits = std::nullopt,
                   std::optional<RtlWidth> dets_bits = std::nullopt);

    void setExclDriver(RtlSignal* driver,
                       Instruction* instr = nullptr,
                       std::optional<RtlWidth> src_bits = std::nullopt,
                       std::optional<RtlWidth> dets_bits = std::nullopt);

    RtlSignalDriver* getDriver(unsigned int i);

    void setDefaultDriver(RtlSignal* driver,
                          std::optional<RtlWidth> src_bits = std::nullopt,
                          std::optional<RtlWidth> dets_bits = std::nullopt);

    RtlSignalDriver* getDefaultDriver();

    void addCondition(RtlSignal* cond,
                      RtlSignal* driver,
                      Instruction* instr = nullptr,
                      std::optional<RtlWidth> src_bits = std::nullopt,
                      std::optional<RtlWidth> dets_bits = std::nullopt);

    RtlSignal* getCondition(unsigned int i);

    unsigned int getDriversNum();

    unsigned int getConditionsNum();

private:
    std::optional<std::string> name;
    std::optional<std::string> type;
    std::optional<std::string> value;

    RtlWidth bitwidth;

    RtlSignalDriver* default_driver;

    std::vector<RtlSignal*> conditions;
    std::vector<RtlSignalDriver*> drivers;
    std::vector<Instruction*> instructions;
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_SIGNAL_HPP__ */
