/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsion_type_policy_h__
#define jsion_type_policy_h__

#include "TypeOracle.h"

namespace js {
namespace ion {

class MInstruction;
class MDefinition;

// A type policy directs the type analysis phases, which insert conversion,
// boxing, unboxing, and type changes as necessary.
class TypePolicy
{
  public:
    // Analyze the inputs of the instruction and perform one of the following
    // actions for each input:
    //  * Nothing; the input already type-checks.
    //  * If untyped, optionally ask the input to try and specialize its value.
    //  * Replace the operand with a conversion instruction.
    //  * Insert an unconditional deoptimization (no conversion possible).
    virtual bool adjustInputs(MInstruction *def) = 0;
};

class BoxInputsPolicy : public TypePolicy
{
  protected:
    static MDefinition *boxAt(MInstruction *at, MDefinition *operand);

  public:
    virtual bool adjustInputs(MInstruction *def);
};

class ArithPolicy : public BoxInputsPolicy
{
  protected:
    // Specifies three levels of specialization:
    //  - < Value. This input is expected and required.
    //  - == Any. Inputs are probably primitive.
    //  - == None. This op should not be specialized.
    MIRType specialization_;

  public:
    bool adjustInputs(MInstruction *def);
};

class BinaryStringPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class BitwisePolicy : public BoxInputsPolicy
{
  protected:
    // Specifies three levels of specialization:
    //  - < Value. This input is expected and required.
    //  - == Any. Inputs are probably primitive.
    //  - == None. This op should not be specialized.
    MIRType specialization_;

  public:
    bool adjustInputs(MInstruction *def);
};

class TableSwitchPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class ComparePolicy : public BoxInputsPolicy
{
  protected:
    MIRType specialization_;

  public:
    ComparePolicy()
      : specialization_(MIRType_None)
    {
    }

    bool adjustInputs(MInstruction *def);
};

// Policy for MTest instructions.
class TestPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

// Single-string input. If the input is a Value, it is unboxed.
class StringPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

// Expect an Int for operand Op. If the input is a Value, it is unboxed.
template <unsigned Op>
class IntPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

// Expect a double for operand Op. If the input is a Value, it is unboxed.
template <unsigned Op>
class DoublePolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

template <unsigned Op>
class ObjectPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins);
    bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};

// Single-object input. If the input is a Value, it is unboxed. If it is
// a primitive, we use ValueToNonNullObject.
class SingleObjectPolicy : public ObjectPolicy<0>
{ };

template <unsigned Op>
class BoxPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins);
    bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};

// Ignore the input, unless unspecialized, and then use BoxInputsPolicy.
class SimplePolicy : public BoxInputsPolicy
{
    bool specialized_;

  public:
    SimplePolicy()
      : specialized_(true)
    { }

    bool adjustInputs(MInstruction *def);
    bool specialized() const {
        return specialized_;
    }
    void unspecialize() {
        specialized_ = false;
    }
};

// Combine multiple policies.
template <class Lhs, class Rhs>
class MixPolicy
  : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def) {
        return Lhs::staticAdjustInputs(def) && Rhs::staticAdjustInputs(def);
    }
    virtual bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

class CallSetElementPolicy : public SingleObjectPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

// First operand will be boxed to an Value (except for an object)
// Second operand will forcefully be unboxed to an object
class InstanceOfPolicy : public TypePolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class StoreTypedArrayPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

// Accepts integers and doubles. Everything else is boxed.
class ClampPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

static inline bool
CoercesToDouble(MIRType type)
{
    if (type == MIRType_Undefined || type == MIRType_Double)
        return true;
    return false;
}


} // namespace ion
} // namespace js

#endif // jsion_type_policy_h__

