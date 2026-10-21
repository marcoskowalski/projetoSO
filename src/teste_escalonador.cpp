// Teste do Escalonador (RM e EDF) com os critérios de desempate.
#include <iostream>

#include "../headers/Escalonador.h"
#include "../headers/EscalonadorRM.h"
#include "../headers/EscalonadorEDF.h"

static int falhas = 0;
#define VERIFICA(cond)                                                     \
    do {                                                                   \
        if (!(cond)) { std::cerr << "FALHOU (linha " << __LINE__ << "): " #cond "\n"; falhas++; } \
    } while (0)

// Cria um TCB já PRONTO, ativado no instante 'ingresso'.
static TCB criar(int id, int periodo, int ingresso, int duracao, int prazo)
{
    TCB t(Tarefa(id, periodo, ingresso, duracao, prazo, 0xFF0000));
    t.novaAtivacao(ingresso);
    return t;
}

int main()
{
    // 1. RM: menor período vence, mesmo chegando depois.
    {
        TCB a = criar(1, 10, 0, 2, 10), b = criar(2, 5, 0, 2, 5), c = criar(3, 20, 0, 2, 20);
        EscalonadorRM rm;
        auto r = rm.escolher({&a, &b, &c}, 1);
        VERIFICA(r.escolhidas.size() == 1 && r.escolhidas[0]->getTarefa().getId() == 2);
        // Com 2 CPUs, as duas mais prioritárias, em ordem.
        r = rm.escolher({&a, &b, &c}, 2);
        VERIFICA(r.escolhidas.size() == 2 && r.escolhidas[0]->getTarefa().getId() == 2
                 && r.escolhidas[1]->getTarefa().getId() == 1);
        VERIFICA(r.sorteadas.empty());
    }
    // 2. EDF: menor prazo ABSOLUTO vence (período maior, mas prazo mais perto).
    {
        TCB a = criar(1, 5, 0, 2, 5);     // prazo absoluto 5
        TCB b = criar(2, 20, 0, 2, 3);    // prazo absoluto 3
        EscalonadorEDF edf;
        auto r = edf.escolher({&a, &b}, 1);
        VERIFICA(r.escolhidas[0]->getTarefa().getId() == 2);
    }
    // 3. Mais CPUs que tarefas: todas são escolhidas (nenhuma fica de fora).
    {
        TCB a = criar(1, 5, 0, 2, 5), b = criar(2, 6, 0, 2, 6);
        EscalonadorEDF edf;
        VERIFICA(edf.escolher({&a, &b}, 4).escolhidas.size() == 2);
        VERIFICA(edf.escolher({}, 2).escolhidas.empty());
    }
    // 4. Desempate (1): empatadas no algoritmo, quem estava executando fica.
    {
        TCB a = criar(1, 10, 0, 2, 10), b = criar(2, 10, 0, 2, 10);
        b.setEstado(EstadoTarefa::executando);
        EscalonadorRM rm;
        for (int i = 0; i < 20; i++)   // várias vezes: não pode depender da sorte
            VERIFICA(rm.escolher({&a, &b}, 1).escolhidas[0]->getTarefa().getId() == 2);
    }
    // 5. Desempate (2): mesmo período no RM, vence o menor prazo absoluto.
    {
        TCB a = criar(1, 10, 0, 2, 10);   // prazo abs 10
        TCB b = criar(2, 10, 0, 2, 6);    // prazo abs 6
        EscalonadorRM rm;
        VERIFICA(rm.escolher({&a, &b}, 1).escolhidas[0]->getTarefa().getId() == 2);
    }
    // 6. Desempate (3): mesmo período e prazo absoluto; chegou antes vence.
    {
        TCB a = criar(1, 10, 4, 2, 6);    // ativada em 4, prazo abs 10
        TCB b = criar(2, 10, 2, 2, 8);    // ativada em 2, prazo abs 10
        EscalonadorRM rm;
        VERIFICA(rm.escolher({&a, &b}, 1).escolhidas[0]->getTarefa().getId() == 2);
    }
    // 7. Desempate (4): empate até o ingresso; menor duração vence.
    {
        TCB a = criar(1, 10, 0, 4, 10), b = criar(2, 10, 0, 2, 10);
        EscalonadorRM rm;
        VERIFICA(rm.escolher({&a, &b}, 1).escolhidas[0]->getTarefa().getId() == 2);
    }
    // 8. Desempate (5): empate total -> sorteio, e ele é reportado.
    {
        TCB a = criar(1, 10, 0, 2, 10), b = criar(2, 10, 0, 2, 10);
        EscalonadorRM rm(42);
        int venceu1 = 0, venceu2 = 0;
        for (int i = 0; i < 200; i++)
        {
            auto r = rm.escolher({&a, &b}, 1);
            VERIFICA(r.sorteadas.size() == 1);
            VERIFICA(r.sorteadas[0] == r.escolhidas[0]->getTarefa().getId());
            (r.escolhidas[0]->getTarefa().getId() == 1 ? venceu1 : venceu2)++;
        }
        VERIFICA(venceu1 > 0 && venceu2 > 0);   // as duas já venceram alguma vez
    }
    // 9. Empate total, mas todas cabem nas CPUs: não houve sorteio de verdade.
    {
        TCB a = criar(1, 10, 0, 2, 10), b = criar(2, 10, 0, 2, 10);
        EscalonadorRM rm;
        VERIFICA(rm.escolher({&a, &b}, 2).sorteadas.empty());
    }
    // 10. Fábrica de escalonadores.
    {
        VERIFICA(criarEscalonador("rm") && criarEscalonador("rm")->nome() == "RM");
        VERIFICA(criarEscalonador("Edf") && criarEscalonador("Edf")->nome() == "EDF");
        VERIFICA(criarEscalonador("XYZ") == nullptr);
    }

    std::cout << (falhas == 0 ? "Todos os testes passaram.\n" : "Houve falhas.\n");
    return falhas == 0 ? 0 : 1;
}
