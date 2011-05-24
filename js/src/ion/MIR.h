/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=79:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Anderson <danderson@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsion_mir_h__
#define jsion_mir_h__

#include "jscntxt.h"
#include "TempAllocPolicy.h"
#include "InlineList.h"

namespace js {
namespace ion {

#define MIR_OPCODE_LIST(_)                                                  \
    _(Constant)                                                             \
    _(Parameter)                                                            \
    _(Goto)                                                                 \
    _(Test)                                                                 \
    _(Phi)                                                                  \
    _(BitAnd)                                                               \
    _(Return)                                                               \
    _(Copy)                                                                 \
    _(Box)                                                                  \
    _(Convert)

enum MIRType
{
    MIRType_Undefined,
    MIRType_Null,
    MIRType_Boolean,
    MIRType_Int32,
    MIRType_String,
    MIRType_Object,
    MIRType_Double,
    MIRType_Value,
    MIRType_Any,        // Any type.
    MIRType_None        // Invalid, used as a placeholder.
};

static const inline
MIRType MIRTypeFromValue(const js::Value &vp)
{
    if (vp.isDouble())
        return MIRType_Double;
    switch (vp.extractNonDoubleType()) {
      case JSVAL_TYPE_INT32:
        return MIRType_Int32;
      case JSVAL_TYPE_UNDEFINED:
        return MIRType_Undefined;
      case JSVAL_TYPE_STRING:
        return MIRType_String;
      case JSVAL_TYPE_BOOLEAN:
        return MIRType_Boolean;
      case JSVAL_TYPE_NULL:
        return MIRType_Null;
      case JSVAL_TYPE_OBJECT:
        return MIRType_Object;
      default:
        JS_NOT_REACHED("unexpected jsval type");
        return MIRType_None;
    }
}

// Forward declarations of MIR types.
#define FORWARD_DECLARE(op) class M##op;
 MIR_OPCODE_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class MInstruction;
class MBasicBlock;
class MUse;
class MIRGraph;

class MIRGenerator
{
  public:
    MIRGenerator(JSContext *cx, TempAllocator &temp, JSScript *script, JSFunction *fun,
                 MIRGraph &graph);

    TempAllocator &temp() {
        return temp_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    uint32 nslots() const {
        return nslots_;
    }
    uint32 nargs() const {
        return fun()->nargs;
    }
    uint32 nlocals() const {
        return script->nfixed;
    }
    uint32 calleeSlot() const {
        JS_ASSERT(fun());
        return 0;
    }
    uint32 thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32 firstArgSlot() const {
        JS_ASSERT(fun());
        return 2;
    }
    uint32 argSlot(uint32 i) const {
        return firstArgSlot() + i;
    }
    uint32 firstLocalSlot() const {
        return (fun() ? fun()->nargs + 2 : 0);
    }
    uint32 localSlot(uint32 i) const {
        return firstLocalSlot() + i;
    }
    uint32 firstStackSlot() const {
        return firstLocalSlot() + nlocals();
    }
    uint32 stackSlot(uint32 i) const {
        return firstStackSlot() + i;
    }
    MIRGraph &graph() {
        return graph_;
    }

    template <typename T>
    T * allocate(size_t count = 1)
    {
        return reinterpret_cast<T *>(temp().allocate(sizeof(T) * count));
    }

  public:
    JSContext *cx;
    JSScript *script;

  protected:
    jsbytecode *pc;
    TempAllocator &temp_;
    JSFunction *fun_;
    uint32 nslots_;
    MIRGraph &graph_;
};

// Represents a use of a definition.
class MUse : public TempObject
{
    friend class MInstruction;

    MUse *next_;            // Next use in the use chain.
    MInstruction *ins_;     // The instruction that is using this operand.
    uint32 index_;          // The index of this operand in its owner.

    MUse(MUse *next, MInstruction *owner, uint32 index)
      : next_(next), ins_(owner), index_(index)
    { }

    void setNext(MUse *next) {
        next_ = next;
    }

  public:
    static inline MUse *New(MIRGenerator *gen, MUse *next, MInstruction *owner, uint32 index)
    {
        return new (gen->temp()) MUse(next, owner, index);
    }

    MInstruction *ins() const {
        return ins_;
    }
    uint32 index() const {
        return index_;
    }
    MUse *next() const {
        return next_;
    }
};

class MUseIterator
{
    MInstruction *def;
    MUse *use;
    MUse *prev_;

  public:
    inline MUseIterator(MInstruction *def);

    bool more() const {
        return !!use;
    }
    void next() {
        prev_ = use;
        use = use->next();
    }
    MUse * operator ->() const {
        return use;
    }
    MUse * operator *() const {
        return use;
    }
    MUse *prev() const {
        return prev_;
    }

    // Unlink the current entry from the use chain, advancing to the next
    // instruction at the same time. The old use node is returned.
    inline MUse *unlink();
};

class MOperand : public TempObject
{
    friend class MInstruction;

    MInstruction *ins_;

    MOperand(MInstruction *ins)
      : ins_(ins)
    { }

    void setInstruction(MInstruction *ins) {
        ins_ = ins;
    }

  public:
    static MOperand *New(MIRGenerator *gen, MInstruction *ins)
    {
        return new (gen->temp()) MOperand(ins);
    }

    MInstruction *ins() const {
        return ins_;
    }
};

class MInstructionVisitor
{
  public:
#define VISIT_INS(op) virtual bool visit(M##op *) { JS_NOT_REACHED("implement " #op); return false; }
    MIR_OPCODE_LIST(VISIT_INS)
#undef VISIT_INS
};

// An MInstruction is an SSA name and definition. It has an opcode, a list of
// operands, and a list of its uses by subsequent instructions.
class MInstruction
  : public TempObject,
    public InlineListNode<MInstruction>
{
    friend class MBasicBlock;

  public:
    enum Opcode {
#   define DEFINE_OPCODES(op) Op_##op,
        MIR_OPCODE_LIST(DEFINE_OPCODES)
#   undef DEFINE_OPCODES
        Op_Invalid
    };

  private:
    MBasicBlock *block_;    // Containing basic block.
    MUse *uses_;            // Use chain.
    uint32 id_;             // Ordered id for register allocation.
    MIRType assumedType_;   // Assumed type from MIR generation.
    MIRType resultType_;    // Actual result type.
    uint32 usedTypes_;      // Set of used types.
    bool inWorklist_;       // Flag for worklist algorithms.

  private:
    void setBlock(MBasicBlock *block) {
        block_ = block;
    }

  public:
    MInstruction()
      : block_(NULL),
        uses_(NULL),
        id_(0),
        assumedType_(MIRType_None),
        resultType_(MIRType_None),
        usedTypes_(0),
        inWorklist_(0)
    { }

    virtual Opcode op() const = 0;
    void printName(FILE *fp);
    virtual void printOpcode(FILE *fp);

    uint32 id() const {
        JS_ASSERT(block_);
        return id_;
    }
    void setId(uint32 id) {
        id_ = id;
    }
    bool inWorklist() const {
        return inWorklist_;
    }
    void setInWorklist() {
        JS_ASSERT(!inWorklist());
        inWorklist_ = true;
    }
    void setNotInWorklist() {
        JS_ASSERT(inWorklist());
        inWorklist_ = false;
    }
    void assumeType(MIRType type) {
        assumedType_ = type;
    }

    MBasicBlock *block() const {
        JS_ASSERT(block_);
        return block_;
    }
    MIRType assumedType() const {
        return assumedType_;
    }
    MIRType type() const {
        return resultType_;
    }

    // Returns a normalized type this instruction should be used as. If exactly
    // one type was requested, then that type is returned. If more than one
    // type was requested, then Value is returned.
    MIRType usedAsType() const;

    // Returns this instruction's use chain.
    MUse *uses() const {
        return uses_;
    }

    // Removes a use, returning the next use in the use list.
    MUse *removeUse(MUse *prev, MUse *use);

    // Number of uses of this instruction.
    size_t useCount() const;

    // Returns the instruction at a given operand.
    virtual MOperand *getOperand(size_t index) const = 0;
    virtual size_t numOperands() const = 0;
    inline MInstruction *getInput(size_t index) const {
        return getOperand(index)->ins();
    }
    virtual MIRType requiredInputType(size_t index) const {
        return MIRType_None;
    }

    // Replaces an operand, taking care to update use chains. No memory is
    // allocated; the existing data structures are re-linked.
    void replaceOperand(MUse *prev, MUse *use, MInstruction *ins);
    void replaceOperand(MUseIterator &use, MInstruction *ins);

    bool addUse(MIRGenerator *gen, MInstruction *ins, size_t index) {
        uses_ = MUse::New(gen, uses_, ins, index);
        return !!uses_;
    }

    // Adds a use from a node that is being recycled during operand
    // replacement.
    void addUse(MUse *use) {
        JS_ASSERT(use->ins()->getInput(use->index()) == this);
        use->setNext(uses_);
        uses_ = use;
    }

  public:   // Functions for analysis phases.
    // Analyzes inputs and uses and updates type information. If type
    // information changed, returns true, otherwise, returns false.
    bool addUsedTypes(uint32 types) {
        uint32 newTypes = types | usedTypes();
        if (newTypes == usedTypes())
            return false;
        usedTypes_ = newTypes;
        return true;
    }
    bool useAsType(MIRType type) {
        JS_ASSERT(type < MIRType_Value);
        return addUsedTypes(1 << uint32(type));
    }
    uint32 usedTypes() const {
        return usedTypes_;
    }

  public:
    // Opcode testing and casts.
#   define OPCODE_CASTS(opcode)                                             \
    bool is##opcode() const {                                               \
        return op() == Op_##opcode;                                         \
    }                                                                       \
    inline M##opcode *to##opcode();
    MIR_OPCODE_LIST(OPCODE_CASTS)
#   undef OPCODE_CASTS

    virtual bool accept(MInstructionVisitor *visitor) = 0;

  protected:
    // Sets a raw operand; instruction implementation dependent.
    virtual void setOperand(size_t index, MOperand *operand) = 0;
    void setOperand(size_t index, MInstruction *ins);

    // Initializes an operand for the first time.
    bool initOperand(MIRGenerator *gen, size_t index, MInstruction *ins) {
        MOperand *operand = MOperand::New(gen, ins);
        if (!operand)
            return false;
        setOperand(index, operand);
        return ins->addUse(gen, this, index);
    }

    void setResultType(MIRType type) {
        resultType_ = type;
    }
};

#define INSTRUCTION_HEADER(opcode)                                          \
    Opcode op() const {                                                     \
        return MInstruction::Op_##opcode;                                   \
    }                                                                       \
    bool accept(MInstructionVisitor *visitor) {                             \
        return visitor->visit(this);                                        \
    }

template <typename T, size_t Arity>
class MFixedArityList
{
    T list_[Arity];

  public:
    T &operator [](size_t index) {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
};

template <typename T>
class MFixedArityList<T, 0>
{
  public:
    T &operator [](size_t index) {
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
    const T &operator [](size_t index) const {
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
};

template <size_t Arity>
class MAryInstruction : public MInstruction
{
  protected:
    MFixedArityList<MOperand *, Arity> operands_;

    void setOperand(size_t index, MOperand *operand) {
        operands_[index] = operand;
    }

  public:
    MOperand *getOperand(size_t index) const {
        return operands_[index];
    }
    size_t numOperands() const {
        return Arity;
    }
};

class MConstant : public MAryInstruction<0>
{
    js::Value value_;

    MConstant(const Value &v);

  public:
    INSTRUCTION_HEADER(Constant);
    static MConstant *New(MIRGenerator *gen, const Value &v);

    const js::Value &value() const {
        return value_;
    }
    void printOpcode(FILE *fp);
};

class MParameter : public MAryInstruction<0>
{
    int32 index_;

  public:
    static const int32 CALLEE_SLOT = -2;
    static const int32 THIS_SLOT = -1;

    MParameter(int32 index)
      : index_(index)
    {
        setResultType(MIRType_Value);
    }

  public:
    INSTRUCTION_HEADER(Parameter);
    static MParameter *New(MIRGenerator *gen, int32 index);

    int32 index() const {
        return index_;
    }
    void printOpcode(FILE *fp);
};

class MControlInstruction : public MInstruction
{
  protected:
    MBasicBlock *successors[2];

  public:
    MControlInstruction()
      : successors()
    { }

    uint32 numSuccessors() const {
        if (successors[1])
            return 2;
        if (successors[0])
            return 1;
        return 0;
    }

    MBasicBlock *getSuccessor(uint32 i) const {
        JS_ASSERT(i < numSuccessors());
        return successors[i];
    }
};

template <size_t Arity>
class MAryControlInstruction : public MControlInstruction
{
    MFixedArityList<MOperand *, Arity> operands_;

  protected:
    void setOperand(size_t index, MOperand *operand) {
        operands_[index] = operand;
    }

  public:
    MOperand *getOperand(size_t index) const {
        return operands_[index];
    }
    size_t numOperands() const {
        return Arity;
    }
};

class MGoto : public MAryControlInstruction<0>
{
    MGoto(MBasicBlock *target)
    {
        successors[0] = target;
    }

  public:
    INSTRUCTION_HEADER(Goto);
    static MGoto *New(MIRGenerator *gen, MBasicBlock *target);
};

class MTest : public MAryControlInstruction<1>
{
    MTest(MBasicBlock *if_true, MBasicBlock *if_false)
    {
        successors[0] = if_true;
        successors[1] = if_false;
    }

  public:
    INSTRUCTION_HEADER(Test);
    static MTest *New(MIRGenerator *gen, MInstruction *ins,
                      MBasicBlock *ifTrue, MBasicBlock *ifFalse);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
};

class MReturn : public MAryControlInstruction<1>
{
  public:
    INSTRUCTION_HEADER(Return);
    static MReturn *New(MIRGenerator *gen, MInstruction *ins);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Value;
    }
};

class MUnaryInstruction : public MAryInstruction<1>
{
  protected:
    inline bool init(MIRGenerator *gen, MInstruction *ins)
    {
        if (!initOperand(gen, 0, ins))
            return false;
        return true;
    }
};

class MBinaryInstruction : public MAryInstruction<2>
{
  protected:
    inline bool init(MIRGenerator *gen, MInstruction *left, MInstruction *right)
    {
        if (!initOperand(gen, 0, left) ||
            !initOperand(gen, 1, right)) {
            return false;
        }
        return true;
    }
};

class MCopy : public MUnaryInstruction
{
    MCopy(MInstruction *ins)
    {
        setResultType(ins->type());
    }

  public:
    INSTRUCTION_HEADER(Copy);
    static MCopy *New(MIRGenerator *gen, MInstruction *ins);

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
};

class MBox : public MUnaryInstruction
{
  public:
    INSTRUCTION_HEADER(Box);
    static MBox *New(MIRGenerator *gen, MInstruction *ins)
    {
        // Cannot box a box.
        JS_ASSERT(ins->type() != MIRType_Value);

        MBox *box = new (gen->temp()) MBox();
        if (!box || !box->init(gen, ins))
            return NULL;
        return box;
    }

    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
};

class MConvert : public MUnaryInstruction
{
    MConvert(MIRType type)
    {
        setResultType(type);
    }

  public:
    INSTRUCTION_HEADER(Convert);
    static MConvert *New(MIRGenerator *gen, MInstruction *ins, MIRType type)
    {
        // Conversions must be pure, for now. The front-end is responsible for
        // downgrading specializations that would result in impure conversions.
        // This assertion is also too strict, it will have to be modified
        // later.
        JS_ASSERT(ins->type() != MIRType_Object);

        MConvert *convert = new (gen->temp()) MConvert(type);
        if (!convert || !convert->init(gen, ins))
            return NULL;
        return convert;
    }
};

class MBitAnd : public MBinaryInstruction
{
    MBitAnd()
    {
        setResultType(MIRType_Int32);
    }

  public:
    INSTRUCTION_HEADER(BitAnd);
    static MBitAnd *New(MIRGenerator *gen, MInstruction *left, MInstruction *right);

    MIRType requiredInputType(size_t index) const {
        return assumedType();
    }
};

class MPhi : public MInstruction
{
    js::Vector<MOperand *, 2, TempAllocPolicy> inputs_;
    uint32 slot_;

    MPhi(JSContext *cx, uint32 slot)
      : inputs_(TempAllocPolicy(cx)),
        slot_(slot)
    {
    }

  protected:
    void setOperand(size_t index, MOperand *operand) {
        inputs_[index] = operand;
    }

  public:
    INSTRUCTION_HEADER(Phi);
    static MPhi *New(MIRGenerator *gen, uint32 slot);

    MOperand *getOperand(size_t index) const {
        return inputs_[index];
    }
    size_t numOperands() const {
        return inputs_.length();
    }
    uint32 slot() const {
        return slot_;
    }
    bool addInput(MIRGenerator *gen, MInstruction *ins);
    MIRType requiredInputType(size_t index) const {
        return MIRType_Any;
    }
};

#undef INSTRUCTION_HEADER

inline
MUseIterator::MUseIterator(MInstruction *def)
  : def(def),
    use(def->uses()),
    prev_(NULL)
{
}

inline MUse *
MUseIterator::unlink()
{
    MUse *old = use;
    use = def->removeUse(prev(), use);
    return old;
}

// Implement opcode casts now that the compiler can see the inheritance.
#define OPCODE_CASTS(opcode)                                                \
    M##opcode *MInstruction::to##opcode()                                   \
    {                                                                       \
        JS_ASSERT(is##opcode());                                            \
        return static_cast<M##opcode *>(this);                              \
    }
MIR_OPCODE_LIST(OPCODE_CASTS)
#undef OPCODE_CASTS

} // namespace ion
} // namespace js

#endif // jsion_mir_h__

