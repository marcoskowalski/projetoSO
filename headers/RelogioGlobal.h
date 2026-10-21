

#ifndef PROJETOSO_RELOGIOGLOBAL_H
#define PROJETOSO_RELOGIOGLOBAL_H


class RelogioGlobal
{
    private:
        int tickAtual;
    public:
    RelogioGlobal();
    ~RelogioGlobal();

    int getTickAtual() const;
    void incrementaTick();
    void resetaTick();

};


#endif //PROJETOSO_RELOGIOGLOBAL_H