#include <stdint.h>
#include <stdarg.h>
#include <trap.h>
#include <lib/printk.h>
#include <csr.h>
#include <stddef.h>

#define SCAUSE_INTERRUPT_BIT (1ULL << 63)
#define SCAUSE_CAUSE_MASK (~SCAUSE_INTERRUPT_BIT)

#define IRQ_S_SOFTWARE 1
#define IRQ_S_TIMER 5
#define IRQ_S_EXTERNAL 9

#define EXC_INST_MISALIGNED 0
#define EXC_INST_ACCESS 1
#define EXC_ILLEGAL_INST 2
#define EXC_BREAKPOINT 3
#define EXC_LOAD_MISALIGNED 4
#define EXC_LOAD_ACCESS 5
#define EXC_STORE_MISALIGNED 6
#define EXC_STORE_ACCESS 7
#define EXC_ECALL_FROM_U 8
#define EXC_ECALL_FROM_S 9
#define EXC_INST_PAGE_FAULT 12
#define EXC_LOAD_PAGE_FAULT 13
#define EXC_STORE_PAGE_FAULT 15

#define SSTATUS_SIE (1ULL << 1)
#define SSTATUS_SPP (1ULL << 8)

#define SIE_SSIE (1ULL << 1)
#define SIE_STIE (1ULL << 5)
#define SIE_SEIE (1ULL << 9)

#define STVEC_MODE_DIRECT 0ULL
#define STVEC_MODE_MASK 0x3ULL

extern void trap_entry(void);

const char *trap_interrupt_name(uint64_t cause)
{
	switch (cause) {
	case IRQ_S_SOFTWARE:
		return "supervisor software interrupt";
	case IRQ_S_TIMER:
		return "supervisor timer interrupt";
	case IRQ_S_EXTERNAL:
		return "supervisor external interrupt";
	default:
		return "unknown supervisor interrupt";
	}
}

const char *trap_exception_name(uint64_t cause)
{
	switch (cause) {
	case EXC_INST_MISALIGNED:
		return "instruction address misaligned";
	case EXC_INST_ACCESS:
		return "instruction access fault";
	case EXC_ILLEGAL_INST:
		return "illegal instruction";
	case EXC_BREAKPOINT:
		return "breakpoint";
	case EXC_LOAD_MISALIGNED:
		return "load address misaligned";
	case EXC_LOAD_ACCESS:
		return "load access fault";
	case EXC_STORE_MISALIGNED:
		return "store/AMO address misaligned";
	case EXC_STORE_ACCESS:
		return "store/AMO access fault";
	case EXC_ECALL_FROM_U:
		return "environment call from U-mode";
	case EXC_ECALL_FROM_S:
		return "environment call from S-mode";
	case EXC_INST_PAGE_FAULT:
		return "instruction page fault";
	case EXC_LOAD_PAGE_FAULT:
		return "load page fault";
	case EXC_STORE_PAGE_FAULT:
		return "store/AMO page fault";
	default:
		return "unknown exception";
	}
}

static const char *trap_mode_name(const struct trap_frame *tf)
{
	return (tf->sstatus & SSTATUS_SPP) ? "S-mode" : "U-mode";
}

static void trap_dump_frame(const struct trap_frame *tf)
{
	klog("mode:    %s", trap_mode_name(tf));
	klog("sepc:    0x%016lx", tf->sepc);
	klog("sstatus: 0x%016lx", tf->sstatus);
	klog("scause:  0x%016lx", tf->scause);
	klog("stval:   0x%016lx", tf->stval);

	klog("ra: 0x%016lx  sp: 0x%016lx  gp: 0x%016lx  tp: 0x%016lx", tf->ra,
		 tf->sp, tf->gp, tf->tp);

	klog("t0: 0x%016lx  t1: 0x%016lx  t2: 0x%016lx", tf->t0, tf->t1, tf->t2);

	klog("s0: 0x%016lx  s1: 0x%016lx", tf->s0, tf->s1);

	klog("a0: 0x%016lx  a1: 0x%016lx  a2: 0x%016lx  a3: 0x%016lx", tf->a0,
		 tf->a1, tf->a2, tf->a3);

	klog("a4: 0x%016lx  a5: 0x%016lx  a6: 0x%016lx  a7: 0x%016lx", tf->a4,
		 tf->a5, tf->a6, tf->a7);

	klog("s2: 0x%016lx  s3: 0x%016lx  s4: 0x%016lx  s5: 0x%016lx", tf->s2,
		 tf->s3, tf->s4, tf->s5);

	klog("s6: 0x%016lx  s7: 0x%016lx  s8: 0x%016lx  s9: 0x%016lx", tf->s6,
		 tf->s7, tf->s8, tf->s9);

	klog("s10:0x%016lx  s11:0x%016lx", tf->s10, tf->s11);

	klog("t3: 0x%016lx  t4: 0x%016lx  t5: 0x%016lx  t6: 0x%016lx", tf->t3,
		 tf->t4, tf->t5, tf->t6);
}

__attribute__((noreturn)) void kpanic(struct trap_frame *tf, const char *fmt,
									  ...)
{
	uint64_t raw = tf ? tf->scause : 0;
	uint64_t is_interrupt = raw & SCAUSE_INTERRUPT_BIT;
	uint64_t cause = raw & SCAUSE_CAUSE_MASK;

	klog("");
	klog("------------[ kernel panic ]------------");

	if (fmt != NULL) {
		va_list args;
		va_start(args, fmt);
		vklog(fmt, args);
		va_end(args);
	}

	if (tf != NULL) {
		klog("trap: %s cause=%lu <%s>",
			 is_interrupt ? "interrupt" : "exception", cause,
			 is_interrupt ? trap_interrupt_name(cause) :
							trap_exception_name(cause));

		trap_dump_frame(tf);
	}

	klog("----------------------------------------");

	while (1) {
		cpu_wfi();
	}
}

static void handle_interrupt(struct trap_frame *tf, uint64_t cause)
{
	switch (cause) {
	case IRQ_S_TIMER:
		/* todo: tick the scheduler*/
		return;
	case IRQ_S_SOFTWARE:
	case IRQ_S_EXTERNAL:
	default:
		klog(
			"trap: ignoring unhandled interrupt cause=%lu <%s> sepc=0x%lx stval=0x%lx",
			cause, trap_interrupt_name(cause), tf->sepc, tf->stval);
		return;
	}
}

static void handle_exception(struct trap_frame *tf, uint64_t cause)
{
	switch (cause) {
	case EXC_ECALL_FROM_U: {
		tf->sepc += 4;
		kpanic(tf, "U-mode ecall");
	}

	case EXC_ECALL_FROM_S:
		tf->sepc += 4;
		kpanic(tf, "S-mode ecall");

	case EXC_BREAKPOINT:
		kpanic(tf, "breakpoint");

	case EXC_ILLEGAL_INST:
		kpanic(tf, "illegal instruction at sepc=0x%lx", tf->sepc);

	case EXC_INST_PAGE_FAULT:
	case EXC_LOAD_PAGE_FAULT:
	case EXC_STORE_PAGE_FAULT:
		kpanic(tf, "page fault at sepc=0x%lx fault=0x%lx", tf->sepc, tf->stval);

	case EXC_INST_MISALIGNED:
	case EXC_INST_ACCESS:
	case EXC_LOAD_MISALIGNED:
	case EXC_LOAD_ACCESS:
	case EXC_STORE_MISALIGNED:
	case EXC_STORE_ACCESS:
		kpanic(tf, "%s", trap_exception_name(cause));

	default:
		kpanic(tf, "unhandled exception cause=%lu", cause);
	}
}

void trap_handler(struct trap_frame *tf)
{
	uint64_t raw = tf->scause;
	uint64_t is_interrupt = raw & SCAUSE_INTERRUPT_BIT;
	uint64_t cause = raw & SCAUSE_CAUSE_MASK;

	if (is_interrupt) {
		handle_interrupt(tf, cause);
		return;
	}

	handle_exception(tf, cause);
}

void trap_enable_interrupts(void)
{
	csr_set(sstatus, SSTATUS_SIE);
}

void trap_disable_interrupts(void)
{
	csr_clear(sstatus, SSTATUS_SIE);
}

void trap_enable_interrupt_sources(void)
{
	csr_set(sie, SIE_SSIE | SIE_STIE | SIE_SEIE);
}

void trap_disable_interrupt_sources(void)
{
	csr_clear(sie, SIE_SSIE | SIE_STIE | SIE_SEIE);
}

void trap_init(void)
{
	uint64_t stvec_value =
		((uint64_t)trap_entry & ~STVEC_MODE_MASK) | STVEC_MODE_DIRECT;

	trap_disable_interrupts();
	trap_disable_interrupt_sources();

	csr_write(stvec, stvec_value);

	klog("trap: stvec=0x%lx", csr_read(stvec));
}