// Testes automatizados do núcleo do simulador (sem biblioteca externa).
// Cada CHECK imprime a falha com o número da linha; o programa retorna 0 se
// tudo passou e 1 caso contrário (assim o "ctest" enxerga o resultado).
#include <filesystem>
#include <fstream>
#include <iostream>

#include "LeitorConfig.h"
#include "Simulador.h"

namespace
{
    int falhas = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cout << "FALHOU (linha " << __LINE__ << "): " #cond "\n";       \
            falhas++;                                                            \
        }                                                                        \
    } while (0)

    Configuracao cfg(const std::string& algoritmo, int cpus, int quantum = 1)
    {
        return Configuracao(quantum, cpus, algoritmo);
    }

    // id, periodo, ingresso, duracao, prazo
    Tarefa tarefa(int id, int periodo, int ingresso, int duracao, int prazo)
    {
        return Tarefa(id, periodo, ingresso, duracao, prazo, 0x808080);
    }

    int contar(const Simulador& s, TipoEvento tipo, int id = -2)
    {
        int n = 0;
        for (const Evento& e : s.eventos())
            if (e.tipo == tipo && (id == -2 || e.idTarefa == id)) n++;
        return n;
    }

    int ticksExecutados(const Simulador& s, int id)
    {
        int n = 0;
        for (const auto& linha : s.traco())
            for (int quem : linha)
                if (quem == id) n++;
        return n;
    }

    std::string escreverArquivo(const std::string& nome, const std::string& texto)
    {
        const std::string caminho = (std::filesystem::temp_directory_path() / nome).string();
        std::ofstream(caminho) << texto;
        return caminho;
    }

    // RM falha e EDF não, no mesmo conjunto (U = 2/5 + 4/7 = 0,97 > limite do RM).
    void testeRmContraEdf()
    {
        std::vector<Tarefa> ts = {tarefa(1, 5, 0, 2, 5), tarefa(2, 7, 0, 4, 7)};

        Simulador rm(cfg("RM", 1), ts, 1);
        CHECK(rm.executarCompleto());
        CHECK(contar(rm, TipoEvento::perdaPrazo, 1) == 0);
        CHECK(contar(rm, TipoEvento::perdaPrazo, 2) >= 1);   // T2 estoura o prazo no instante 7
        // Perder o prazo não encerra a tarefa: ela executa as 10 vezes completas.
        CHECK(ticksExecutados(rm, 1) == 10 * 2);
        CHECK(ticksExecutados(rm, 2) == 10 * 4);
        CHECK(contar(rm, TipoEvento::termino, 2) == 10);

        Simulador edf(cfg("EDF", 1), ts, 1);
        CHECK(edf.executarCompleto());
        CHECK(contar(edf, TipoEvento::perdaPrazo) == 0);
        CHECK(ticksExecutados(edf, 1) == 20);
        CHECK(ticksExecutados(edf, 2) == 40);
    }

    // O instante exato da perda de prazo é o do prazo (T2: chega em 0, prazo 7).
    void testeInstanteDaPerdaDePrazo()
    {
        Simulador rm(cfg("RM", 1), {tarefa(1, 5, 0, 2, 5), tarefa(2, 7, 0, 4, 7)}, 1);
        rm.executarCompleto();
        bool achou = false;
        for (const Evento& e : rm.eventos())
            if (e.tipo == TipoEvento::perdaPrazo && e.idTarefa == 2 && e.instante == 7) achou = true;
        CHECK(achou);
    }

    // Terminar exatamente no prazo CUMPRE o prazo.
    void testeTerminarNoPrazoNaoEhPerda()
    {
        Simulador s(cfg("EDF", 1), {tarefa(1, 4, 0, 4, 4)}, 1);   // usa 100% da CPU, termina em 4, prazo 4
        CHECK(s.executarCompleto());
        CHECK(contar(s, TipoEvento::perdaPrazo) == 0);
    }

    // Duração maior que o período: ativações se acumulam, nenhuma é descartada.
    void testeSobrecarga()
    {
        Simulador s(cfg("EDF", 1), {tarefa(1, 5, 0, 7, 5)}, 1);
        CHECK(s.executarCompleto());
        CHECK(s.estadoAtual().tcbs[0].getExecucoes() == 10);
        CHECK(ticksExecutados(s, 1) == 70);
        CHECK(contar(s, TipoEvento::chegada, 1) == 10);
        CHECK(contar(s, TipoEvento::perdaPrazo, 1) >= 1);
    }

    // Várias CPUs: CPU ociosa é desligada e o tempo desligada é contabilizado.
    void testeMultiCpu()
    {
        Simulador s(cfg("EDF", 3), {tarefa(1, 10, 0, 2, 10)}, 1);
        CHECK(s.executarCompleto());
        CHECK(contar(s, TipoEvento::cpuDesligada) >= 2);      // CPUs 1 e 2 nunca tiveram o que fazer
        CHECK(s.ticksDesligada(1) == s.tickAtual());
        CHECK(s.ticksDesligada(2) == s.tickAtual());
        CHECK(s.ticksDesligada(0) < s.tickAtual());
    }

    // Com mais tarefas do que CPUs, nenhuma CPU pode ficar desligada enquanto
    // existe tarefa na fila de prontos (requisito 1.2).
    void testeSemOciosidadeComFila()
    {
        Simulador s(cfg("EDF", 2),
                    {tarefa(1, 10, 0, 4, 10), tarefa(2, 10, 0, 4, 10), tarefa(3, 10, 0, 4, 10)}, 1);
        CHECK(s.executarCompleto());
        // Nos 8 primeiros ticks há 3 tarefas para 2 CPUs: as duas CPUs ocupadas.
        for (std::size_t t = 0; t < 4; t++)
            for (int quem : s.traco()[t]) CHECK(quem != -1);
    }

    // Duas tarefas totalmente idênticas: o desempate é por sorteio e é registrado.
    void testeSorteio()
    {
        Simulador s(cfg("EDF", 1), {tarefa(1, 10, 0, 2, 10), tarefa(2, 10, 0, 2, 10)}, 1);
        CHECK(s.executarCompleto());
        CHECK(contar(s, TipoEvento::sorteio) >= 1);
        // Mesma semente => mesma simulação (reprodutível).
        Simulador s2(cfg("EDF", 1), {tarefa(1, 10, 0, 2, 10), tarefa(2, 10, 0, 2, 10)}, 1);
        s2.executarCompleto();
        CHECK(s.traco() == s2.traco());
    }

    // Retroceder e avançar de novo reproduz exatamente a mesma simulação.
    void testeRetrocederEAvancar()
    {
        Simulador s(cfg("EDF", 2),
                    {tarefa(1, 5, 0, 2, 5), tarefa(2, 5, 0, 2, 5), tarefa(3, 7, 1, 3, 7)}, 7);
        CHECK(s.executarCompleto());
        const auto tracoFinal = s.traco();
        const std::size_t eventosFinal = s.eventos().size();
        const int fim = s.tickAtual();

        for (int i = 0; i < 12; i++) CHECK(s.voltar());
        CHECK(s.tickAtual() == fim - 12);
        CHECK(s.numEventosVisiveis() <= eventosFinal);
        CHECK(!s.terminou());
        for (int i = 0; i < 12; i++) CHECK(s.passo());
        CHECK(s.tickAtual() == fim);
        CHECK(s.terminou());
        CHECK(s.traco() == tracoFinal);
        CHECK(s.eventos().size() == eventosFinal);
        CHECK(s.numEventosVisiveis() == eventosFinal);

        // Não dá para voltar além do instante 0.
        while (s.voltar()) {}
        CHECK(s.tickAtual() == 0);
        CHECK(!s.voltar());
    }

    void testeEdicao()
    {
        Simulador s(cfg("EDF", 1), {tarefa(1, 10, 0, 6, 10)}, 1);
        for (int i = 0; i < 3; i++) s.passo();       // tarefa 1 executou 3 de 6 ticks

        CHECK(!s.editarTarefa(99, "duracao", 2).ok);                 // tarefa inexistente
        CHECK(!s.editarTarefa(1, "tempo_restante", 0).ok);           // fora do intervalo
        CHECK(!s.editarTarefa(1, "tempo_restante", 7).ok);           // maior que a duração
        CHECK(!s.editarTarefa(1, "periodo", 0).ok);                  // aperiódica: só no Projeto B
        CHECK(!s.editarTarefa(1, "ingresso", 5).ok);                 // já chegou ao sistema
        CHECK(!s.editarTarefa(1, "banana", 5).ok);                   // campo inexistente
        CHECK(!s.editarTarefa(1, "periodo", 2).ok);                  // próxima ativação cairia no passado

        // Erros não podem ter alterado nada.
        CHECK(s.estadoAtual().tcbs[0].getTempoRestante() == 3);

        const ResultadoEdicao r = s.editarTarefa(1, "tempo_restante", 1);
        CHECK(r.ok);
        CHECK(s.tickAtual() == 3);
        CHECK(s.executarCompleto());
        CHECK(ticksExecutados(s, 1) == 3 + 1 + 9 * 6);               // 1ª ativação terminou antes

        // Edição depois de retroceder descarta o futuro antigo.
        Simulador q(cfg("EDF", 1), {tarefa(1, 10, 0, 4, 10)}, 1);
        q.executarCompleto();
        for (int i = 0; i < 5; i++) q.voltar();
        const int t = q.tickAtual();
        CHECK(q.passo());                                            // dá para reavançar (futuro guardado)
        q.voltar();
        CHECK(q.alterarAlgoritmo("rm").ok);
        CHECK(!q.alterarAlgoritmo("fifo").ok);
        CHECK(q.traco().size() == static_cast<std::size_t>(t));      // futuro descartado
        CHECK(!q.alterarNumCpus(2).ok);                              // fora do instante 0
        while (q.voltar()) {}
        CHECK(q.alterarNumCpus(2).ok);
        CHECK(q.executarCompleto());

        // Encerrar uma tarefa à força.
        Simulador e(cfg("EDF", 1), {tarefa(1, 5, 0, 2, 5), tarefa(2, 5, 0, 2, 5)}, 3);
        e.passo();
        CHECK(e.encerrarTarefa(2).ok);
        CHECK(!e.encerrarTarefa(2).ok);
        CHECK(e.executarCompleto());
        CHECK(e.estadoAtual().tcbs[1].getExecucoes() == 0);
    }

    // O leitor deve ignorar (com aviso) tarefas aperiódicas, mesmo com prazo 0.
    void testeLeitor()
    {
        LeitorConfig l;
        const std::string ok = escreverArquivo("so_aperiodica_ok.txt",
            "rm;2;2;\n1;FF0000;0;2;5;5;\n\n2 ; 00ff00 ; 1 ; 3 ; 10 ; 8\n3;0000FF;0;1;0;0\n");
        CHECK(l.carregar(ok));
        CHECK(l.getErros().empty());
        CHECK(l.getTarefas().size() == 2);
        CHECK(l.getAvisos().size() == 1);
        CHECK(l.getConfiguracao().getAlgoritmo() == "RM");

        const std::string ruim = escreverArquivo("erros.txt",
            "XYZ;0;0\n1;FF0000;0;2;5;5\n1;00FF00;0;3;4;4\n3;GGGGGG;0;1;5;5\n4;FF00FF;0;1;-3;5\n5;FF00FF;0;1;5\n");
        LeitorConfig l2;
        CHECK(!l2.carregar(ruim));
        CHECK(l2.getErros().size() >= 5);

        LeitorConfig l3;
        CHECK(!l3.carregar("/caminho/que/nao/existe.txt"));
    }
}

int main()
{
    testeRmContraEdf();
    testeInstanteDaPerdaDePrazo();
    testeTerminarNoPrazoNaoEhPerda();
    testeSobrecarga();
    testeMultiCpu();
    testeSemOciosidadeComFila();
    testeSorteio();
    testeRetrocederEAvancar();
    testeEdicao();
    testeLeitor();

    if (falhas == 0) std::cout << "Todos os testes passaram.\n";
    else std::cout << falhas << " verificacao(oes) falhou(aram).\n";
    return falhas == 0 ? 0 : 1;
}
