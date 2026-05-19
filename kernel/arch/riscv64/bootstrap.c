#include <arch/csr.h>
#include <stdint.h>

extern void main(uint64_t hartid, uint64_t dtb);

void bootstrap(uint64_t hartid, uint64_t dtb)
{
	uint64_t x = csr_read(mstatus);
	x &= ~MSTATUS_MPP_MASK;
	x |= MSTATUS_MPP_S;
	csr_write(mstatus, x);

	csr_write(mepc, (uint64_t)main);

	csr_write(satp, 0);

	csr_write(medeleg, 0xffff);
	csr_write(mideleg, 0xffff);
	csr_set(sie, SIE_SEIE | SIE_STIE);

	csr_write(0x3b1, 0x3fffffffffffffull);
	csr_write(0x3a0, 0xf);

	__asm__ volatile("mv tp, %0" : : "r"(hartid));
	__asm__ volatile("mv a0, %0" : : "r"(hartid));
	__asm__ volatile("mv a1, %0" : : "r"(dtb));

	mret();
}