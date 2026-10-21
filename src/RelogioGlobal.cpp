

#include "../headers/RelogioGlobal.h"

RelogioGlobal::RelogioGlobal(){
    tickAtual = 0;
}
RelogioGlobal::~RelogioGlobal(){}

int RelogioGlobal::getTickAtual() const{
    return tickAtual;
}
void RelogioGlobal::incrementaTick(){
    tickAtual++;
}
void RelogioGlobal::resetaTick(){
    tickAtual = 0;
}



