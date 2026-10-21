#include "../headers/EscalonadorEDF.h"

int EscalonadorEDF::prioridade(const TCB& tcb) const { return tcb.getPrazoAbsoluto(); }
std::string EscalonadorEDF::nome() const { return "EDF"; }
