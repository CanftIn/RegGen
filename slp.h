#ifndef SLP_H
#define SLP_H

#include "util.h"

typedef struct A_stm_* A_stm;
typedef struct A_exp_* A_exp;
typedef struct A_expList_* A_expList;
typedef enum {
  A_plus, A_minus, A_times, A_div
} A_binop;

/*
 * Stm -> Stm; Stm     (CompoundStm)
 * Stm -> id := Exp      (AssignStm)
 * Stm -> print(ExpList)  (PrintStm)
 */
struct A_stm_ {
  enum {A_compoundStm, A_assignStm, A_printStm} kind;
  union {
    struct {A_stm stm1, stm2;} compound;
    struct {char* id; A_exp exp;} assign;
    struct {A_expList exps;} print;
  } u;
};

A_stm A_CompoundStm(A_stm stm1, A_stm stm2);
A_stm A_AssignStm(char* id, A_exp exp);
A_stm A_PrintStm(A_expList exps);

/*
 * Exp -> id             (IdExp)
 * Exp -> num           (NumExp)
 * Exp -> Exp Binop Exp  (OpExp)
 * Exp -> (Stm, Exp)   (EseqExp)
 */
struct A_exp_ {
  enum {A_idExp, A_numExp, A_opExp, A_eseqExp} kind;
  union {
    char* id;
    int num;
    struct {A_exp left; A_binop oper; A_exp right;} op;
    struct {A_stm stm; A_exp exp;} eseq;
  } u;
};
A_exp A_IdExp(char* id);
A_exp A_NumExp(int num);
A_exp A_OpExp(A_exp left, A_binop oper, A_exp right);
A_exp A_EseqExp(A_stm stm, A_exp exp);


/*
 * ExpList -> Exp, ExpList  (PairExpList)
 * ExpList -> Exp           (LastExpList)
 */
struct A_expList_ {
  enum {A_pairExpList, A_lastExpList} kind;
  union {
    struct {A_exp head; A_expList tail;} pair;
    A_exp last;
  } u;
};
A_expList A_PairExpList(A_exp head, A_expList tail);
A_expList A_LastExpList(A_exp last);

#endif