#include "../headers/EscalonadorRM.h"

int EscalonadorRM::prioridade(const TCB& tcb) const { return tcb.getTarefa().getPeriodo(); }
std::string EscalonadorRM::nome() const { return "RM"; }
