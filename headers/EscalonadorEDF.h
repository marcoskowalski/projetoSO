#ifndef PROJETOSO_ESCALONADOREDF_H
#define PROJETOSO_ESCALONADOREDF_H

#include "Escalonador.h"

// Earliest Deadline First preemptivo: menor prazo absoluto = maior prioridade.
class EscalonadorEDF : public Escalonador
{
    public:
        using Escalonador::Escalonador;   // aceita a semente do sorteio
        [[nodiscard]] int prioridade(const TCB& tcb) const override;
        [[nodiscard]] std::string nome() const override;
};

#endif //PROJETOSO_ESCALONADOREDF_H
