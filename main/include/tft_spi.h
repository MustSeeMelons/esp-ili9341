#ifndef TFT_SPI_H_
#define TFT_SPI_H_

#include <stdint.h>

#define SPI0_BASE 0x3FF43000
#define SPI1_BASE 0x3FF42000
#define SPI2_BASE 0x3FF64000
#define SPI3_BASE 0x3FF65000

typedef struct {
    volatile uint32_t SPI_CMD_REG;       // Trigger operation: SPI_USR = 18
    volatile uint32_t SPI_ADDR_REG;      // Address for address phase
    volatile uint32_t SPI_CTRL_REG;      // Control bits for command, address, dummy, mosi, miso
    volatile uint32_t SPI_CTRL1_REG;     // Reserved
    volatile uint32_t SPI_RD_STATUS_REG; // Reserved
    volatile uint32_t SPI_CTRL2_REG;     // Reserved
    volatile uint32_t SPI_CLOCK_REG;
    volatile uint32_t SPI_USER_REG;
    volatile uint32_t SPI_USER1_REG;
    volatile uint32_t SPI_USER2_REG;
    volatile uint32_t SPI_MOSI_DLEN_REG;
    volatile uint32_t SPI_MISO_DLEN_REG;
    volatile uint32_t SPI_SLV_WR_STATUS_REG;
    volatile uint32_t SPI_PIN_REG;
    volatile uint32_t SPI_SLAVE_REG;
    volatile uint32_t SPI_SLAVE1_REG;
    volatile uint32_t SPI_SLAVE2_REG;
    volatile uint32_t SPI_SLAVE3_REG; // Reserved
    volatile uint32_t SPI_SLV_WRBUF_DLEN_REG;
    volatile uint32_t SPI_SLV_RDBUF_DLEN_REG;
    volatile uint32_t SPI_SLV_RD_BIT_REG;
    volatile uint32_t SPI_W0_REG;
    volatile uint32_t SPI_W1_REG;
    volatile uint32_t SPI_W2_REG;
    volatile uint32_t SPI_W3_REG;
    volatile uint32_t SPI_W4_REG;
    volatile uint32_t SPI_W5_REG;
    volatile uint32_t SPI_W6_REG;
    volatile uint32_t SPI_W7_REG;
    volatile uint32_t SPI_W8_REG;
    volatile uint32_t SPI_W9_REG;
    volatile uint32_t SPI_W10_REG;
    volatile uint32_t SPI_W11_REG;
    volatile uint32_t SPI_W12_REG;
    volatile uint32_t SPI_W13_REG;
    volatile uint32_t SPI_W14_REG;
    volatile uint32_t SPI_W15_REG;
    volatile uint32_t SPI_TX_CRC_REG;
    volatile uint32_t SPI_EXT2_REG; // SPI_ST - current state of SPI
    volatile uint32_t SPI_DMA_CONF_REG;
    volatile uint32_t SPI_DMA_OUT_LINK_REG;
    volatile uint32_t SPI_DMA_IN_LINK_REG;
    volatile uint32_t SPI_DMA_STATUS_REG;
    volatile uint32_t SPI_DMA_INT_ENA_REG;
    volatile uint32_t SPI_DMA_INT_RAW_RE;
    volatile uint32_t SPI_DMA_INT_ST_REG;
    volatile uint32_t SPI_DMA_INT_CLR_REG;
    volatile uint32_t SPI_IN_ERR_EOF_DES_ADDR_REG;
    volatile uint32_t SPI_IN_SUC_EOF_DES_ADDR_REG;
    volatile uint32_t SPI_INLINK_DSCR_REG;
    volatile uint32_t SPI_INLINK_DSCR_BF0_REG;
    volatile uint32_t SPI_INLINK_DSCR_BF1_REG;
    volatile uint32_t SPI_OUT_EOF_BFR_DES_ADDR_REG;
    volatile uint32_t SPI_OUT_EOF_DES_ADDR_REG;
    volatile uint32_t SPI_OUTLINK_DSCR_REG;
    volatile uint32_t SPI_OUTLINK_DSCR_BF0_REG;
    volatile uint32_t SPI_OUTLINK_DSCR_BF1_REG;
    volatile uint32_t SPI_DMA_RSTATUS_REG;
    volatile uint32_t SPI_DMA_TSTATUS_REG;
} SPI_t;

#define TFT_SPI2 ((SPI_t *)SPI2_BASE)

// TODO use SPI2 or SPI3 with IO_MUX (no GPIO Matrix!)
// CS0 15/5
// SCLK 14/18
// MISO 12/19
// MOSI 13/23

// Data lengths in: SPI_MISO_DLEN_REG, SPI_MOSI_DLEN_REG
// Command + address + dummy  + send/receive

// All phases controlled by
// SPI_USR_COMMAND, SPI_USR_ADDR, SPI_USR_DUMMY, SPI_USR_MISO/SPI_USR_MOSI, SPI_USER_REG

// Three line mode: SPI_SIO bit in SPI_USER_REG

// Data Buffer + SPI_USR_MOSI
// SPI_W0_REG -=> SPI_W7_REG (LOW)
// SPI_W8_REG -=> SPI_W15_REG (HIGH)

// Clock: Fapb/2 is max output clock
// SPI_CLOCK_RE (SPI_CLKCNT_N and SPI_CLKDIV_PRE)
// PI_CLK_EQU_SYSCLK needs to be 0

// Clock Polarity (CPOL) and Clock Phase (CPHA)

// Pins: IO_MUX or IO_MUX and GPIO matrix (Matrix is slower!)

#include "esp_err.h"

esp_err_t spi_init();

#endif