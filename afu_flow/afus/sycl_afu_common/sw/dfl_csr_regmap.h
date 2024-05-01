#ifndef __DFL_CSR_REGMAP_H_
#define __DFL_CSR_REGMAP_H_

// Address adjust for direct MMIO access after fpgaMapMMIO()
#define MAPPED_MMIO(mmio_ptr, addr) (mmio_ptr[(addr) >> 3])

// Address space of dfl_csr_avalon_proxy
#define AFU_DFH_REG     0x0     // Read-valid, write-ignored
#define AFU_ID_LO       0x8     // Read-valid, write-ignored
#define AFU_ID_HI       0x10    // Read-valid, write-ignored
#define AFU_NEXT        0x18    // Read-valid, write-ignored
#define AFU_RESERVED    0x20    // Read-valid, write-ignored
#define AFU_RESET       0x28    // Read-zero, write-valid
#define AFU_IRQ_EN      0x30    // Read-zero, write-valid

// Masks
#define AFU_RESET_MASK  ((uint64_t)0x1)
#define AFU_IRQ_EN_MASK ((uint64_t)0x1)

// Values
#define AFU_RESET_VALUE  ((uint64_t)0x1)
#define AFU_IRQ_EN_VALUE ((uint64_t)0x1)

#endif // __DFL_CSR_REGMAP_H_
