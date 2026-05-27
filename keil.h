
#ifndef KEIL_H
#define KEIL_H

#define PINSEL0     (*(REG32 (0xE002C000)))
#define PINSEL1     (*(REG32 (0xE002C004)))
#define PINSEL2     (*(REG32 (0xE002C014)))

#define IOSET0     (*(REG32 (0xE0028004)))
#define IODIR0     (*(REG32 (0xE0028008)))
#define IOCLR0     (*(REG32 (0xE002800C)))

#define S0SPCR        (*(REG32 (0xE0020000)))			
#define S0SPSR        (*(REG32 (0xE0020004)))
#define S0SPDR        (*(REG32 (0xE0020008)))
#define S0SPCCR       (*(REG32 (0xE002000C)))
#define S0SPTCR       (*(REG32 (0xE0020010)))
#define S0SPTSR       (*(REG32 (0xE0020014)))
#define S0SPTOR       (*(REG32 (0xE0020018)))
#define S0SPINT       (*(REG32 (0xE002001C)))

// ---- Timer 0 ------------------------------------- 
#define T0IR           (*(REG32 (0xE0004000)))
#define T0TCR          (*(REG32 (0xE0004004)))
#define T0TC           (*(REG32 (0xE0004008)))
#define T0PR           (*(REG32 (0xE000400C)))
#define T0PC           (*(REG32 (0xE0004010)))
#define T0MCR          (*(REG32 (0xE0004014)))
#define T0MR0          (*(REG32 (0xE0004018)))
#define T0MR1          (*(REG32 (0xE000401C)))
#define T0MR2          (*(REG32 (0xE0004020)))
#define T0MR3          (*(REG32 (0xE0004024)))
#define T0CCR          (*(REG32 (0xE0004028)))
#define T0CR0          (*(REG32 (0xE000402C)))
#define T0CR1          (*(REG32 (0xE0004030)))
#define T0CR2          (*(REG32 (0xE0004034)))
#define T0CR3          (*(REG32 (0xE0004038)))
#define T0EMR          (*(REG32 (0xE000403C)))

#define VPBDIV      (*(REG32 (0xE01FC100)))

#endif
