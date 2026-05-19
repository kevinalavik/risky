#ifndef ARCH_CSR_H
#define ARCH_CSR_H

#include <stdint.h>

#define CSR_MSTATUS 0x300
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MSCRATCH 0x340
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MTVAL 0x343
#define CSR_MIP 0x344

#define CSR_SSTATUS 0x100
#define CSR_SIE 0x104
#define CSR_STVEC 0x105
#define CSR_SSCRATCH 0x140
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143
#define CSR_SATP 0x180

#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303

#define CSR_TIME 0xc01

#define MSTATUS_MPP_MASK (0x1800000UL)
#define MSTATUS_MPP_S (0x800000UL)
#define MSTATUS_MIE (1UL << 3)

#define SIE_SEIE (1UL << 9)
#define SIE_STIE (1UL << 5)
#define SIE_SSIE (1UL << 1)

#define csr_read(csr) ({ \
    uint64_t __val; \
    __asm__ volatile("csrr %0, " #csr : "=r"(__val)); \
    __val; \
})

#define csr_write(csr, val) do { \
    __asm__ volatile("csrw " #csr ", %0" : : "r"(val)); \
} while (0)

#define csr_set(csr, val) do { \
    __asm__ volatile("csrs " #csr ", %0" : : "r"(val)); \
} while (0)

#define csr_clear(csr, val) do { \
    __asm__ volatile("csrc " #csr ", %0" : : "r"(val)); \
} while (0)

static inline uint64_t csr_read_time(void) {
    uint64_t value;
    __asm__ volatile("csrr %0, time" : "=r"(value));
    return value;
}

static inline uint64_t csr_read_mhartid(void) {
    uint64_t value;
    __asm__ volatile("csrr %0, mhartid" : "=r"(value));
    return value;
}

static inline void fence(void) {
    __asm__ volatile("fence" ::: "memory");
}

static inline void fence_i(void) {
    __asm__ volatile("fence.i" ::: "memory");
}

static inline void wfi(void) {
    __asm__ volatile("wfi");
}

static inline void mret(void) {
    __asm__ volatile("mret");
}

static inline void sret(void) {
    __asm__ volatile("sret");
}

#endif // ARCH_CSR_H