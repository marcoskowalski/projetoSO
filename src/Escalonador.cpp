#include "../headers/Escalonador.h"
#include "../headers/EscalonadorRM.h"
#include "../headers/EscalonadorEDF.h"

#include <algorithm>
#include <cctype>

Escalonador::Escalonador(unsigned int semente) : gerador(semente) {}
Escalonador::~Escalonador() = default;

void Escalonador::semear(unsigned int semente) { gerador.seed(semente); }

int Escalonador::comparar(const TCB& a, const TCB& b) const
{
    // Critério do algoritmo (RM: período, EDF: prazo absoluto).
    if (prioridade(a) != prioridade(b)) return prioridade(a) < prioridade(b) ? -1 : 1;

    // (1) Quem estava executando antes fica: evita troca de contexto à toa.
    bool aExecutava = a.getEstado() == EstadoTarefa::executando;
    bool bExecutava = b.getEstado() == EstadoTarefa::executando;
    if (aExecutava != bExecutava) return aExecutava ? -1 : 1;

    // (2) Menor prazo (absoluto, da ativação atual).
    if (a.getPrazoAbsoluto() != b.getPrazoAbsoluto())
        return a.getPrazoAbsoluto() < b.getPrazoAbsoluto() ? -1 : 1;

    // (3) Quem chegou antes (instante de ativação atual).
    if (a.getInstanteAtivacao() != b.getInstanteAtivacao())
        return a.getInstanteAtivacao() < b.getInstanteAtivacao() ? -1 : 1;

    // (4) Menor duração.
    int duracaoA = a.getTarefa().getDuracao();
    int duracaoB = b.getTarefa().getDuracao();
    if (duracaoA != duracaoB) return duracaoA < duracaoB ? -1 : 1;

    return 0;   // empate total: sobra o sorteio (critério 5)
}

ResultadoEscalonamento Escalonador::escolher(const std::vector<TCB*>& candidatas, int numCpus)
{
    ResultadoEscalonamento resultado;
    if (candidatas.empty() || numCpus < 1) return resultado;

    // Cada candidata recebe um número aleatório, usado só no último desempate.
    struct Item { TCB* tcb; unsigned int sorteio; };
    std::vector<Item> itens;
    for (TCB* t : candidatas) itens.push_back({t, static_cast<unsigned int>(gerador())});

    std::sort(itens.begin(), itens.end(), [this](const Item& x, const Item& y) {
        int c = comparar(*x.tcb, *y.tcb);
        if (c != 0) return c < 0;
        return x.sorteio < y.sorteio;
    });

    std::size_t qtde = std::min(itens.size(), static_cast<std::size_t>(numCpus));
    for (std::size_t i = 0; i < qtde; i++) resultado.escolhidas.push_back(itens[i].tcb);

    // O sorteio só influenciou o resultado se houve empate total ENTRE a última
    // tarefa escolhida e a primeira que ficou de fora. Nesse caso, todas as
    // escolhidas empatadas com esse grupo foram decididas na sorte.
    if (qtde < itens.size() && comparar(*itens[qtde - 1].tcb, *itens[qtde].tcb) == 0)
    {
        for (std::size_t i = 0; i < qtde; i++)
            if (comparar(*itens[i].tcb, *itens[qtde].tcb) == 0)
                resultado.sorteadas.push_back(itens[i].tcb->getTarefa().getId());
    }
    return resultado;
}

std::unique_ptr<Escalonador> criarEscalonador(const std::string& nome)
{
    std::string n = nome;
    for (char& c : n) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (n == "RM") return std::make_unique<EscalonadorRM>();
    if (n == "EDF") return std::make_unique<EscalonadorEDF>();
    return nullptr;
}
