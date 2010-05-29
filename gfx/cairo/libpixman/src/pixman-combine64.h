/* WARNING: This file is generated by combine.pl from combine.inc.
   Please edit one of those files rather than this one. */

#line 1 "pixman-combine.c.template"

#define COMPONENT_SIZE 16
#define MASK 0xffffULL
#define ONE_HALF 0x8000ULL

#define A_SHIFT 16 * 3
#define R_SHIFT 16 * 2
#define G_SHIFT 16
#define A_MASK 0xffff000000000000ULL
#define R_MASK 0xffff00000000ULL
#define G_MASK 0xffff0000ULL

#define RB_MASK 0xffff0000ffffULL
#define AG_MASK 0xffff0000ffff0000ULL
#define RB_ONE_HALF 0x800000008000ULL
#define RB_MASK_PLUS_ONE 0x10000000010000ULL

#define ALPHA_16(x) ((x) >> A_SHIFT)
#define RED_16(x) (((x) >> R_SHIFT) & MASK)
#define GREEN_16(x) (((x) >> G_SHIFT) & MASK)
#define BLUE_16(x) ((x) & MASK)

/*
 * Helper macros.
 */

#define MUL_UN16(a, b, t)						\
    ((t) = (a) * (b) + ONE_HALF, ((((t) >> G_SHIFT ) + (t) ) >> G_SHIFT ))

#define DIV_UN16(a, b)							\
    (((uint32_t) (a) * MASK) / (b))

#define ADD_UN16(x, y, t)				     \
    ((t) = x + y,					     \
     (uint64_t) (uint16_t) ((t) | (0 - ((t) >> G_SHIFT))))

#define DIV_ONE_UN16(x)							\
    (((x) + ONE_HALF + (((x) + ONE_HALF) >> G_SHIFT)) >> G_SHIFT)

/*
 * The methods below use some tricks to be able to do two color
 * components at the same time.
 */

/*
 * x_rb = (x_rb * a) / 255
 */
#define UN16_rb_MUL_UN16(x, a, t)						\
    do									\
    {									\
	t  = ((x) & RB_MASK) * (a);					\
	t += RB_ONE_HALF;						\
	x = (t + ((t >> G_SHIFT) & RB_MASK)) >> G_SHIFT;		\
	x &= RB_MASK;							\
    } while (0)

/*
 * x_rb = min (x_rb + y_rb, 255)
 */
#define UN16_rb_ADD_UN16_rb(x, y, t)					\
    do									\
    {									\
	t = ((x) + (y));						\
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);		\
	x = (t & RB_MASK);						\
    } while (0)

/*
 * x_rb = (x_rb * a_rb) / 255
 */
#define UN16_rb_MUL_UN16_rb(x, a, t)					\
    do									\
    {									\
	t  = (x & MASK) * (a & MASK);					\
	t |= (x & R_MASK) * ((a >> R_SHIFT) & MASK);			\
	t += RB_ONE_HALF;						\
	t = (t + ((t >> G_SHIFT) & RB_MASK)) >> G_SHIFT;		\
	x = t & RB_MASK;						\
    } while (0)

/*
 * x_c = (x_c * a) / 255
 */
#define UN16x4_MUL_UN16(x, a)						\
    do									\
    {									\
	uint64_t r1, r2, t;						\
									\
	r1 = (x);							\
	UN16_rb_MUL_UN16 (r1, a, t);					\
									\
	r2 = (x) >> G_SHIFT;						\
	UN16_rb_MUL_UN16 (r2, a, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a) / 255 + y_c
 */
#define UN16x4_MUL_UN16_ADD_UN16x4(x, a, y)				\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = (x);							\
	r2 = (y) & RB_MASK;						\
	UN16_rb_MUL_UN16 (r1, a, t);					\
	UN16_rb_ADD_UN16_rb (r1, r2, t);					\
									\
	r2 = (x) >> G_SHIFT;						\
	r3 = ((y) >> G_SHIFT) & RB_MASK;				\
	UN16_rb_MUL_UN16 (r2, a, t);					\
	UN16_rb_ADD_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a + y_c * b) / 255
 */
#define UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16(x, a, y, b)			\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = x;								\
	r2 = y;								\
	UN16_rb_MUL_UN16 (r1, a, t);					\
	UN16_rb_MUL_UN16 (r2, b, t);					\
	UN16_rb_ADD_UN16_rb (r1, r2, t);					\
									\
	r2 = (x >> G_SHIFT);						\
	r3 = (y >> G_SHIFT);						\
	UN16_rb_MUL_UN16 (r2, a, t);					\
	UN16_rb_MUL_UN16 (r3, b, t);					\
	UN16_rb_ADD_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c) / 255
 */
#define UN16x4_MUL_UN16x4(x, a)						\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = x;								\
	r2 = a;								\
	UN16_rb_MUL_UN16_rb (r1, r2, t);					\
									\
	r2 = x >> G_SHIFT;						\
	r3 = a >> G_SHIFT;						\
	UN16_rb_MUL_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c) / 255 + y_c
 */
#define UN16x4_MUL_UN16x4_ADD_UN16x4(x, a, y)				\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = x;								\
	r2 = a;								\
	UN16_rb_MUL_UN16_rb (r1, r2, t);					\
	r2 = y & RB_MASK;						\
	UN16_rb_ADD_UN16_rb (r1, r2, t);					\
									\
	r2 = (x >> G_SHIFT);						\
	r3 = (a >> G_SHIFT);						\
	UN16_rb_MUL_UN16_rb (r2, r3, t);					\
	r3 = (y >> G_SHIFT) & RB_MASK;					\
	UN16_rb_ADD_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c + y_c * b) / 255
 */
#define UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16(x, a, y, b)			\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = x;								\
	r2 = a;								\
	UN16_rb_MUL_UN16_rb (r1, r2, t);					\
	r2 = y;								\
	UN16_rb_MUL_UN16 (r2, b, t);					\
	UN16_rb_ADD_UN16_rb (r1, r2, t);					\
									\
	r2 = x >> G_SHIFT;						\
	r3 = a >> G_SHIFT;						\
	UN16_rb_MUL_UN16_rb (r2, r3, t);					\
	r3 = y >> G_SHIFT;						\
	UN16_rb_MUL_UN16 (r3, b, t);					\
	UN16_rb_ADD_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)

/*
   x_c = min(x_c + y_c, 255)
 */
#define UN16x4_ADD_UN16x4(x, y)						\
    do									\
    {									\
	uint64_t r1, r2, r3, t;						\
									\
	r1 = x & RB_MASK;						\
	r2 = y & RB_MASK;						\
	UN16_rb_ADD_UN16_rb (r1, r2, t);					\
									\
	r2 = (x >> G_SHIFT) & RB_MASK;					\
	r3 = (y >> G_SHIFT) & RB_MASK;					\
	UN16_rb_ADD_UN16_rb (r2, r3, t);					\
									\
	x = r1 | (r2 << G_SHIFT);					\
    } while (0)
