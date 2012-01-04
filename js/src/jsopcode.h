/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef jsopcode_h___
#define jsopcode_h___

/*
 * JS bytecode definitions.
 */
#include <stddef.h>
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C

/*
 * JS operation bytecodes.
 */
typedef enum JSOp {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    op = val,
#include "jsopcode.tbl"
#undef OPDEF
    JSOP_LIMIT,

    /*
     * These pseudo-ops help js_DecompileValueGenerator decompile JSOP_SETPROP,
     * JSOP_SETELEM, and comprehension-tails, respectively.  They are never
     * stored in bytecode, so they don't preempt valid opcodes.
     */
    JSOP_GETPROP2 = JSOP_LIMIT,
    JSOP_GETELEM2 = JSOP_LIMIT + 1,
    JSOP_FORLOCAL = JSOP_LIMIT + 2,
    JSOP_FAKE_LIMIT = JSOP_FORLOCAL
} JSOp;

/*
 * JS bytecode formats.
 */
#define JOF_BYTE          0       /* single bytecode, no immediates */
#define JOF_JUMP          1       /* signed 16-bit jump offset immediate */
#define JOF_ATOM          2       /* unsigned 16-bit constant index */
#define JOF_UINT16        3       /* unsigned 16-bit immediate operand */
#define JOF_TABLESWITCH   4       /* table switch */
#define JOF_LOOKUPSWITCH  5       /* lookup switch */
#define JOF_QARG          6       /* quickened get/set function argument ops */
#define JOF_LOCAL         7       /* var or block-local variable */
#define JOF_SLOTATOM      8       /* uint16_t slot + constant index */
#define JOF_JUMPX         9       /* signed 32-bit jump offset immediate */
#define JOF_TABLESWITCHX  10      /* extended (32-bit offset) table switch */
#define JOF_LOOKUPSWITCHX 11      /* extended (32-bit offset) lookup switch */
#define JOF_UINT24        12      /* extended unsigned 24-bit literal (index) */
#define JOF_UINT8         13      /* uint8_t immediate, e.g. top 8 bits of 24-bit
                                     atom index */
#define JOF_INT32         14      /* int32_t immediate operand */
#define JOF_OBJECT        15      /* unsigned 16-bit object index */
#define JOF_SLOTOBJECT    16      /* uint16_t slot index + object index */
#define JOF_REGEXP        17      /* unsigned 16-bit regexp index */
#define JOF_INT8          18      /* int8_t immediate operand */
#define JOF_ATOMOBJECT    19      /* uint16_t constant index + object index */
#define JOF_UINT16PAIR    20      /* pair of uint16_t immediates */
#define JOF_TYPEMASK      0x001f  /* mask for above immediate types */

#define JOF_NAME          (1U<<5) /* name operation */
#define JOF_PROP          (2U<<5) /* obj.prop operation */
#define JOF_ELEM          (3U<<5) /* obj[index] operation */
#define JOF_XMLNAME       (4U<<5) /* XML name: *, a::b, @a, @a::b, etc. */
#define JOF_VARPROP       (5U<<5) /* x.prop for this, arg, var, or local x */
#define JOF_MODEMASK      (7U<<5) /* mask for above addressing modes */
#define JOF_SET           (1U<<8) /* set (i.e., assignment) operation */
#define JOF_DEL           (1U<<9) /* delete operation */
#define JOF_DEC          (1U<<10) /* decrement (--, not ++) opcode */
#define JOF_INC          (2U<<10) /* increment (++, not --) opcode */
#define JOF_INCDEC       (3U<<10) /* increment or decrement opcode */
#define JOF_POST         (1U<<12) /* postorder increment or decrement */
#define JOF_ASSIGNING     JOF_SET /* hint for Class.resolve, used for ops
                                     that do simplex assignment */
#define JOF_DETECTING    (1U<<14) /* object detection for JSNewResolveOp */
#define JOF_BACKPATCH    (1U<<15) /* backpatch placeholder during codegen */
#define JOF_LEFTASSOC    (1U<<16) /* left-associative operator */
#define JOF_DECLARING    (1U<<17) /* var, const, or function declaration op */
#define JOF_INDEXBASE    (1U<<18) /* atom segment base setting prefix op */
#define JOF_CALLOP       (1U<<19) /* call operation that pushes function and
                                     this */
#define JOF_PARENHEAD    (1U<<20) /* opcode consumes value of expression in
                                     parenthesized statement head */
#define JOF_INVOKE       (1U<<21) /* JSOP_CALL, JSOP_NEW, JSOP_EVAL */
#define JOF_TMPSLOT      (1U<<22) /* interpreter uses extra temporary slot
                                     to root intermediate objects besides
                                     the slots opcode uses */
#define JOF_TMPSLOT2     (2U<<22) /* interpreter uses extra 2 temporary slot
                                     besides the slots opcode uses */
#define JOF_TMPSLOT3     (3U<<22) /* interpreter uses extra 3 temporary slot
                                     besides the slots opcode uses */
#define JOF_TMPSLOT_SHIFT 22
#define JOF_TMPSLOT_MASK  (JS_BITMASK(2) << JOF_TMPSLOT_SHIFT)

#define JOF_SHARPSLOT    (1U<<24) /* first immediate is uint16_t stack slot no.
                                     that needs fixup when in global code (see
                                     js::frontend::CompileScript) */
#define JOF_GNAME        (1U<<25) /* predicted global name */
#define JOF_TYPESET      (1U<<26) /* has an entry in a script's type sets */
#define JOF_DECOMPOSE    (1U<<27) /* followed by an equivalent decomposed
                                   * version of the opcode */
#define JOF_ARITH        (1U<<28) /* unary or binary arithmetic opcode */

/* Shorthands for type from format and type from opcode. */
#define JOF_TYPE(fmt)   ((fmt) & JOF_TYPEMASK)
#define JOF_OPTYPE(op)  JOF_TYPE(js_CodeSpec[op].format)

/* Shorthands for mode from format and mode from opcode. */
#define JOF_MODE(fmt)   ((fmt) & JOF_MODEMASK)
#define JOF_OPMODE(op)  JOF_MODE(js_CodeSpec[op].format)

#define JOF_TYPE_IS_EXTENDED_JUMP(t) \
    ((unsigned)((t) - JOF_JUMPX) <= (unsigned)(JOF_LOOKUPSWITCHX - JOF_JUMPX))

/*
 * Immediate operand getters, setters, and bounds.
 */

/* Common uint16_t immediate format helpers. */
#define UINT16_LEN              2
#define UINT16_HI(i)            ((jsbytecode)((i) >> 8))
#define UINT16_LO(i)            ((jsbytecode)(i))
#define GET_UINT16(pc)          ((uintN)(((pc)[1] << 8) | (pc)[2]))
#define SET_UINT16(pc,i)        ((pc)[1] = UINT16_HI(i), (pc)[2] = UINT16_LO(i))
#define UINT16_LIMIT            ((uintN)1 << 16)

/* Short (2-byte signed offset) relative jump macros. */
#define JUMP_OFFSET_LEN         2
#define JUMP_OFFSET_HI(off)     ((jsbytecode)((off) >> 8))
#define JUMP_OFFSET_LO(off)     ((jsbytecode)(off))
#define GET_JUMP_OFFSET(pc)     (int16_t(GET_UINT16(pc)))
#define SET_JUMP_OFFSET(pc,off) ((pc)[1] = JUMP_OFFSET_HI(off),               \
                                 (pc)[2] = JUMP_OFFSET_LO(off))
#define JUMP_OFFSET_MIN         ((int16_t)0x8000)
#define JUMP_OFFSET_MAX         ((int16_t)0x7fff)

/*
 * When a short jump won't hold a relative offset, its 2-byte immediate offset
 * operand is an unsigned index of a span-dependency record, maintained until
 * code generation finishes -- after which some (but we hope not nearly all)
 * span-dependent jumps must be extended (see js::frontend::OptimizeSpanDeps in
 * frontend/BytecodeEmitter.cpp).
 *
 * If the span-dependency record index overflows SPANDEP_INDEX_MAX, the jump
 * offset will contain SPANDEP_INDEX_HUGE, indicating that the record must be
 * found (via binary search) by its "before span-dependency optimization" pc
 * offset (from script main entry point).
 */
#define GET_SPANDEP_INDEX(pc)   (uint16_t(GET_UINT16(pc)))
#define SET_SPANDEP_INDEX(pc,i) ((pc)[1] = JUMP_OFFSET_HI(i),                 \
                                 (pc)[2] = JUMP_OFFSET_LO(i))
#define SPANDEP_INDEX_MAX       ((uint16_t)0xfffe)
#define SPANDEP_INDEX_HUGE      ((uint16_t)0xffff)

/* Ultimately, if short jumps won't do, emit long (4-byte signed) offsets. */
#define JUMPX_OFFSET_LEN        4
#define JUMPX_OFFSET_B3(off)    ((jsbytecode)((off) >> 24))
#define JUMPX_OFFSET_B2(off)    ((jsbytecode)((off) >> 16))
#define JUMPX_OFFSET_B1(off)    ((jsbytecode)((off) >> 8))
#define JUMPX_OFFSET_B0(off)    ((jsbytecode)(off))
#define GET_JUMPX_OFFSET(pc)    (int32_t(((pc)[1] << 24) | ((pc)[2] << 16)    \
                                          | ((pc)[3] << 8) | (pc)[4]))
#define SET_JUMPX_OFFSET(pc,off)((pc)[1] = JUMPX_OFFSET_B3(off),              \
                                 (pc)[2] = JUMPX_OFFSET_B2(off),              \
                                 (pc)[3] = JUMPX_OFFSET_B1(off),              \
                                 (pc)[4] = JUMPX_OFFSET_B0(off))
#define JUMPX_OFFSET_MIN        (int32_t(0x80000000))
#define JUMPX_OFFSET_MAX        (int32_t(0x7fffffff))

/*
 * A literal is indexed by a per-script atom or object maps. Most scripts
 * have relatively few literals, so the standard JOF_ATOM, JOF_OBJECT and
 * JOF_REGEXP formats specifies a fixed 16 bits of immediate operand index.
 * A script with more than 64K literals must wrap the bytecode into
 * JSOP_INDEXBASE and JSOP_RESETBASE pair.
 */
#define INDEX_LEN               2
#define INDEX_HI(i)             ((jsbytecode)((i) >> 8))
#define INDEX_LO(i)             ((jsbytecode)(i))
#define GET_INDEX(pc)           GET_UINT16(pc)
#define SET_INDEX(pc,i)         ((pc)[1] = INDEX_HI(i), (pc)[2] = INDEX_LO(i))

#define GET_INDEXBASE(pc)       (JS_ASSERT(*(pc) == JSOP_INDEXBASE),          \
                                 ((uintN)((pc)[1])) << 16)
#define INDEXBASE_LEN           1

#define UINT24_HI(i)            ((jsbytecode)((i) >> 16))
#define UINT24_MID(i)           ((jsbytecode)((i) >> 8))
#define UINT24_LO(i)            ((jsbytecode)(i))
#define GET_UINT24(pc)          ((jsatomid)(((pc)[1] << 16) |                 \
                                            ((pc)[2] << 8) |                  \
                                            (pc)[3]))
#define SET_UINT24(pc,i)        ((pc)[1] = UINT24_HI(i),                      \
                                 (pc)[2] = UINT24_MID(i),                     \
                                 (pc)[3] = UINT24_LO(i))

#define GET_INT8(pc)            ((jsint)int8_t((pc)[1]))

#define GET_INT32(pc)           ((jsint)((uint32_t((pc)[1]) << 24) |          \
                                         (uint32_t((pc)[2]) << 16) |          \
                                         (uint32_t((pc)[3]) << 8)  |          \
                                         uint32_t((pc)[4])))
#define SET_INT32(pc,i)         ((pc)[1] = (jsbytecode)(uint32_t(i) >> 24),   \
                                 (pc)[2] = (jsbytecode)(uint32_t(i) >> 16),   \
                                 (pc)[3] = (jsbytecode)(uint32_t(i) >> 8),    \
                                 (pc)[4] = (jsbytecode)uint32_t(i))

/* Index limit is determined by SN_3BYTE_OFFSET_FLAG, see frontend/BytecodeEmitter.h. */
#define INDEX_LIMIT_LOG2        23
#define INDEX_LIMIT             (uint32_t(1) << INDEX_LIMIT_LOG2)

/* Actual argument count operand format helpers. */
#define ARGC_HI(argc)           UINT16_HI(argc)
#define ARGC_LO(argc)           UINT16_LO(argc)
#define GET_ARGC(pc)            GET_UINT16(pc)
#define ARGC_LIMIT              UINT16_LIMIT

/* Synonyms for quick JOF_QARG and JOF_LOCAL bytecodes. */
#define GET_ARGNO(pc)           GET_UINT16(pc)
#define SET_ARGNO(pc,argno)     SET_UINT16(pc,argno)
#define ARGNO_LEN               2
#define ARGNO_LIMIT             UINT16_LIMIT

#define GET_SLOTNO(pc)          GET_UINT16(pc)
#define SET_SLOTNO(pc,varno)    SET_UINT16(pc,varno)
#define SLOTNO_LEN              2
#define SLOTNO_LIMIT            UINT16_LIMIT

struct JSCodeSpec {
    int8_t              length;         /* length including opcode byte */
    int8_t              nuses;          /* arity, -1 if variadic */
    int8_t              ndefs;          /* number of stack results */
    uint8_t             prec;           /* operator precedence */
    uint32_t            format;         /* immediate operand format */

#ifdef __cplusplus
    uint32_t type() const { return JOF_TYPE(format); }
#endif
};

extern const JSCodeSpec js_CodeSpec[];
extern uintN            js_NumCodeSpecs;
extern const char       *js_CodeName[];
extern const char       js_EscapeMap[];

/* Silence unreferenced formal parameter warnings */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif

/*
 * Return a GC'ed string containing the chars in str, with any non-printing
 * chars or quotes (' or " as specified by the quote argument) escaped, and
 * with the quote character at the beginning and end of the result string.
 */
extern JSString *
js_QuoteString(JSContext *cx, JSString *str, jschar quote);

/*
 * JSPrinter operations, for printf style message formatting.  The return
 * value from js_GetPrinterOutput() is the printer's cumulative output, in
 * a GC'ed string.
 *
 * strict is true if the context in which the output will appear has
 * already been marked as strict, thus indicating that nested
 * functions need not be re-marked with a strict directive.  It should
 * be false in the outermost printer.
 */

extern JSPrinter *
js_NewPrinter(JSContext *cx, const char *name, JSFunction *fun,
              uintN indent, JSBool pretty, JSBool grouped, JSBool strict);

extern void
js_DestroyPrinter(JSPrinter *jp);

extern JSString *
js_GetPrinterOutput(JSPrinter *jp);

extern int
js_printf(JSPrinter *jp, const char *format, ...);

extern JSBool
js_puts(JSPrinter *jp, const char *s);

/*
 * Get index operand from the bytecode using a bytecode analysis to deduce the
 * the index register. This function is infallible, in spite of taking cx as
 * its first parameter; it uses only cx->runtime when calling JS_GetTrapOpcode.
 * The GET_*_FROM_BYTECODE macros that call it pick up cx from their caller's
 * lexical environments.
 */
uintN
js_GetIndexFromBytecode(JSScript *script, jsbytecode *pc, ptrdiff_t pcoff);

/*
 * A slower version of GET_ATOM when the caller does not want to maintain
 * the index segment register itself.
 */
#define GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom)                       \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(*(pc) != JSOP_DOUBLE);                                      \
        JS_ASSERT(js_CodeSpec[*(pc)].format & JOF_ATOM);                      \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        (atom) = (script)->getAtom(index_);                                   \
    JS_END_MACRO

#define GET_NAME_FROM_BYTECODE(script, pc, pcoff, name)                       \
    JS_BEGIN_MACRO                                                            \
        JSAtom *atom_;                                                        \
        GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom_);                     \
        JS_ASSERT(js_CodeSpec[*(pc)].format & (JOF_NAME | JOF_PROP));         \
        (name) = atom_->asPropertyName();                                     \
    JS_END_MACRO

#define GET_DOUBLE_FROM_BYTECODE(script, pc, pcoff, dbl)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        JS_ASSERT(index_ < (script)->consts()->length);                       \
        (dbl) = (script)->getConst(index_).toDouble();                        \
    JS_END_MACRO

#define GET_OBJECT_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        obj = (script)->getObject(index_);                                    \
    JS_END_MACRO

#define GET_FUNCTION_FROM_BYTECODE(script, pc, pcoff, fun)                    \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        fun = (script)->getFunction(index_);                                  \
    JS_END_MACRO

#define GET_REGEXP_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        obj = (script)->getRegExp(index_);                                    \
    JS_END_MACRO

#ifdef __cplusplus
namespace js {

extern uintN
StackUses(JSScript *script, jsbytecode *pc);

extern uintN
StackDefs(JSScript *script, jsbytecode *pc);

}  /* namespace js */
#endif  /* __cplusplus */

/*
 * Decompilers, for script, function, and expression pretty-printing.
 */
extern JSBool
js_DecompileScript(JSPrinter *jp, JSScript *script);

extern JSBool
js_DecompileFunctionBody(JSPrinter *jp);

extern JSBool
js_DecompileFunction(JSPrinter *jp);

/*
 * Some C++ compilers treat the language linkage (extern "C" vs.
 * extern "C++") as part of function (and thus pointer-to-function)
 * types. The use of this typedef (defined in "C") ensures that
 * js_DecompileToString's definition (in "C++") gets matched up with
 * this declaration.
 */
typedef JSBool (* JSDecompilerPtr)(JSPrinter *);

extern JSString *
js_DecompileToString(JSContext *cx, const char *name, JSFunction *fun,
                     uintN indent, JSBool pretty, JSBool grouped, JSBool strict,
                     JSDecompilerPtr decompiler);

/*
 * Find the source expression that resulted in v, and return a newly allocated
 * C-string containing it.  Fall back on v's string conversion (fallback) if we
 * can't find the bytecode that generated and pushed v on the operand stack.
 *
 * Search the current stack frame if spindex is JSDVG_SEARCH_STACK.  Don't
 * look for v on the stack if spindex is JSDVG_IGNORE_STACK.  Otherwise,
 * spindex is the negative index of v, measured from cx->fp->sp, or from a
 * lower frame's sp if cx->fp is native.
 *
 * The caller must call JS_free on the result after a succsesful call.
 */
extern char *
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback);

/*
 * Given bytecode address pc in script's main program code, return the operand
 * stack depth just before (JSOp) *pc executes.
 */
extern uintN
js_ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

JS_END_EXTERN_C

#define JSDVG_IGNORE_STACK      0
#define JSDVG_SEARCH_STACK      1

#ifdef __cplusplus
/*
 * Get the length of variable-length bytecode like JSOP_TABLESWITCH.
 */
extern size_t
js_GetVariableBytecodeLength(jsbytecode *pc);

namespace js {

static inline char *
DecompileValueGenerator(JSContext *cx, intN spindex, const Value &v,
                        JSString *fallback)
{
    return js_DecompileValueGenerator(cx, spindex, v, fallback);
}

/*
 * Sprintf, but with unlimited and automatically allocated buffering.
 */
struct Sprinter {
    JSContext       *context;       /* context executing the decompiler */
    LifoAlloc       *pool;          /* string allocation pool */
    char            *base;          /* base address of buffer in pool */
    size_t          size;           /* size of buffer allocated at base */
    ptrdiff_t       offset;         /* offset of next free char in buffer */
};

#define INIT_SPRINTER(cx, sp, ap, off) \
    ((sp)->context = cx, (sp)->pool = ap, (sp)->base = NULL, (sp)->size = 0,  \
     (sp)->offset = off)

/*
 * Attempt to reserve len space in sp (including a trailing NULL byte). If the
 * attempt succeeds, return a pointer to the start of that space and adjust the
 * length of sp's contents. The caller *must* completely fill this space
 * (including the space for the trailing NULL byte) on success.
 */
extern char *
SprintReserveAmount(Sprinter *sp, size_t len);

extern ptrdiff_t
SprintPut(Sprinter *sp, const char *s, size_t len);

extern ptrdiff_t
SprintCString(Sprinter *sp, const char *s);

extern ptrdiff_t
SprintString(Sprinter *sp, JSString *str);

extern ptrdiff_t
Sprint(Sprinter *sp, const char *format, ...);

extern bool
CallResultEscapes(jsbytecode *pc);

static inline uintN
GetDecomposeLength(jsbytecode *pc, size_t len)
{
    /*
     * The last byte of a DECOMPOSE op stores the decomposed length. This can
     * vary across different instances of an opcode due to INDEXBASE ops.
     */
    JS_ASSERT(size_t(js_CodeSpec[*pc].length) == len);
    return (uintN) pc[len - 1];
}

static inline uintN
GetBytecodeLength(jsbytecode *pc)
{
    JSOp op = (JSOp)*pc;
    JS_ASSERT(op < JSOP_LIMIT);

    if (js_CodeSpec[op].length != -1)
        return js_CodeSpec[op].length;
    return js_GetVariableBytecodeLength(pc);
}

extern bool
IsValidBytecodeOffset(JSContext *cx, JSScript *script, size_t offset);

inline bool
FlowsIntoNext(JSOp op)
{
    /* JSOP_YIELD is considered to flow into the next instruction, like JSOP_CALL. */
    return op != JSOP_STOP && op != JSOP_RETURN && op != JSOP_RETRVAL && op != JSOP_THROW &&
           op != JSOP_GOTO && op != JSOP_GOTOX && op != JSOP_RETSUB;
}

/*
 * Counts accumulated for a single opcode in a script. The counts tracked vary
 * between opcodes, and this structure ensures that counts are accessed in
 * a coherent fashion.
 */
class OpcodeCounts
{
    friend struct ::JSScript;
    double *counts;
#ifdef DEBUG
    size_t capacity;
#endif

 public:

    enum BaseCounts {
        BASE_INTERP = 0,
        BASE_METHODJIT,

        BASE_METHODJIT_STUBS,
        BASE_METHODJIT_CODE,
        BASE_METHODJIT_PICS,

        BASE_COUNT
    };

    enum AccessCounts {
        ACCESS_MONOMORPHIC = BASE_COUNT,
        ACCESS_DIMORPHIC,
        ACCESS_POLYMORPHIC,

        ACCESS_BARRIER,
        ACCESS_NOBARRIER,

        ACCESS_UNDEFINED,
        ACCESS_NULL,
        ACCESS_BOOLEAN,
        ACCESS_INT32,
        ACCESS_DOUBLE,
        ACCESS_STRING,
        ACCESS_OBJECT,

        ACCESS_COUNT
    };

    static bool accessOp(JSOp op) {
        /*
         * Access ops include all name, element and property reads, as well as
         * SETELEM and SETPROP (for ElementCounts/PropertyCounts alignment).
         */
        if (op == JSOP_SETELEM || op == JSOP_SETPROP || op == JSOP_SETMETHOD)
            return true;
        int format = js_CodeSpec[op].format;
        return !!(format & (JOF_NAME | JOF_GNAME | JOF_ELEM | JOF_PROP))
            && !(format & (JOF_SET | JOF_INCDEC));
    }

    enum ElementCounts {
        ELEM_ID_INT = ACCESS_COUNT,
        ELEM_ID_DOUBLE,
        ELEM_ID_OTHER,
        ELEM_ID_UNKNOWN,

        ELEM_OBJECT_TYPED,
        ELEM_OBJECT_PACKED,
        ELEM_OBJECT_DENSE,
        ELEM_OBJECT_OTHER,

        ELEM_COUNT
    };

    static bool elementOp(JSOp op) {
        return accessOp(op) && !!(js_CodeSpec[op].format & JOF_ELEM);
    }

    enum PropertyCounts {
        PROP_STATIC = ACCESS_COUNT,
        PROP_DEFINITE,
        PROP_OTHER,

        PROP_COUNT
    };

    static bool propertyOp(JSOp op) {
        return accessOp(op) && !!(js_CodeSpec[op].format & JOF_PROP);
    }

    enum ArithCounts {
        ARITH_INT = BASE_COUNT,
        ARITH_DOUBLE,
        ARITH_OTHER,
        ARITH_UNKNOWN,

        ARITH_COUNT
    };

    static bool arithOp(JSOp op) {
        return !!(js_CodeSpec[op].format & (JOF_INCDEC | JOF_ARITH));
    }

    static size_t numCounts(JSOp op)
    {
        if (accessOp(op)) {
            if (elementOp(op))
                return ELEM_COUNT;
            if (propertyOp(op))
                return PROP_COUNT;
            return ACCESS_COUNT;
        }
        if (arithOp(op))
            return ARITH_COUNT;
        return BASE_COUNT;
    }

    static const char *countName(JSOp op, size_t which);

    double *rawCounts() { return counts; }

    double& get(size_t which) {
        JS_ASSERT(which < capacity);
        return counts[which];
    }

    // Boolean conversion, for 'if (counters) ...'
    operator void*() const {
        return counts;
    }
};

} /* namespace js */
#endif /* __cplusplus */

#if defined(DEBUG) && defined(__cplusplus)
/*
 * Disassemblers, for debugging only.
 */
extern JS_FRIEND_API(JSBool)
js_Disassemble(JSContext *cx, JSScript *script, JSBool lines, js::Sprinter *sp);

extern JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc, uintN loc,
                JSBool lines, js::Sprinter *sp);

extern JS_FRIEND_API(void)
js_DumpPCCounts(JSContext *cx, JSScript *script, js::Sprinter *sp);
#endif

#endif /* jsopcode_h___ */
