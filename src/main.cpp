// Executa a simulação completa (modo b do requisito 1.5) e imprime um
// relatório em texto. É uma ferramenta de verificação do núcleo do simulador:
// o gráfico de gantt e a interface com o usuário serão feitos por cima dele.
//
// Uso: ProjetoSO arquivo_de_configuracao.txt [semente]
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <map>

#include "LeitorConfig.h"
#include "Simulador.h"

namespace
{
    // Mostra, para cada CPU, os intervalos [inicio,fim) em que executou cada
    // tarefa ou ficou desligada.
    void imprimirLinhaDoTempo(const Simulador& sim)
    {
        const auto& traco = sim.traco();
        const int numCpus = sim.configuracao().getNumCpus();
        for (int cpu = 0; cpu < numCpus; cpu++)
        {
            std::cout << "CPU " << cpu << ":";
            std::size_t ini = 0;
            while (ini < traco.size())
            {
                const int quem = traco[ini][static_cast<std::size_t>(cpu)];
                std::size_t fim = ini;
                while (fim < traco.size() && traco[fim][static_cast<std::size_t>(cpu)] == quem) fim++;
                std::cout << " [" << ini << "," << fim << ")="
                          << (quem == -1 ? std::string("desligada") : "T" + std::to_string(quem));
                ini = fim;
            }
            std::cout << "  | desligada por " << sim.ticksDesligada(cpu) << " ticks\n";
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Uso: " << argv[0] << " arquivo_de_configuracao.txt [semente]\n";
        return 1;
    }

    LeitorConfig leitor;
    const bool ok = leitor.carregar(argv[1]);

    for (const std::string& a : leitor.getAvisos()) std::cout << "AVISO: " << a << "\n";
    for (const std::string& e : leitor.getErros())  std::cout << "ERRO:  " << e << "\n";
    if (!ok) return 1;

    const Configuracao cfg = leitor.getConfiguracao();

    unsigned int semente = std::random_device{}();
    if (argc >= 3) semente = static_cast<unsigned int>(std::strtoul(argv[2], nullptr, 10));

    try
    {
        Simulador sim(cfg, leitor.getTarefas(), semente);
        std::cout << "algoritmo=" << cfg.getAlgoritmo() << " quantum=" << cfg.getQuantum()
                  << " cpus=" << cfg.getNumCpus() << " semente=" << semente << "\n\n";

        const bool terminou = sim.executarCompleto();
        if (!terminou)
        {
            std::cout << "ERRO: a simulação passou do limite de " << Simulador::LIMITE_TICKS
                      << " ticks sem terminar. Verifique períodos muito grandes no arquivo.\n";
            return 1;
        }

        std::cout << "Simulação terminou no instante " << sim.tickAtual() << ".\n\n";

        std::cout << "--- Eventos ---\n";
        std::vector<Evento> evs = sim.eventos();
        std::stable_sort(evs.begin(), evs.end(),
                         [](const Evento& a, const Evento& b) { return a.instante < b.instante; });
        for (const Evento& e : evs)
        {
            std::cout << "t=" << std::setw(4) << e.instante << "  " << nomeEvento(e.tipo);
            if (e.idTarefa >= 0) std::cout << "  T" << e.idTarefa;
            if (e.cpu >= 0) std::cout << "  (CPU " << e.cpu << ")";
            std::cout << "\n";
        }

        std::cout << "\n--- Linha do tempo por CPU ---\n";
        imprimirLinhaDoTempo(sim);

        std::cout << "\n--- Resumo por tarefa ---\n";
        std::map<int, int> perdas;
        for (const Evento& e : sim.eventos())
            if (e.tipo == TipoEvento::perdaPrazo) perdas[e.idTarefa]++;
        for (const TCB& t : sim.estadoAtual().tcbs)
        {
            const int id = t.getTarefa().getId();
            std::cout << "T" << id << ": execucoes=" << t.getExecucoes()
                      << " prazos_perdidos=" << perdas[id] << "\n";
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "ERRO: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
