#include <string>

#include <llvm/IR/Instruction.h>

#include "RtlWidth.hpp"
#include "RtlOperation.hpp"
#include "RtlSignal.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlSignal::RtlSignal::RtlSignalDriver::RtlSignalDriver(RtlSignal* driver, RtlWidth dest_bits)
    : signal(driver),
      src_bits(driver->getWidth()),
      dest_bits(dest_bits) {}

rtl::RtlSignal::RtlSignal::RtlSignalDriver::RtlSignalDriver(RtlSignal* driver, RtlWidth dest_bits, RtlWidth src_bits)
    : signal(driver),
      src_bits(src_bits),
      dest_bits(dest_bits) {}

rtl::RtlSignal::RtlSignal()
    : default_driver(nullptr) {}

rtl::RtlSignal::RtlSignal(std::optional<std::string> name,
                          std::optional<std::string> value,
                          std::optional<std::string> type,
                          RtlWidth bitwidth)
    : name(name),
      type(type),
      value(value),
      bitwidth(bitwidth),
      default_driver(nullptr) {}

bool rtl::RtlSignal::isRegister() {
    bool is_register =
        type.has_value()
            && (type.value() == "reg" || type.value() == "output reg");

    return is_register;
}

void rtl::RtlSignal::setWidth(RtlWidth width) {
    this->bitwidth = width;
}

rtl::RtlWidth rtl::RtlSignal::getWidth() {
    return bitwidth;
}

void rtl::RtlSignal::setName(std::string name) {
    this->name = name;
}

std::optional<std::string>& rtl::RtlSignal::getName() {
  return name;
}

void rtl::RtlSignal::setType(std::string type) {
    this->type = type;
}

std::optional<std::string>& rtl::RtlSignal::getType() {
    return type;
}

void rtl::RtlSignal::setValue(std::string value) {
    this->value = value;
}

std::optional<std::string>& rtl::RtlSignal::getValue() {
    return value;
}

void rtl::RtlSignal::setDriver(unsigned int i,
                               RtlSignal* driver,
                               Instruction* instr,
                               std::optional<RtlWidth> src_bits,
                               std::optional<RtlWidth> dets_bits)
{
    // TODO Implement method
}

rtl::RtlSignal::RtlSignalDriver* rtl::RtlSignal::getDriver(unsigned int i) {
    return drivers.at(i);
}

void rtl::RtlSignal::setExclDriver(RtlSignal* driver,
                                   Instruction* instr,
                                   std::optional<RtlWidth> src_bits,
                                   std::optional<RtlWidth> dets_bits)
{
    for (auto* driver : drivers) {
        if (driver != nullptr) {
            delete driver;
            driver = nullptr;
        }
    }

    drivers.clear();
    conditions.clear();
    instructions.clear();

    instructions.push_back(instr);

    auto dest_bits_actual = dets_bits.value_or(this->getWidth());

    auto* rtl_driver =
        src_bits.has_value()
            ? new RtlSignalDriver(driver, dest_bits_actual, src_bits.value())
            : new RtlSignalDriver(driver, dest_bits_actual);

    drivers.push_back(rtl_driver);
}

void rtl::RtlSignal::setDefaultDriver(RtlSignal* driver,
                                      std::optional<RtlWidth> src_bits,
                                      std::optional<RtlWidth> dets_bits)
{
    assert(driver != nullptr);
    delete default_driver;

    auto dest_bits_actual = dets_bits.value_or(this->getWidth());

    default_driver =
        src_bits.has_value()
            ? new RtlSignalDriver(driver, dest_bits_actual, src_bits.value())
            : new RtlSignalDriver(driver, dest_bits_actual);
}

rtl::RtlSignal::RtlSignalDriver* rtl::RtlSignal::getDefaultDriver() {
    return default_driver;
}

void rtl::RtlSignal::addCondition(RtlSignal* cond,
                                  RtlSignal* driver,
                                  Instruction* instr,
                                  std::optional<RtlWidth> src_bits,
                                  std::optional<RtlWidth> dets_bits)
{
    conditions.push_back(cond);

    auto dest_bits_actual = dets_bits.value_or(this->getWidth());

    auto* rtl_driver =
        src_bits.has_value()
            ? new RtlSignalDriver(driver, dest_bits_actual, src_bits.value())
            : new RtlSignalDriver(driver, dest_bits_actual);

    drivers.push_back(rtl_driver);
    instructions.push_back(instr);
}

rtl::RtlSignal* rtl::RtlSignal::getCondition(unsigned int i) {
    return conditions.at(i);
}

unsigned int rtl::RtlSignal::getDriversNum() {
    return drivers.size();
}

unsigned int rtl::RtlSignal::getConditionsNum() {
    return conditions.size();
}