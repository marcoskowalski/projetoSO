#ifndef PROJETOSO_ESCALONADORRM_H
#define PROJETOSO_ESCALONADORRM_H

#include "Escalonador.h"

// Rate Monotonic preemptivo: menor período = maior prioridade (prioridade fixa).
class EscalonadorRM : public Escalonador
{
    public:
        using Escalonador::Escalonador;   // aceita a semente do sorteio
        [[nodiscard]] int prioridade(const TCB& tcb) const override;
        [[nodiscard]] std::string nome() const override;
};

#endif //PROJETOSO_ESCALONADORRM_H
