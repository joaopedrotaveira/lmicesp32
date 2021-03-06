/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

#include "radio.h"
#include "lmic.h"

// ----------------------------------------
// Registers Mapping
#define RegFifo 0x00     // common
#define RegOpMode 0x01   // common
#define RegFrfMsb 0x06   // common
#define RegFrfMid 0x07   // common
#define RegFrfLsb 0x08   // common
#define RegPaConfig 0x09 // common
#define RegPaRamp 0x0A   // common
#define RegOcp 0x0B      // common
#define RegLna 0x0C      // common
#define LORARegFifoAddrPtr 0x0D
#define LORARegFifoTxBaseAddr 0x0E
#define LORARegFifoRxBaseAddr 0x0F
#define LORARegFifoRxCurrentAddr 0x10
#define LORARegIrqFlagsMask 0x11
#define LORARegIrqFlags 0x12
#define LORARegRxNbBytes 0x13
#define LORARegRxHeaderCntValueMsb 0x14
#define LORARegRxHeaderCntValueLsb 0x15
#define LORARegRxPacketCntValueMsb 0x16
#define LORARegRxpacketCntValueLsb 0x17
#define LORARegModemStat 0x18
#define LORARegPktSnrValue 0x19
#define LORARegPktRssiValue 0x1A
#define LORARegRssiValue 0x1B
#define LORARegHopChannel 0x1C
#define LORARegModemConfig1 0x1D
#define LORARegModemConfig2 0x1E
#define LORARegSymbTimeoutLsb 0x1F
#define LORARegPreambleMsb 0x20
#define LORARegPreambleLsb 0x21
#define LORARegPayloadLength 0x22
#define LORARegPayloadMaxLength 0x23
#define LORARegHopPeriod 0x24
#define LORARegFifoRxByteAddr 0x25
#define LORARegModemConfig3 0x26
#define LORARegFeiMsb 0x28
#define LORAFeiMib 0x29
#define LORARegFeiLsb 0x2A
#define LORARegRssiWideband 0x2C
#define LORARegDetectOptimize 0x31
#define LORARegInvertIQ 0x33
#define LORARegDetectionThreshold 0x37
#define LORARegSyncWord 0x39
#define RegDioMapping1 0x40 // common
#define RegDioMapping2 0x41 // common
#define RegVersion 0x42     // common
// #define RegAgcRef                                  0x43 // common
// #define RegAgcThresh1                              0x44 // common
// #define RegAgcThresh2                              0x45 // common
// #define RegAgcThresh3                              0x46 // common
// #define RegPllHop                                  0x4B // common
// #define RegTcxo                                    0x58 // common
#define RegPaDac 0x4D // common
// #define RegPll                                     0x5C // common
// #define RegPllLowPn                                0x5E // common
// #define RegFormerTemp                              0x6C // common
// #define RegBitRateFrac                             0x70 // common

// ----------------------------------------
// spread factors and mode for RegModemConfig2
#define SX1272_MC2_FSK 0x00
#define SX1272_MC2_SF7 0x70
#define SX1272_MC2_SF8 0x80
#define SX1272_MC2_SF9 0x90
#define SX1272_MC2_SF10 0xA0
#define SX1272_MC2_SF11 0xB0
#define SX1272_MC2_SF12 0xC0
// bandwidth for RegModemConfig1
#define SX1272_MC1_BW_125 0x00
#define SX1272_MC1_BW_250 0x40
#define SX1272_MC1_BW_500 0x80
// coding rate for RegModemConfig1
#define SX1272_MC1_CR_4_5 0x08
#define SX1272_MC1_CR_4_6 0x10
#define SX1272_MC1_CR_4_7 0x18
#define SX1272_MC1_CR_4_8 0x20
#define SX1272_MC1_IMPLICIT_HEADER_MODE_ON 0x04 // required for receive
#define SX1272_MC1_RX_PAYLOAD_CRCON 0x02
#define SX1272_MC1_LOW_DATA_RATE_OPTIMIZE 0x01 // mandated for SF11 and SF12
// transmit power configuration for RegPaConfig
#define SX1272_PAC_PA_SELECT_PA_BOOST 0x80
#define SX1272_PAC_PA_SELECT_RFIO_PIN 0x00

// sx1276 RegModemConfig1
#define SX1276_MC1_BW_125 0x70
#define SX1276_MC1_BW_250 0x80
#define SX1276_MC1_BW_500 0x90
#define SX1276_MC1_CR_4_5 0x02
#define SX1276_MC1_CR_4_6 0x04
#define SX1276_MC1_CR_4_7 0x06
#define SX1276_MC1_CR_4_8 0x08

#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON 0x01

// sx1276 RegModemConfig2
#define SX1276_MC2_RX_PAYLOAD_CRCON 0x04

// sx1276 RegModemConfig3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE 0x08
#define SX1276_MC3_AGCAUTO 0x04

// preamble for lora networks (nibbles swapped)
#define LORA_MAC_PREAMBLE 0x34

#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1 0x0A
#ifdef CFG_sx1276_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x70
#elif CFG_sx1272_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x74
#endif

// ----------------------------------------
// Constants for radio registers
#define OPMODE_LORA 0x80
#define OPMODE_MASK 0x07
#define OPMODE_SLEEP 0x00
#define OPMODE_STANDBY 0x01
#define OPMODE_FSTX 0x02
#define OPMODE_TX 0x03
#define OPMODE_FSRX 0x04
#define OPMODE_RX 0x05
#define OPMODE_RX_SINGLE 0x06
#define OPMODE_CAD 0x07

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 0x80
#define IRQ_LORA_RXDONE_MASK 0x40
#define IRQ_LORA_CRCERR_MASK 0x20
#define IRQ_LORA_HEADER_MASK 0x10
#define IRQ_LORA_TXDONE_MASK 0x08
#define IRQ_LORA_CDDONE_MASK 0x04
#define IRQ_LORA_FHSSCH_MASK 0x02
#define IRQ_LORA_CDDETD_MASK 0x01

// ----------------------------------------
// DIO function mappings                D0D1D2D3
#define MAP_DIO0_LORA_RXDONE 0x00 // 00------
#define MAP_DIO0_LORA_TXDONE 0x40 // 01------
#define MAP_DIO1_LORA_RXTOUT 0x00 // --00----
#define MAP_DIO1_LORA_NOP 0x30    // --11----
#define MAP_DIO2_LORA_NOP 0xC0    // ----11--

#define LNA_RX_GAIN (0x20 | 0x03)

static void writeReg(uint8_t addr, uint8_t data) {
  hal_pin_nss(0);
  hal_spi(addr | 0x80);
  hal_spi(data);
  hal_pin_nss(1);

#if LMIC_DEBUG_LEVEL > 2
  hal_pin_nss(0);
  hal_spi(addr & 0x7F);
  uint8_t val = hal_spi(0x00);
  PRINT_DEBUG_3("Reg %x, Write %x, Readback:%x", addr, data, val);
  hal_pin_nss(1);
#endif

}

static uint8_t readReg(uint8_t addr) {
  hal_pin_nss(0);
  hal_spi(addr & 0x7F);
  uint8_t val = hal_spi(0x00);
  PRINT_DEBUG_3("Reg %x, Read:%x", addr, val);
  hal_pin_nss(1);
  return val;
}

static void writeBuf(uint8_t addr, uint8_t *buf, uint8_t len) {
  hal_pin_nss(0);
  hal_spi(addr | 0x80);
  for (uint8_t i = 0; i < len; i++) {
    hal_spi(buf[i]);
  }
  hal_pin_nss(1);
}

static void readBuf(uint8_t addr, uint8_t *buf, uint8_t len) {
  hal_pin_nss(0);
  hal_spi(addr & 0x7F);
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = hal_spi(0x00);
  }
  hal_pin_nss(1);
}

static void opmode(uint8_t mode) {
  writeReg(RegOpMode, (readReg(RegOpMode) & ~OPMODE_MASK) | mode);
}

static void opmodeLora() {
  uint8_t u = OPMODE_LORA;
#ifdef CFG_sx1276_radio
  u |= 0x8; // TBD: sx1276 high freq
#endif
  writeReg(RegOpMode, u);
}

// configure LoRa modem (cfg1, cfg2)
static void configLoraModem(rps_t rps) {
  sf_t sf = rps.sf;

#ifdef CFG_sx1276_radio
  uint8_t mc1 = 0, mc2 = 0, mc3 = 0;

  switch (rps.bw) {
  case BW125:
    mc1 |= SX1276_MC1_BW_125;
    break;
  case BW250:
    mc1 |= SX1276_MC1_BW_250;
    break;
  case BW500:
    mc1 |= SX1276_MC1_BW_500;
    break;
  default:
    ASSERT(0);
  }
  switch (rps.cr) {
  case CR_4_5:
    mc1 |= SX1276_MC1_CR_4_5;
    break;
  case CR_4_6:
    mc1 |= SX1276_MC1_CR_4_6;
    break;
  case CR_4_7:
    mc1 |= SX1276_MC1_CR_4_7;
    break;
  case CR_4_8:
    mc1 |= SX1276_MC1_CR_4_8;
    break;
  default:
    ASSERT(0);
  }

  if (rps.ih) {
    mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
    writeReg(LORARegPayloadLength, rps.ih); // required length
  }
  // set ModemConfig1
  writeReg(LORARegModemConfig1, mc1);

  mc2 = (SX1272_MC2_SF7 + ((sf - 1) << 4));
  if (!rps.nocrc) {
    mc2 |= SX1276_MC2_RX_PAYLOAD_CRCON;
  }
  writeReg(LORARegModemConfig2, mc2);

  mc3 = SX1276_MC3_AGCAUTO;
  if ((sf == SF11 || sf == SF12) && rps.bw == BW125) {
    mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
  }
  writeReg(LORARegModemConfig3, mc3);
#elif CFG_sx1272_radio
  uint8_t mc1 = (rps.bw << 6);

  switch (rps.cr) {
  case CR_4_5:
    mc1 |= SX1272_MC1_CR_4_5;
    break;
  case CR_4_6:
    mc1 |= SX1272_MC1_CR_4_6;
    break;
  case CR_4_7:
    mc1 |= SX1272_MC1_CR_4_7;
    break;
  case CR_4_8:
    mc1 |= SX1272_MC1_CR_4_8;
    break;
  }

  if ((sf == SF11 || sf == SF12) && rps.bw == BW125) {
    mc1 |= SX1272_MC1_LOW_DATA_RATE_OPTIMIZE;
  }

  if (rps.nocrc == 0) {
    mc1 |= SX1272_MC1_RX_PAYLOAD_CRCON;
  }

  if (rps.ih) {
    mc1 |= SX1272_MC1_IMPLICIT_HEADER_MODE_ON;
    writeReg(LORARegPayloadLength, rps.ih); // required length
  }
  // set ModemConfig1
  writeReg(LORARegModemConfig1, mc1);

  // set ModemConfig2 (sf, AgcAutoOn=1 SymbTimeoutHi=00)
  writeReg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf - 1) << 4)) | 0x04);
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

static void configChannel(uint32_t freq) {
  // set frequency: FQ = (FRF * 32 Mhz) / (2 ^ 19)
  uint64_t frf = ((uint64_t)freq << 19) / 32000000;
  writeReg(RegFrfMsb, (uint8_t)(frf >> 16));
  writeReg(RegFrfMid, (uint8_t)(frf >> 8));
  writeReg(RegFrfLsb, (uint8_t)(frf >> 0));
}

static void configPower(int8_t pw) {
#ifdef CFG_sx1276_radio
  // no boost +20dB used for now
  if (pw > 17) {
    pw = 17;
  } else if (pw < 2) {
    pw = 2;
  }
  pw -= 2;
  // check board type for output pin
  // output on PA_BOOST for RFM95W
  writeReg(RegPaConfig, (uint8_t)(0x80 | pw));
  // no boost +20dB
  writeReg(RegPaDac, (readReg(RegPaDac) & 0xF8) | 0x4);

#elif CFG_sx1272_radio
  // set PA config (2-17 dBm using PA_BOOST)
  if (pw > 17) {
    pw = 17;
  } else if (pw < 2) {
    pw = 2;
  }
  writeReg(RegPaConfig, (uint8_t)(0x80 | (pw - 2)));
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

static void txlora(uint32_t freq, rps_t rps, int8_t txpow, uint8_t *frame,
                   uint8_t dataLen) {
  // select LoRa modem (from sleep mode)
  // writeReg(RegOpMode, OPMODE_LORA);
  opmodeLora();
  ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);

  // enter standby mode (required for FIFO loading))
  opmode(OPMODE_STANDBY);
  // configure LoRa modem (cfg1, cfg2)
  configLoraModem(rps);
  // configure frequency
  configChannel(freq);
  // configure output power
  writeReg(RegPaRamp,
           (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
  configPower(txpow);
  // set sync word
  writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

  // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
  writeReg(RegDioMapping1,
           MAP_DIO0_LORA_TXDONE | MAP_DIO1_LORA_NOP | MAP_DIO2_LORA_NOP);
  // clear all radio IRQ flags
  writeReg(LORARegIrqFlags, 0xFF);
  // mask all IRQs but TxDone
  writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

  // initialize the payload size and address pointers
  writeReg(LORARegFifoTxBaseAddr, 0x00);
  writeReg(LORARegFifoAddrPtr, 0x00);
  writeReg(LORARegPayloadLength, dataLen);

  // download buffer to the radio FIFO
  writeBuf(RegFifo, frame, dataLen);

  // enable antenna switch for TX
  hal_pin_rxtx(1);

  // now we actually start the transmission
  opmode(OPMODE_TX);
  hal_forbid_sleep();

#if LMIC_DEBUG_LEVEL > 0
  uint8_t sf = rps.sf + 6; // 1 == SF7
  uint8_t bw = rps.bw;
  uint8_t cr = rps.cr;
  lmic_printf("%u: TXMODE, freq=%u, len=%d, SF=%d, BW=%d, CR=4/%d, IH=%d\n",
              os_getTime().tick(), freq, dataLen, sf,
              bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
              cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)),
              rps.ih);
#endif
}

// start transmitter
static void starttx(uint32_t freq, rps_t rps, int8_t txpow, uint8_t *frame,
                    uint8_t dataLen) {
  ASSERT((readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP);
  txlora(freq, rps, txpow, frame, dataLen);
  // the radio will go back to STANDBY mode as soon as the TX is finished
  // the corresponding IRQ will inform us about completion.
}

enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };

static CONST_TABLE(uint8_t, rxlorairqmask)[] = {
    [RXMODE_SINGLE] = IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK,
    [RXMODE_SCAN] = IRQ_LORA_RXDONE_MASK,
    [RXMODE_RSSI] = 0x00,
};

// start LoRa receiver
static void rxlora(uint8_t rxmode, uint32_t freq, rps_t rps, uint8_t rxsyms,
                   OsTime const &rxtime) {
  // select LoRa modem (from sleep mode)
  opmodeLora();
  ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);
  // enter standby mode (warm up))
  opmode(OPMODE_STANDBY);
  // don't use MAC settings at startup
  if (rxmode == RXMODE_RSSI) { // use fixed settings for rssi scan
    writeReg(LORARegModemConfig1, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
    writeReg(LORARegModemConfig2, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
  } else { // single or continuous rx mode
    // configure LoRa modem (cfg1, cfg2)
    configLoraModem(rps);
    // configure frequency
    configChannel(freq);
  }
  // set LNA gain
  writeReg(RegLna, LNA_RX_GAIN);
  // set max payload size
  writeReg(LORARegPayloadMaxLength, 64);
#if !defined(DISABLE_INVERT_IQ_ON_RX)
  // use inverted I/Q signal (prevent mote-to-mote communication)
  writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ) | (1 << 6));
#endif
  // set symbol timeout (for single rx)
  writeReg(LORARegSymbTimeoutLsb, rxsyms);
  // set sync word
  writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

  // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
  writeReg(RegDioMapping1,
           MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT | MAP_DIO2_LORA_NOP);
  // clear all radio IRQ flags
  writeReg(LORARegIrqFlags, 0xFF);
  // enable required radio IRQs
  writeReg(LORARegIrqFlagsMask, ~TABLE_GET_U1(rxlorairqmask, rxmode));

  // enable antenna switch for RX
  hal_pin_rxtx(0);

  // now instruct the radio to receive
  if (rxmode == RXMODE_SINGLE) { // single rx
    hal_waitUntil(rxtime);       // busy wait until exact rx time
    opmode(OPMODE_RX_SINGLE);
  } else { // continous rx (scan or rssi)
    opmode(OPMODE_RX);
  }
  hal_forbid_sleep();

#if LMIC_DEBUG_LEVEL > 0
  if (rxmode == RXMODE_RSSI) {
    lmic_printf("RXMODE_RSSI\n");
  } else {
    uint8_t sf = rps.sf + 6; // 1 == SF7
    uint8_t bw = rps.bw;
    uint8_t cr = rps.cr;
    lmic_printf(
        "%u: %s, freq=%u, SF=%d, BW=%d, CR=4/%d, IH=%d\n", os_getTime().tick(),
        rxmode == RXMODE_SINGLE
            ? "RXMODE_SINGLE"
            : (rxmode == RXMODE_SCAN ? "RXMODE_SCAN" : "UNKNOWN_RX"),
        freq, sf, bw == BW125 ? 125 : (bw == BW250 ? 250 : 500),
        cr == CR_4_5 ? 5 : (cr == CR_4_6 ? 6 : (cr == CR_4_7 ? 7 : 8)), rps.ih);
  }
#endif
}

static void startrx(uint8_t rxmode, uint32_t freq, rps_t rps, uint8_t rxsyms,
                    OsTime const &rxtime) {
  ASSERT((readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP);
  rxlora(rxmode, freq, rps, rxsyms, rxtime);
  // the radio will go back to STANDBY mode as soon as the RX is finished
  // or timed out, and the corresponding IRQ will inform us about completion.
}

void Radio::init() {
  hal_disableIRQs();

  // manually reset radio
#ifdef CFG_sx1276_radio
  hal_pin_rst(0); // drive RST pin low
#else
  hal_pin_rst(1); // drive RST pin high
#endif
  // wait >100us
  hal_wait(OsDeltaTime::from_ms(1));
  hal_pin_rst(2); // configure RST pin floating!
  // wait 5ms
  hal_wait(OsDeltaTime::from_ms(5));

  opmode(OPMODE_SLEEP);

#if !defined(CFG_noassert) || LMIC_DEBUG_LEVEL > 0
  // some sanity checks, e.g., read version number
  uint8_t v = readReg(RegVersion);

  PRINT_DEBUG_1("Chip version : %x", v);

#endif
#ifdef CFG_sx1276_radio
  ASSERT(v == 0x12);
#elif CFG_sx1272_radio
  ASSERT(v == 0x22);
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif

  opmode(OPMODE_SLEEP);
  hal_allow_sleep();

  hal_enableIRQs();
}

uint8_t Radio::rssi() {
  hal_disableIRQs();
  uint8_t r = readReg(LORARegRssiValue);
  hal_enableIRQs();
  return r;
}

static CONST_TABLE(int32_t, LORA_RXDONE_FIXUP)[] = {
    [FSK] = us2osticks(0), // (   0 ticks)
    [SF7] = us2osticks(0), // (   0 ticks)
    [SF8] = us2osticks(1648),   [SF9] = us2osticks(3265),
    [SF10] = us2osticks(7049),  [SF11] = us2osticks(13641),
    [SF12] = us2osticks(31189),
};

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
void Radio::irq_handler(OsJobBase &nextJob, uint8_t dio,
                        OsTime const &trigger) {
  OsTime now = os_getTime();
  if (now - trigger < OsDeltaTime::from_sec(1)) {
    now = trigger;
  } else {
    PRINT_DEBUG_1("Not using interupt trigger %u", trigger.tick());
  }

  if ((readReg(RegOpMode) & OPMODE_LORA) != 0) { // LORA modem
    uint8_t flags = readReg(LORARegIrqFlags);

    PRINT_DEBUG_2("irq: dio: 0x%x flags: 0x%x", dio, flags);

    if (flags & IRQ_LORA_TXDONE_MASK) {
      // save exact tx time
      PRINT_DEBUG_1("End TX  %i", txEnd.tick());
      txEnd = now; // - OsDeltaTime::from_us(43); // TXDONE FIXUP
      // forbid sleep to keep precise time counting.
      // hal_forbid_sleep();
      hal_allow_sleep();

    } else if (flags & IRQ_LORA_RXDONE_MASK) {
      // save exact rx time
      if (currentRps.bw == BW125) {
        now -= OsDeltaTime(TABLE_GET_S4(LORA_RXDONE_FIXUP, currentRps.sf));
      }
      PRINT_DEBUG_1("End RX -  Start RX : %i us ", (now - rxTime).to_us());
      rxTime = now;

      // read the PDU and inform the MAC that we received something
      uint8_t length =
          (readReg(LORARegModemConfig1) & SX1272_MC1_IMPLICIT_HEADER_MODE_ON)
              ? readReg(LORARegPayloadLength)
              : readReg(LORARegRxNbBytes);

      // for security clamp length of data
      length = length < MAX_LEN_FRAME ? length : MAX_LEN_FRAME;

      // set FIFO read address pointer
      writeReg(LORARegFifoAddrPtr, readReg(LORARegFifoRxCurrentAddr));
      // now read the FIFO
      readBuf(RegFifo, framePtr, length);
      frameLength = length;

      // read rx quality parameters
      // TODO restore
      // LMIC.snr = readReg(LORARegPktSnrValue); // SNR [dB] * 4
      // LMIC.rssi =
      //    readReg(LORARegPktRssiValue) - 125 + 64; // RSSI [dBm] (-196...+63)
      hal_allow_sleep();
    } else if (flags & IRQ_LORA_RXTOUT_MASK) {
      PRINT_DEBUG_1("RX timeout  %i", now.tick());

      // indicate timeout
      frameLength = 0;
      hal_allow_sleep();
    }
    // mask all radio IRQs
    writeReg(LORARegIrqFlagsMask, 0xFF);
    // clear radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
  }
  // go from stanby to sleep
  opmode(OPMODE_SLEEP);
  // run os job (use preset func ptr)
  nextJob.setRunnable();
}

void Radio::rst() {
  hal_disableIRQs();
  // put radio to sleep
  opmode(OPMODE_SLEEP);
  hal_allow_sleep();
  hal_enableIRQs();
}

void Radio::tx(uint32_t freq, rps_t rps, int8_t txpow) {
  hal_disableIRQs();
  // transmit frame now
  starttx(freq, rps, txpow, framePtr, frameLength);
  hal_enableIRQs();
}

void Radio::rx(uint32_t freq, rps_t rps, uint8_t rxsyms, OsTime const &rxtime) {
  hal_disableIRQs();
  currentRps = rps;
  // receive frame now (exactly at rxtime)
  startrx(RXMODE_SINGLE, freq, rps, rxsyms, rxtime);
  hal_enableIRQs();
}

void Radio::rxon(uint32_t freq, rps_t rps, uint8_t rxsyms,
                 OsTime const &rxtime) {
  hal_disableIRQs();
  currentRps = rps;
  // start scanning for beacon now
  startrx(RXMODE_SCAN, freq, rps, rxsyms, rxtime);
  hal_enableIRQs();
}

Radio::Radio(uint8_t *frame, uint8_t &framLength, OsTime &reftxEnd,
             OsTime &refrxTime)
    : framePtr(frame), frameLength(framLength), txEnd(reftxEnd),
      rxTime(refrxTime) {}
