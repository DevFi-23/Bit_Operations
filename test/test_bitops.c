#include "../include/bitops.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/* ------------------------------------------------------------------ */
/* Mini test framework                                                 */
/* ------------------------------------------------------------------ */
static uint32_t test_total  = 0;
static uint32_t test_passed = 0;
static uint32_t test_failed = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        test_total++;                                                       \
        if (cond) {                                                         \
            test_passed++;                                                  \
        } else {                                                            \
            test_failed++;                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                   \
    } while (0)

#define CHECK_EQ_U32(actual, expected)                                      \
    do {                                                                    \
        uint32_t a_ = (uint32_t)(actual);                                   \
        uint32_t e_ = (uint32_t)(expected);                                 \
        test_total++;                                                       \
        if (a_ == e_) {                                                     \
            test_passed++;                                                  \
        } else {                                                            \
            test_failed++;                                                  \
            printf("FAIL %s:%d: %s = 0x%08" PRIX32                          \
                   ", expected 0x%08" PRIX32 "\n",                          \
                   __FILE__, __LINE__, #actual, a_, e_);                    \
        }                                                                   \
    } while (0)

/* Reference model */
static uint32_t ref_mask(uint32_t w)
{
    uint32_t m = 0u;
    uint32_t i;
    for (i = 0u; i < w && i < 32u; i++) {
        m |= (UINT32_C(1) << i);
    }
    return m;
}

static const uint32_t PATTERNS[] = {
    0x00000000u, 0xFFFFFFFFu, 0xA5A5A5A5u, 0x5A5A5A5Au,
    0x12345678u, 0x80000001u, 0xF78912C4u
};
#define N_PATTERNS (sizeof(PATTERNS) / sizeof(PATTERNS[0]))

static void test_bit_set(void)
{
    uint32_t reg;
    uint32_t i;
    uint32_t p;

    /* Gia tri co dinh */
    reg = 0x00000000u; BIT_SET(reg, 5);  CHECK_EQ_U32(reg, 0x00000020u);
    reg = 0xFFFFFFFFu; BIT_SET(reg, 0);  CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    reg = 0x00000000u; BIT_SET(reg, 0);  CHECK_EQ_U32(reg, 0x00000001u);
    reg = 0x00000000u; BIT_SET(reg, 31); CHECK_EQ_U32(reg, 0x80000000u);

    /* Quet ca 32 vi tri, pos la bien runtime */
    for (i = 0u; i < 32u; i++) {
        reg = 0u;
        BIT_SET(reg, i);
        CHECK_EQ_U32(reg, UINT32_C(1) << i);
    }

    /* Cac bit khac khong bi anh huong */
    for (p = 0u; p < N_PATTERNS; p++) {
        for (i = 0u; i < 32u; i++) {
            reg = PATTERNS[p];
            BIT_SET(reg, i);
            CHECK_EQ_U32(reg, PATTERNS[p] | (UINT32_C(1) << i));
        }
    }

    /* Idempotent: set 2 lan = set 1 lan */
    reg = 0u; BIT_SET(reg, 9); BIT_SET(reg, 9);
    CHECK_EQ_U32(reg, 0x00000200u);

    /* pos ngoai dai: reg khong doi */
    reg = 0xA5A5A5A5u; BIT_SET(reg, 32);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0u; BIT_SET(reg, 33);
    CHECK_EQ_U32(reg, 0u);
    reg = 0u; BIT_SET(reg, 100);
    CHECK_EQ_U32(reg, 0u);
    reg = 0u; BIT_SET(reg, 0xFFFFFFFFu);
    CHECK_EQ_U32(reg, 0u);
    {
        uint32_t big = 32u;
        reg = 0u; BIT_SET(reg, big);
        CHECK_EQ_U32(reg, 0u);
    }
    {
        int neg = -1; /* -1 -> unsigned rat lon -> bi chan */
        reg = 0u; BIT_SET(reg, neg);
        CHECK_EQ_U32(reg, 0u);
    }

    /* pos la bieu thuc: kiem tra ngoac trong macro */
    reg = 0u; BIT_SET(reg, 2 + 3);
    CHECK_EQ_U32(reg, 0x00000020u);
}

static void test_bit_clr(void)
{
    uint32_t reg;
    uint32_t i;
    uint32_t p;

    reg = 0x00000001u; BIT_CLR(reg, 0);  CHECK_EQ_U32(reg, 0x00000000u);
    reg = 0xF78912C4u; BIT_CLR(reg, 7);  CHECK_EQ_U32(reg, 0xF7891244u);
    reg = 0xF0000000u; BIT_CLR(reg, 31); CHECK_EQ_U32(reg, 0x70000000u);
    reg = 0x00000000u; BIT_CLR(reg, 3);  CHECK_EQ_U32(reg, 0x00000000u);

    for (i = 0u; i < 32u; i++) {
        reg = 0xFFFFFFFFu;
        BIT_CLR(reg, i);
        CHECK_EQ_U32(reg, ~(UINT32_C(1) << i));
    }

    for (p = 0u; p < N_PATTERNS; p++) {
        for (i = 0u; i < 32u; i++) {
            reg = PATTERNS[p];
            BIT_CLR(reg, i);
            CHECK_EQ_U32(reg, PATTERNS[p] & ~(UINT32_C(1) << i));
        }
    }

    /* Idempotent */
    reg = 0xFFFFFFFFu; BIT_CLR(reg, 4); BIT_CLR(reg, 4);
    CHECK_EQ_U32(reg, 0xFFFFFFEFu);

    /* pos ngoai dai */
    reg = 0xFFFFFFFFu; BIT_CLR(reg, 32);
    CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    reg = 0xFFFFFFFFu; BIT_CLR(reg, 33);
    CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    reg = 0xFFFFFFFFu; BIT_CLR(reg, 0xFFFFFFFFu);
    CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    {
        int neg = -1;
        reg = 0xFFFFFFFFu; BIT_CLR(reg, neg);
        CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    }

    /* pos la bieu thuc */
    reg = 0xFFFFFFFFu; BIT_CLR(reg, 1 + 1);
    CHECK_EQ_U32(reg, 0xFFFFFFFBu);
}

static void test_bit_tgl(void)
{
    uint32_t reg;
    uint32_t i;
    uint32_t p;

    reg = 0x00000000u; BIT_TGL(reg, 0);  CHECK_EQ_U32(reg, 0x00000001u);
    reg = 0x12345678u; BIT_TGL(reg, 8);  CHECK_EQ_U32(reg, 0x12345778u);
    reg = 0x00000000u; BIT_TGL(reg, 31); CHECK_EQ_U32(reg, 0x80000000u);
    reg = 0xFFFFFFFFu; BIT_TGL(reg, 31); CHECK_EQ_U32(reg, 0x7FFFFFFFu);

    /* Toggle 2 lan thi ve gia tri ban dau, va moi lan chi doi dung 1 bit */
    for (p = 0u; p < N_PATTERNS; p++) {
        for (i = 0u; i < 32u; i++) {
            reg = PATTERNS[p];
            BIT_TGL(reg, i);
            CHECK_EQ_U32(reg, PATTERNS[p] ^ (UINT32_C(1) << i));
            BIT_TGL(reg, i);
            CHECK_EQ_U32(reg, PATTERNS[p]);
        }
    }

    /* pos ngoai dai */
    reg = 0xFFFFFFFFu; BIT_TGL(reg, 32);
    CHECK_EQ_U32(reg, 0xFFFFFFFFu);
    reg = 0x0F0F0F0Fu; BIT_TGL(reg, 64);
    CHECK_EQ_U32(reg, 0x0F0F0F0Fu);
    reg = 0x0F0F0F0Fu; BIT_TGL(reg, 0xFFFFFFFFu);
    CHECK_EQ_U32(reg, 0x0F0F0F0Fu);
    {
        int neg = -1;
        reg = 0x0F0F0F0Fu; BIT_TGL(reg, neg);
        CHECK_EQ_U32(reg, 0x0F0F0F0Fu);
    }
}

static void test_bit_read(void)
{
    uint32_t reg;
    uint32_t i;
    uint32_t p;

    /* Tra ve dung 0 hoac 1, khong phai mask */
    reg = 0x00000020u;
    CHECK_EQ_U32(BIT_READ(reg, 5), 1u);
    CHECK_EQ_U32(BIT_READ(reg, 4), 0u);
    CHECK_EQ_U32(BIT_READ(reg, 6), 0u);

    reg = 0x80000000u;
    CHECK_EQ_U32(BIT_READ(reg, 31), 1u);
    CHECK_EQ_U32(BIT_READ(reg, 30), 0u);

    reg = 0x00000001u;
    CHECK_EQ_U32(BIT_READ(reg, 0), 1u);
    CHECK_EQ_U32(BIT_READ(reg, 1), 0u);

    /* Quet ca 32 bit cua nhieu pattern, so voi cach tinh khac */
    for (p = 0u; p < N_PATTERNS; p++) {
        for (i = 0u; i < 32u; i++) {
            uint32_t expected = ((PATTERNS[p] & (UINT32_C(1) << i)) != 0u) ? 1u : 0u;
            reg = PATTERNS[p];
            CHECK_EQ_U32(BIT_READ(reg, i), expected);
        }
    }

    /* pos ngoai dai -> 0 (ke ca khi reg toan 1) */
    reg = 0xFFFFFFFFu;
    CHECK_EQ_U32(BIT_READ(reg, 32), 0u);
    CHECK_EQ_U32(BIT_READ(reg, 33), 0u);
    CHECK_EQ_U32(BIT_READ(reg, 100), 0u);
    CHECK_EQ_U32(BIT_READ(reg, 0xFFFFFFFFu), 0u);
    {
        int neg = -1;
        CHECK_EQ_U32(BIT_READ(reg, neg), 0u);
    }

    /* Dung duoc trong bieu thuc / dieu kien */
    reg = 0x00000008u;
    CHECK(BIT_READ(reg, 3) == 1u);
    CHECK(BIT_READ(reg, 2) == 0u);
    if (BIT_READ(reg, 3)) { CHECK(1); } else { CHECK(0); }
    CHECK_EQ_U32(BIT_READ(reg, 1 + 2), 1u);

    /* Doc khong lam thay doi reg */
    reg = 0xA5A5A5A5u;
    (void)BIT_READ(reg, 7);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
}

static void test_mask(void)
{
    uint32_t w;

    CHECK_EQ_U32(MASK(0),  0x00000000u);
    CHECK_EQ_U32(MASK(1),  0x00000001u);
    CHECK_EQ_U32(MASK(2),  0x00000003u);
    CHECK_EQ_U32(MASK(4),  0x0000000Fu);
    CHECK_EQ_U32(MASK(8),  0x000000FFu);
    CHECK_EQ_U32(MASK(16), 0x0000FFFFu);
    CHECK_EQ_U32(MASK(31), 0x7FFFFFFFu);
    CHECK_EQ_U32(MASK(32), 0xFFFFFFFFu);

    /* w > 32 -> mask day du (hanh vi hien tai cua thu vien) */
    CHECK_EQ_U32(MASK(33),  0xFFFFFFFFu);
    CHECK_EQ_U32(MASK(100), 0xFFFFFFFFu);
    CHECK_EQ_U32(MASK(0xFFFFFFFFu), 0xFFFFFFFFu);

    /* Quet w = 0..32 voi w la bien runtime */
    for (w = 0u; w <= 32u; w++) {
        CHECK_EQ_U32(MASK(w), ref_mask(w));
    }
}

static void test_field_get(void)
{
    uint32_t reg;
    uint32_t pos, w, p;

    /* Gia tri co dinh tren 0x12345678 */
    reg = 0x12345678u;
    CHECK_EQ_U32(FIELD_GET(reg, 0, 4),   0x8u);
    CHECK_EQ_U32(FIELD_GET(reg, 4, 8),   0x67u);
    CHECK_EQ_U32(FIELD_GET(reg, 8, 8),   0x56u);
    CHECK_EQ_U32(FIELD_GET(reg, 28, 4),  0x1u);
    CHECK_EQ_U32(FIELD_GET(reg, 16, 16), 0x1234u);
    CHECK_EQ_U32(FIELD_GET(reg, 0, 32),  0x12345678u);
    CHECK_EQ_U32(FIELD_GET(reg, 31, 1),  0u);

    /* Bit cao nhat */
    reg = 0x80000000u;
    CHECK_EQ_U32(FIELD_GET(reg, 31, 1), 1u);
    CHECK_EQ_U32(FIELD_GET(reg, 28, 4), 0x8u);

    /* w = 0 -> 0 */
    reg = 0xFFFFFFFFu;
    CHECK_EQ_U32(FIELD_GET(reg, 0, 0),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 15, 0), 0u);
    CHECK_EQ_U32(FIELD_GET(reg, 31, 0), 0u);

    /* Bien hop le: pos + w == 32 */
    reg = 0xFFFFFFFFu;
    CHECK_EQ_U32(FIELD_GET(reg, 16, 16), 0xFFFFu);
    CHECK_EQ_U32(FIELD_GET(reg, 31, 1),  1u);
    CHECK_EQ_U32(FIELD_GET(reg, 24, 8),  0xFFu);

    /* Ngay ngoai bien / tham so khong hop le -> 0 */
    CHECK_EQ_U32(FIELD_GET(reg, 28, 5),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 31, 2),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 1, 32),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 0, 33),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 32, 1),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 32, 0),  0u);
    CHECK_EQ_U32(FIELD_GET(reg, 100, 4), 0u);
    CHECK_EQ_U32(FIELD_GET(reg, 0xFFFFFFFFu, 1), 0u);
    {
        int neg = -1;
        CHECK_EQ_U32(FIELD_GET(reg, neg, 1), 0u);
    }

    /* Quet moi (pos, w) hop le, so voi model tham chieu */
    for (p = 0u; p < N_PATTERNS; p++) {
        reg = PATTERNS[p];
        for (pos = 0u; pos < 32u; pos++) {
            for (w = 0u; w <= 32u - pos; w++) {
                uint32_t expected = (reg >> pos) & ref_mask(w);
                if (w == 0u) { expected = 0u; }
                CHECK_EQ_U32(FIELD_GET(reg, pos, w), expected);
            }
        }
    }

    /* Quet moi (pos, w) khong hop le -> luon 0 */
    reg = 0xFFFFFFFFu;
    for (pos = 0u; pos < 32u; pos++) {
        for (w = 33u - pos; w <= 40u; w++) {
            CHECK_EQ_U32(FIELD_GET(reg, pos, w), 0u);
        }
    }

    /* pos, w la bieu thuc */
    reg = 0x12345678u;
    CHECK_EQ_U32(FIELD_GET(reg, 2 + 2, 4 + 4), 0x67u);

    /* Doc khong lam thay doi reg */
    reg = 0xA5A5A5A5u;
    (void)FIELD_GET(reg, 4, 8);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
}


static void test_field_set(void)
{
    uint32_t reg;
    uint32_t pos, w, p;

    /* Gia tri co dinh */
    reg = 0x00000000u; FIELD_SET(reg, 4, 8, 0xABu);
    CHECK_EQ_U32(reg, 0x00000AB0u);

    reg = 0xFFFFFFFFu; FIELD_SET(reg, 4, 8, 0x00u);
    CHECK_EQ_U32(reg, 0xFFFFF00Fu);

    /* Giu nguyen bit hai ben */
    reg = 0xFFFFFFFFu; FIELD_SET(reg, 8, 4, 0x5u);
    CHECK_EQ_U32(reg, 0xFFFFF5FFu);

    reg = 0x12345678u; FIELD_SET(reg, 16, 16, 0xABCDu);
    CHECK_EQ_U32(reg, 0xABCD5678u);

    reg = 0x12345678u; FIELD_SET(reg, 0, 32, 0xDEADBEEFu);
    CHECK_EQ_U32(reg, 0xDEADBEEFu);

    reg = 0x00000000u; FIELD_SET(reg, 31, 1, 1u);
    CHECK_EQ_U32(reg, 0x80000000u);

    /* val rong hon w bi cat */
    reg = 0x00000000u; FIELD_SET(reg, 0, 8, 0x1FFu);
    CHECK_EQ_U32(reg, 0x000000FFu);

    reg = 0x00000000u; FIELD_SET(reg, 4, 4, 0xFFu);
    CHECK_EQ_U32(reg, 0x000000F0u);

    /* val am / val 64 bit */
    reg = 0u; FIELD_SET(reg, 8, 8, -1);
    CHECK_EQ_U32(reg, 0x0000FF00u);

    reg = 0u; FIELD_SET(reg, 0, 16, UINT64_C(0x1FFFFFFFF));
    CHECK_EQ_U32(reg, 0x0000FFFFu);

    /* w = 0: reg khong doi */
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 0, 0, 0xFFu);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 31, 0, 0xFFu);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);

    /* Tham so khong hop le: reg khong doi */
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 28, 5, 0x1Fu);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 32, 1, 1u);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 0, 33, 0xFFFFFFFFu);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 31, 2, 3u);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    reg = 0xA5A5A5A5u; FIELD_SET(reg, 0xFFFFFFFFu, 1, 1u);
    CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    {
        int neg = -1;
        reg = 0xA5A5A5A5u; FIELD_SET(reg, neg, 1, 1u);
        CHECK_EQ_U32(reg, 0xA5A5A5A5u);
    }

    /* Quet moi (pos, w) hop le:
     *  - field doc lai == val & mask
     *  - bit ngoai field khong doi */
    for (p = 0u; p < N_PATTERNS; p++) {
        for (pos = 0u; pos < 32u; pos++) {
            for (w = 1u; w <= 32u - pos; w++) {
                uint32_t val   = 0xDEADBEEFu;
                uint32_t m     = ref_mask(w);
                uint32_t fmask = m << pos;
                uint32_t expected = (PATTERNS[p] & ~fmask) | ((val & m) << pos);

                reg = PATTERNS[p];
                FIELD_SET(reg, pos, w, val);
                CHECK_EQ_U32(reg, expected);
                CHECK_EQ_U32(FIELD_GET(reg, pos, w), val & m);
                CHECK_EQ_U32(reg & ~fmask, PATTERNS[p] & ~fmask);
            }
        }
    }

    /* Quet moi (pos, w) khong hop le: reg khong doi */
    for (pos = 0u; pos < 32u; pos++) {
        for (w = 33u - pos; w <= 40u; w++) {
            reg = 0xA5A5A5A5u;
            FIELD_SET(reg, pos, w, 0xFFFFFFFFu);
            CHECK_EQ_U32(reg, 0xA5A5A5A5u);
        }
    }
}

/* ------------------------------------------------------------------ */
int main(void)
{
    test_bit_set();
    test_bit_clr();
    test_bit_tgl();
    test_bit_read();
    test_mask();
    test_field_get();
    test_field_set();

    printf("===SUMMARY===\n");
    printf("Total       : %u\n", (unsigned)test_total);
    printf("Test passed : %u\n", (unsigned)test_passed);
    printf("Test failed : %u\n", (unsigned)test_failed);
    return (test_failed == 0u) ? 0 : 1;
}