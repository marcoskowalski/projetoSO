// Teste simples do LeitorConfig.
// Uso: ./teste_leitor arquivo.txt
#include <iomanip>
#include <iostream>

#include "LeitorConfig.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Uso: " << argv[0] << " arquivo_de_configuracao.txt\n";
        return 1;
    }

    LeitorConfig leitor;
    bool ok = leitor.carregar(argv[1]);

    Configuracao cfg = leitor.getConfiguracao();
    std::cout << "algoritmo=" << cfg.getAlgoritmo() << " quantum=" << cfg.getQuantum()
              << " cpus=" << cfg.getNumCpus() << "\n";

    for (const Tarefa& t : leitor.getTarefas())
    {
        std::cout << "id=" << t.getId() << " cor=#" << std::hex << std::uppercase
                  << std::setw(6) << std::setfill('0') << t.getCor() << std::dec << std::setfill(' ')
                  << " ingresso=" << t.getIngresso() << " duracao=" << t.getDuracao()
                  << " periodo=" << t.getPeriodo() << " prazo=" << t.getPrazo() << "\n";
    }

    for (const std::string& a : leitor.getAvisos()) std::cout << "AVISO: " << a << "\n";
    for (const std::string& e : leitor.getErros())  std::cout << "ERRO:  " << e << "\n";

    return ok ? 0 : 1;
}