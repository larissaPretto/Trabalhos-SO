#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct pcb{
    int id;
    int t_cheg;
    int *t_CPU;
    int *disp;
    char *estado;
    int t_espera;
    int t_exe_CPU;
    struct pcb* prox;
};
typedef struct pcb PCB;

struct Fila {
    PCB* ini;
    PCB* fim;
};
typedef struct Fila fila;

struct Dispositivos {
    int* tempServ;
    int* t_exe;
    int* exe_fim;
};
typedef struct Dispositivos dispo;

fila* criarFila(void)
{
    fila* f = (fila*) malloc(sizeof(fila));
    f->ini = f->fim = NULL;
    return f;
}

void inserirFila(fila* f, int t_cheg, int *t_CPU, int *disp, char *estado, int qtdD, int t_espera, int t_exe_CPU, int id)
{
    PCB* p = (PCB*) malloc(sizeof(PCB));
    p->id = id;
    p->t_CPU  = (int*)malloc(qtdD*2 * sizeof(int));
    p->disp  = (int*)malloc(qtdD*2 * sizeof(int));
    p->t_cheg = t_cheg;
    for(int i = 0; i < qtdD*2; i++) {
        p->t_CPU[i] = t_CPU[i];
        p->disp[i] = disp[i];
    }
    p->estado = (char*)malloc(10 * sizeof(char));
    p->estado = estado;
    p->t_espera = t_espera;
    p->t_exe_CPU = t_exe_CPU;
    p->prox = NULL;
    if (f->fim != NULL)
        f->fim->prox = p;
    else
        f->ini = p;

    f->fim = p;
}

int filaVazia(fila* f)
{
    return (f->ini==NULL);
}

void mudarFila(fila* f, fila* proxF, int qtdD, char *estado)
{
    PCB* p;
    if (filaVazia(f))
        printf("Fila vazia");

    p = f->ini;

    f->ini = p->prox;
    if(f->ini == NULL)
        f->fim = NULL;

    inserirFila(proxF, p->t_cheg, p->t_CPU, p->disp, estado, qtdD, p->t_espera, p->t_exe_CPU, p->id);
}

void liberarFila(fila* f)
{
    PCB* p = f->ini;
    while (p != NULL) {
        PCB* c = p->prox;
        free(p->t_CPU);
        free(p->disp);
        free(p);
        p = c;
    }
    free(f);
}

void leituraDados(int* qtdP, int* qtdD, fila* processos, dispo* d)
{
    FILE *file;
    char linha[100], val[100][100];
    int  cont = 2, t_cheg, linhaCont = 1, cCpu = 0, cDisp = 0, p = 0, c;
    int *t_CPU, *disp;

    file = fopen("/home/larissa/Documentos/input1.txt","r");
     if (file != NULL) {
        while (fgets(linha, sizeof(linha), file) != NULL) {
            char *ptr = strtok(linha, " ");
            c = 0;
            while (ptr) {
                strcpy(val[c], ptr);
                ptr = strtok(NULL, " ");
                c++;
            }
            if (linhaCont == 1) {
                *qtdP = atoi(val[0]);
                *qtdD = atoi(val[1]);
                d->tempServ = (int*)malloc(*qtdD * sizeof(int));
                d->t_exe = (int*)malloc(*qtdD * sizeof(int));
                d->exe_fim= (int*)malloc(*qtdD * sizeof(int));
                for (int i = 1; i <= *qtdD; i++) {
                    d->t_exe[i] = 0;
                    d->exe_fim[i] = 0;
                    d->tempServ[i] = atoi(val[cont]);
                    cont++;
                }
            } else {
                t_cheg = atoi(val[0]);
                t_CPU = (int*)malloc(*qtdD*2 * sizeof(int));
                disp = (int*)malloc(*qtdD*2 * sizeof(int));
                for (int i = 1; i < c ;i++) {
                    if (i %2 == 1) {
                        t_CPU[cCpu] = atoi(val[i]);
                        cCpu++;
                    } else {
                        disp[cDisp] = atoi(val[i]);
                        cDisp++;
                    }
                }
                p++;
                inserirFila(processos, t_cheg, t_CPU, disp, "Proc", *qtdD, 0, 0, p);
                free(t_CPU);
                free(disp);
            }
            linhaCont++;
            cCpu = 0;
            cDisp = 0;
        }
        fclose(file);
     } else
         printf("\nArquivo nao abriu\n");
}

int escritaDados(int time, fila* f, int control, int end, int ini)
{
    FILE *file;
    PCB* p;

    if (ini == 0)
        file = fopen("/home/larissa/Documentos/output1.txt","a");
    else
        file = fopen("/home/larissa/Documentos/output1.txt","w");
    if (file != NULL && !filaVazia(f)) {
        if (end == 0) {
            if (control == 0) {
                fprintf(file, "\n t -> %d ", time);
                control = 1;
            }
            for (p = f->ini; p != NULL; p = p->prox)
                fprintf(file,"- | P%d -> %s | ", p->id, p->estado);
        } else if (end == 1) {
            fprintf(file,"\n------------------------------------------------------------------------------\n");
            for (p = f->ini; p != NULL; p = p->prox)
                fprintf(file," P%d -> | t_esp = %d| - | t_CPU = %d | \n", p->id, p->t_espera, p->t_exe_CPU);
        }
    }
    fclose(file);
    return control;
}

void verificarNovosProcessos(fila* f, fila* f_New, int time, int qtdD)
{
    PCB* p = f->ini;
    for (p = f->ini; p!= NULL; p = p->prox) {
        if (p->t_cheg == time) {
            mudarFila(f, f_New, qtdD, "New");
        }
    }
}

void verificarFilas(fila* f_Ready, fila* f_New, fila* *f_Blocked, int qtdD, dispo* d)
{
    PCB* p;
    int cont = 0;
    for (p = f_Ready->ini; p!= NULL; p = p->prox)
        cont++;

    if (cont <= 2) {
        for (int i = 1; i <= qtdD; i++) {
            if (!filaVazia(f_Blocked[i]) && d->exe_fim[i] == 1) {
                mudarFila(f_Blocked[i], f_Ready, qtdD, "Ready");
                d->exe_fim[i] = 0;
                cont++;
            }
        }
    }
    if (!filaVazia(f_New) && cont <= 2) {
        mudarFila(f_New, f_Ready, qtdD, "Ready");
        cont++;
    }
}

void dispositivos(fila* f, dispo* d, int disp)
{
    PCB* p = f->ini;
    if (d->t_exe[disp] == 0) {
        d->t_exe[disp] = d->tempServ[disp];
        d->t_exe[disp] -= 1;
    } else {
        d->t_exe[disp] -= 1;
        if (d->t_exe[disp] == 0) {
            for (int i = 0; i < sizeof(p->disp[0])-1; i++) {
                p->disp[i] = p->disp[i+1];
            }
            p->disp[sizeof(p->disp)-1] = 0;
            d->exe_fim[disp] = 1;
        }
    }
}

void tempo(int t, fila* processos, fila* f_New, int *time, int qtdD, fila* *f_Blocked,  dispo* d, fila* f_Ready, fila* f_Exit, fila* f_Running)
{
    for (int i = 1; i <= t; i++) {
        int control = 0;
        *time += 1;
        verificarNovosProcessos(processos, f_New, *time, qtdD);
        control = escritaDados(*time, f_New,control,0,0);
        control = escritaDados(*time, f_Ready,control,0,0);
        control = escritaDados(*time, f_Running,control,0,0);
        for (int c = 1; c <= qtdD; c++) {
            if (!filaVazia(f_Blocked[c])) {
                dispositivos(f_Blocked[c], d, c);
                control = escritaDados(*time, f_Blocked[c],control,0,0);
            }
        }
        verificarFilas(f_Ready, f_New, f_Blocked, qtdD, d);
        control = escritaDados(*time, f_Exit,control,0,0);
    }
}

int cpu(fila* f, int *time, int qtdD, int *qtdP, fila* processos, fila* f_New, fila* *f_Blocked,  dispo* d, fila* f_Ready, fila* f_Exit, fila* f_Running)
{
    PCB* p = f->ini;
    int s = 0;

    if (p->t_CPU[0] > 4) {
        tempo(4, processos, f_New, &*time, qtdD, f_Blocked, d,  f_Ready, f_Exit, f_Running);
        p->t_exe_CPU += 4;
        p->t_CPU[0] = p->t_CPU[0] - 4;
        s = -1;
    } else {
        tempo(p->t_CPU[0], processos, f_New, &*time, qtdD, f_Blocked, d,  f_Ready, f_Exit, f_Running);
        p->t_exe_CPU += p->t_CPU[0];
        for (int i = 0; i < sizeof(p->t_CPU)-1; i++) {
            p->t_CPU[i] = p->t_CPU[i+1];
        }
        p->t_CPU[sizeof(p->t_CPU)-1] = 0;
        s = p->disp[0];
    }
    if (p->t_CPU[0] == 0) {
        p->t_espera = *time - p->t_cheg - p->t_exe_CPU;
        *qtdP -= 1;
        s = 0;
     }
     return s;
}

int main()
{
    int qtdP = 0, qtdD = 0, time = 0, disp;
    fila* processos = criarFila();
    fila* f_New = criarFila();
    fila* f_Ready = criarFila();
    fila* f_Running = criarFila();
    fila* f_Exit = criarFila();
    dispo* d = (dispo*)malloc(sizeof(dispo));

    leituraDados(&qtdP, &qtdD, processos, d);

    fila* f_Blocked[qtdD];
    for (int i = 1; i <= qtdD; i++)
        f_Blocked[i] = criarFila();

    verificarNovosProcessos(processos, f_New, time, qtdD);
    escritaDados(time ,f_New,0,0,1);
    verificarFilas(f_Ready, f_New, f_Blocked, qtdD, d);
    while (qtdP > 0) {
        if (filaVazia(f_Running) && !filaVazia(f_Ready)) {
            mudarFila(f_Ready, f_Running, qtdD, "Running");
            disp = cpu(f_Running, &time, qtdD, &qtdP, processos, f_New, f_Blocked, d, f_Ready, f_Exit, f_Running);
            if (disp > 0)
                mudarFila(f_Running, f_Blocked[disp], qtdD, "Blocked");
            else if (disp == -1)
                mudarFila(f_Running, f_Ready, qtdD, "Ready");
            else
                mudarFila(f_Running, f_Exit, qtdD, "Terminated");
        } else
            tempo(1, processos, f_New, &time, qtdD, f_Blocked, d, f_Ready, f_Exit, f_Running);
    }
    escritaDados(time+1, f_Exit,0,0,0);
    escritaDados(time+1, f_Exit,0,1,0);
    free(d->tempServ);
    free(d->t_exe);
    free(d->exe_fim);
    free(d);
    liberarFila(processos);
    liberarFila(f_New);
    liberarFila(f_Ready);
    liberarFila(f_Running);
    for (int i = 1; i <= qtdD; i++)
        liberarFila(f_Blocked[i]);
    liberarFila(f_Exit);
    return 0;
}
