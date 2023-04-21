#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

sem_t MutexG, mutex, semEspera, semFimRodada;
int pedidos = 0, pedidosT = 0, pedidosConsumidos = 0,
rodadas = 0;

typedef struct {
    int id;
    int capG;
    int nClientes;
    int pedidosF;
    int pediu;
} th_arg, *ptr_th_arg;

int barAberto(void)
{
    if(rodadas == 0)
        return 0;
    return 1;
}

void fazPedido(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    arg->pediu = 1 + (rand() % 2);
    if (arg->pediu == 1) {
        sem_wait(&mutex);
        pedidos += 1;
        pedidosT += 1;
        sem_post(&mutex);
        printf("\nPedido realizado por cliente %d\n", arg->id);
    } else {
        printf("\nCliente %d nao vai pedir nessa rodada\n", arg->id);
        pedidosT += 1;
        pedidosConsumidos += 1;
    }
}

void recebePedido(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    if (arg->pediu == 1)
        printf("\nCliente %d recebeu o pedido\n", arg->id);
}


void esperaPedido(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    if (arg->pediu == 1) {
        printf("\nCliente %d está esperando\n", arg->id);
        sem_wait(&semEspera);
    }
}

void consomePedido(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    if (arg->pediu == 1) {
        printf("\nCliente %d está consumindo o pedido\n", arg->id);
        int t = 1 + (rand() % 5);
        sleep(t);
        printf("\nCliente %d consumiu o pedido\n", arg->id);
        sem_wait(&mutex);
        pedidosConsumidos += 1;
        sem_post(&mutex);
    }
    sem_wait(&semFimRodada);
}

int recebeMaximoPedidos(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    sem_wait(&MutexG);
    arg->pedidosF = 0;
    while(1) {
        if (pedidos >= arg->capG) {
            printf("\nGarcom %d esta indo buscar %d pedidos na copa\n", arg->id, arg->capG);
            sem_wait(&mutex);
            pedidos -= arg->capG;
            arg->pedidosF = arg->capG;
            sem_post(&mutex);
            break;
        } else if (pedidosT == arg->nClientes && pedidos > 0 ) {
            printf("\nNao ha mais clientes fazendo novos pedidos, garcom %d vai para a copa pegar %d pedidos restantes\n", arg->id, pedidos);
            sem_wait(&mutex);
            arg->pedidosF = pedidos;
            pedidos -= pedidos;
            sem_post(&mutex);
            break;
        } else if (!barAberto() || pedidosConsumidos == arg->nClientes) {
            break;
        }
    }
    sem_post(&MutexG);
    return arg->pedidosF;
}

void registraPedidos(void *args, int p)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    if (p > 0 && barAberto() && pedidosConsumidos != arg->nClientes)
    printf("\nGarcom %d resgistrou %d pedidos \n", arg->id, p);
}

void entregaPedidos(void *args, int p)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    if (p > 0 && barAberto() && pedidosConsumidos != arg->nClientes) {
        printf("\nGarcom %d esta entregando os pedidos \n", arg->id);
        for (int i = 0; i < arg->pedidosF; i++) {
            sem_post(&semEspera);
        }
    }
}

void *tCliente(void *args)
{
    ptr_th_arg arg = (ptr_th_arg)args;
    while(barAberto()) {
        fazPedido(arg);
        esperaPedido(arg);
        recebePedido(arg);
        consomePedido(arg);
    }
    pthread_exit(NULL);
}

void *tGarcom(void *args)
{
    int p = 0;
    ptr_th_arg arg = (ptr_th_arg)args;
    while(barAberto()) {
        p = recebeMaximoPedidos(arg);
        registraPedidos(arg, p);
        entregaPedidos(arg, p);
        sem_wait(&mutex);
        if (pedidos == 0 && pedidosConsumidos == arg->nClientes) {
            rodadas -= 1;
            if (barAberto())
                printf("\nNOVA RODADA\n\n");
            for (int i = 0; i < arg->nClientes; i++) {
                sem_post(&semFimRodada);
            }
            pedidosT = pedidosConsumidos = 0;
        }
        sem_post(&mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int nClientes = atoi(argv[1]), nGarcons = atoi(argv[2]), capG = atoi(argv[3]), i;
    rodadas = atoi(argv[4]);

    sem_init(&mutex, 0, 1);
    sem_init(&MutexG, 0, 1);
    sem_init(&semEspera, 0, 0);
    sem_init(&semFimRodada, 0, 0);

    pthread_t th[nClientes+nGarcons];
    th_arg arg[nClientes+nGarcons];
    for (int i = 0; i < nGarcons+nClientes; i++) {
        arg[i].capG = capG;
        arg[i].nClientes = nClientes;
        if (i >= nClientes)
            arg[i].id = i+1-nClientes;
        else
            arg[i].id = i+1;
    }
    for (i = 0; i < nGarcons; i++)
        pthread_create(&th[i], NULL, tGarcom, &arg[i]);

    for (int j = i; j < nClientes+nGarcons; j++)
        pthread_create(&th[j], NULL, tCliente, &arg[j]);

    for (int i = 0; i < nGarcons+nClientes; i++)
        pthread_join(th[i], NULL);

    sem_destroy(&mutex);
    sem_destroy(&MutexG);
    sem_destroy(&semEspera);
    sem_destroy(&semFimRodada);
    return 0;
}
