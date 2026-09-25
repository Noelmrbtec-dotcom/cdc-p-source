# Documentação — CDC-P

Esta pasta contém documentação complementar do método **CDC-P** 
(Concurrency Deterministic Control with Preemption), descrito no livro:

> **CDC-P: Um Executivo Determinístico Autônomo**  
> Fundamentos, Implementação e Aplicações  
> Marcos Roberto Braga — 2024 (atualizado em 2026)

---

## 📂 Conteúdo

Esta pasta reúne material de apoio ao código-fonte disponível nas pastas 
[`pic16f628a/`](../pic16f628a/) e [`ch32v003/`](../ch32v003/).

### Previsão de conteúdo

| Arquivo | Descrição |
|---------|-----------|
| `diagrama-metodo.pdf` | Diagrama geral do método CDC-P (tese + fundamentos + supervisão) |
| `diagrama-relogio.pdf` | O relógio com bisturi que ilustra a capa do livro |
| `diagrama-sete-camadas.pdf` | Diagrama das sete camadas de proteção |
| `diagrama-despachador.pdf` | Fluxo do despachador de ordem fixa |
| `timing.pdf` | Diagrama de tempo de uma tarefa periódica |
| `apresentacao.pdf` | Slides de apresentação do método (opcional) |

*Os arquivos serão adicionados conforme forem sendo produzidos.*

---

## 📐 Sobre os diagramas

Os diagramas documentam visualmente os conceitos descritos no livro. 
Eles complementam os capítulos teóricos e ajudam a compreender a 
arquitetura do método.

### Diagrama geral

Mostra a tese central (*"tempo real é físico, não abstrato"*) e como ela 
se desdobra nos oito fundamentos:

1. Tarefas atômicas
2. Despachador de ordem fixa
3. Semáforo por bit
4. Bases temporais separadas
5. Prioridade tridimensional
6. Aritmética modular
7. URG-S (urgência situacional)
8. Preempção cooperativa

### Diagrama das sete camadas

Ilustra a hierarquia de proteção do sistema:

1. Auto-regulagem temporal
2. Bloqueio por violação de *deadline*
3. Diagnóstico nativo (Xerife)
4. Recuperação progressiva (Síndico)
5. *Fail-safe* final (*reset_cpu*)
6. Aceleração automática (preempção)
7. URG-S (urgência situacional)

### Diagrama do despachador

Representa o fluxo do despachador de ordem fixa com quatro níveis de 
prioridade (URG-S, preempção, kernel, aplicação).

---

## 🎨 Como os diagramas foram gerados

Os diagramas originais foram produzidos em **TikZ** (dentro do LaTeX) e 
exportados como pdf para uso nesta documentação.

---

## 📖 Referências no livro

Os diagramas desta pasta ilustram conceitos dos seguintes capítulos:

| Diagrama | Capítulo do livro |
|----------|-------------------|
| Diagrama geral | Cap. 1–4 (Filosofia e Contexto) |
| Despachador | Cap. 6 (Despachador de ordem fixa) |
| Prioridade tridimensional | Cap. 9 (Prioridade tridimensional) |
| Sete camadas | Cap. 16 (As sete camadas de proteção) |
| Relógio | Capa e Cap. 2 (Tempo real é físico) |

---

## 🤝 Como contribuir

Se você encontrar erros ou tiver sugestões de melhoria na documentação, 
abra uma **issue** no repositório ou envie um **pull request**.

Contribuições são bem-vindas, especialmente:

- Correções de erros conceituais
- Sugestões de diagramas adicionais
- Melhorias na clareza das explicações

---

## 📝 Licença

A documentação é disponibilizada sob a licença **MIT**, a mesma do 
código-fonte. Consulte o arquivo [`LICENSE`](../LICENSE) na raiz do 
repositório para mais detalhes.

---

## 📬 Contato

- **Autor:** Marcos Roberto Braga
- **E-mail:** [noelmrb_tec@yahoo.com]

---

*"O CDC-P não inventou nada. Organizou o que já existia."*
