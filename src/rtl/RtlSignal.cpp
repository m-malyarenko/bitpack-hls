#include <string>

#include <llvm/IR/Instruction.h>

#include "RtlWidth.hpp"
#include "RtlOperation.hpp"
#include "RtlSignal.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlSignal::RtlSignal()
    : driver(nullptr),
      default_driver(nullptr) {}

rtl::RtlSignal::RtlSignal(std::optional<std::string> name,
                          std::optional<std::string> value,
                          std::optional<std::string> type,
                          RtlWidth bitwidth)
    : name(name),
      type(type),
      value(value),
      bitwidth(bitwidth),
      driver(nullptr),
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

void rtl::RtlSignal::setDriver(unsigned int i, RtlSignal* driver, Instruction* instr) {
    // TODO Implement method
}

rtl::RtlSignal* rtl::RtlSignal::getDriver(unsigned int i) {
    return drivers.at(i);
}

void rtl::RtlSignal::setExclDriver(RtlSignal* driver, Instruction* instr) {
    drivers.clear();
    conditions.clear();
    instructions.clear();

    instructions.push_back(instr);
    drivers.push_back(driver);
}

void rtl::RtlSignal::setDefaultDriver(RtlSignal* driver) {
    assert(driver != nullptr);

    default_driver = driver;
}

rtl::RtlSignal* rtl::RtlSignal::getDefaultDriver() {
    return default_driver;
}

void rtl::RtlSignal::addCondition(RtlSignal* cond, RtlSignal* driver, Instruction* instr) {
    conditions.push_back(cond);
    drivers.push_back(driver);
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