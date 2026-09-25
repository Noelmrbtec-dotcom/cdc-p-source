# cdc-p-source
Fontes do livro CDC-P Um Executivo Determinístico Autônomo Fundamentos, Implementação e Aplicações
# CDC-P — Código-fonte

Este repositório contém o código-fonte completo do método **CDC-P** 
(Concurrency Deterministic Control with Preemption), descrito no livro:

> **CDC-P: Um Executivo Determinístico Autônomo**  
> Fundamentos, Implementação e Aplicações  
> Marcos Roberto Braga — 2024 (atualizado em 2026)

## 📚 Sobre o método

O CDC-P é um executivo determinístico autônomo para sistemas de tempo real 
em microcontroladores. Foi desenvolvido ao longo de 25 anos e testado em 
quatro aplicações reais: automação residencial, robótica de competição, 
instrumentação de precisão e RTOS determinístico.

A tese central é que **tempo real não é uma abstração que o software mantém, 
mas um fenômeno físico que o hardware gera**.

## 📂 Estrutura

- [`pic16f628a/`](pic16f628a/) — Implementação de referência para PIC16F628A (CCS C)
- [`ch32v003/`](ch32v003/) — Implementação moderna para CH32V003 (RISC-V, GCC)
- [`docs/`](docs/) — Documentação e diagramas

## 🚀 Como usar

### PIC16F628A
1. Abra o projeto no **CCS C Compiler**
2. Configure o MCU para 4 MHz
3. Compile e grave via **PICkit** ou gravador compatível

### CH32V003
1. Abra o projeto no **MounRiver Studio** ou **GCC + Makefile**
2. Compile com `make`
3. Grave via **WCH-Link** ou gravador compatível

## 📖 Documentação

O código é explicado em detalhe no livro. Os capítulos mais relevantes são:

- **Cap. 5–12** — Fundamentos (tarefas atômicas, despachador, semáforos, etc.)
- **Cap. 13–17** — Supervisão e autonomia
- **Cap. 18–22** — Implementação
- **Apêndice A** — Código completo para PIC16F628A
- **Apêndice B** — Código completo para CH32V003

## 📖 Sobre o livro

Este código é parte do livro **CDC-P: Um Executivo Determinístico Autônomo**.

📄 PDF LIVRO-CDC-P: [[link da Hotmart](https://go.hotmart.com/K107748750A)]
💻 LinkedIn: [[link do post](https://lnkd.in/p/dEzXg9fM)]

## 📝 Licença

Este código é disponibilizado sob a licença [MIT].

Consulte o arquivo [`LICENSE`](LICENSE) para mais detalhes.

## 📬 Contato

- **Autor:** Marcos Roberto Braga
- **E-mail:** [noelmrb_tec@yahoo.com]

---

*"Tempo real não é uma abstração que o software mantém.*  
*É um fenômeno físico que o hardware gera.*  
*O software apenas coordena."*
