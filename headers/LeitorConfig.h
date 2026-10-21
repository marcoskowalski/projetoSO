#ifndef PROJETOSO_LEITORCONFIG_H
#define PROJETOSO_LEITORCONFIG_H

#include <set>
#include <string>
#include <vector>

#include "Configuracao.h"
#include "Tarefa.h"

// Lê o arquivo de configuração (texto simples) e preenche a Configuracao e a
// lista de Tarefas. Formato:
//   algoritmo_escalonamento;quantum;qtde_cpus
//   id;cor;ingresso;duracao;periodo;prazo;lista_eventos
// Os erros NÃO lançam exceção: ficam guardados (com o número da linha) para
// que o usuário veja todos os problemas de uma vez.
class LeitorConfig
{
    private:
        Configuracao configuracao;
        std::vector<Tarefa> tarefas;
        std::vector<std::string> erros;
        std::vector<std::string> avisos;
        std::set<int> idsLidos;     // para detectar ids repetidos

        void lerCabecalho(const std::string& linha, int numLinha);
        void lerTarefa(const std::string& linha, int numLinha);

    public:
        LeitorConfig();
        ~LeitorConfig();

        // Lê o arquivo indicado (qualquer nome/caminho). Retorna true se não
        // houve nenhum erro; nesse caso os getters trazem a configuração.
        bool carregar(const std::string& caminho);

        [[nodiscard]] Configuracao getConfiguracao() const;
        [[nodiscard]] std::vector<Tarefa> getTarefas() const;
        [[nodiscard]] std::vector<std::string> getErros() const;
        [[nodiscard]] std::vector<std::string> getAvisos() const;
};

#endif //PROJETOSO_LEITORCONFIG_H
