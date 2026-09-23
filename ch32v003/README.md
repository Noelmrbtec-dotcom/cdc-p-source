# CDC-P — Implementação para CH32V003

Implementação moderna do CDC-P para o microcontrolador **CH32V003** 
(RISC-V RV32EC, 48 MHz), usando o compilador **GCC**.

## 📋 Requisitos

- **MCU:** CH32V003F4P6 (ou compatível)
- **Frequência:** 48 MHz
- **Compilador:** RISC-V GCC (MounRiver Studio ou toolchain oficial)
- **Gravador:** WCH-Link ou compatível

## 📂 Arquivos

| Arquivo | Descrição |
|---------|-----------|
| `main.c` | Código principal (despachador, tarefas, ISRs) |
| `main.h` | Defines e configurações gerais |
| `hardware.h` | Mapeamento de pinos e configuração de hardware |
| `CH32V003_IO_V2.h` | Biblioteca de I/O |
| `CH32V003_USART_V4.h` | Biblioteca de USART (com DMA + IDLE) |
| `CH32V003_SYSTICK_V3.h` | Biblioteca de SysTick |
| `CH32V003_ADC_V3.h` | Biblioteca de ADC |
| `CH32V003_DMA_V3.h` | Biblioteca de DMA |

## 🚀 Como compilar

### Opção 1 — MounRiver Studio
1. Abra o **MounRiver Studio**
2. Importe o projeto
3. Compile (`Ctrl + B`)
4. Grave via **WCH-Link**

### Opção 2 — GCC + Makefile
```bash
make
make flash
