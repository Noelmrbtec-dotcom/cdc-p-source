# CDC-P — Implementação para PIC16F628A

Implementação de referência do CDC-P para o microcontrolador **PIC16F628A** 
(8 bits, 4 MHz), usando o compilador **CCS C**.

## 📋 Requisitos

- **MCU:** PIC16F628A (ou compatível: PIC16F627A, PIC16F648A)
- **Frequência:** 4 MHz
- **Compilador:** CCS C Compiler (versão 4.x ou superior)
- **Gravador:** PICkit 2/3/4 ou compatível

## 📂 Arquivos

| Arquivo | Descrição |
|---------|-----------|
| `main.c` | Código principal (despachador, tarefas, ISRs) |
| `main.h` | Defines e configurações gerais |
| `hardware.h` | Mapeamento de pinos e configuração de hardware |

## 🚀 Como compilar

1. Abra o **CCS C Compiler**
2. Crie um novo projeto para o **PIC16F628A**
3. Adicione os arquivos `main.c`, `main.h` e `hardware.h`
4. Configure o clock para **4 MHz** (cristal externo)
5. Compile (`F9` ou `Compile`)
6. Grave o `.hex` gerado no MCU

## 📖 Documentação

O código é explicado em detalhe no **Apêndice A** do livro.

## 📝 Licença

MIT — consulte o arquivo `LICENSE` na raiz do repositório.
