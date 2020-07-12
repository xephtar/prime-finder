//g++ -std=c++11 150160048.c -lpthread
//./a.out interval_min interval_max np nt
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

pthread_mutex_t lock;

struct interval{
    int start;
    int end;
    int *numbers;
    int count;
};

void* findPrimeNumber(void *input){
    pthread_mutex_lock(&lock);
    int counter = 0;
    for (int c = ((struct interval*)input)->start; c <= ((struct interval*)input)->end; c++){
        int flag = 0;
        for (int x = 2; x <= c/2; x++)
        {
            if (c%x == 0){
                flag = 1;
                break;
            }
        }
        if(!flag){
            ((struct interval*)input)->numbers[counter] = c;
            counter++;
        }
    }
    ((struct interval*)input)->count = counter;
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(int argc,char* argv[]){
    int i; //for loop initial int
    if(argc < 4){
        return 0;
    }

    int max_process = atoi(argv[3]);
    int max_thread = atoi(argv[4]);
    int start_point = atoi(argv[1]);
    int end_point = atoi(argv[2]);
    int range = end_point - start_point;
    int process_step = range / max_process;
    int thread_step = process_step / max_thread;
    pthread_t tid[max_thread];

    int shmid[max_process];

    for (int m = 0; m < max_process; ++m) {
        key_t ipc_key;
        ipc_key = ftok(get_current_dir_name(), m + 1);
        if((shmid[m]=shmget(ipc_key, 32, IPC_CREAT|0666))==-1)
        {
            printf("error creating shared memory\n");
            exit(1);
        }
    }

    int c = 1; //to create three childs of level 1 parent

    int x = start_point;
    int y = start_point + process_step;

    for (i = 1; i <= max_process; i++){
        if(i != 1 && i == max_process){
            x = y + 1;
            y = end_point;
        }else if(i != 1){
            x = y + 1;
            y += process_step;
        }

        //#####################################
        if ( c > 0){//to avoid that child calls fork
            c = fork();
        }
        //#####################################

        if ( c == 0 ){//if it is child
            printf("Proses %d: basladi. Aralik: %d-%d.\n", i, x, y);

            int j = 0;
            int err;

            if (pthread_mutex_init(&lock, NULL) != 0)
            {
                printf("\n mutex init failed\n");
                return 1;
            }
            struct interval thread_interval[max_thread];

            while(j < max_thread){
                if(j == 0){
                    if(max_thread == 1){
                        thread_interval[j].start = x;
                        thread_interval[j].end = y;
                    }else{
                        thread_interval[j].start = x;
                        thread_interval[j].end = thread_interval[j].start + thread_step;
                    }
                }else if(j+1 == max_thread && max_thread != 1){
                    thread_interval[j].start = thread_interval[j-1].end + 1;
                    thread_interval[j].end = y;
                }else{
                    thread_interval[j].start = thread_interval[j-1].end + 1;
                    thread_interval[j].end = thread_interval[j].start + thread_step;
                }

                thread_interval[j].numbers = (int*)malloc((thread_interval[j].end-thread_interval[j].start) * sizeof(int));
                err = pthread_create(&(tid[j]), NULL, findPrimeNumber, (void *) &thread_interval[j]);
                printf("Iplik %d.%d: araniyor. Aralik: %d-%d\n", i,j+1, thread_interval[j].start, thread_interval[j].end);
                if (err != 0){
                    printf("\ncan't create thread :[%s]", strerror(err));
                }
                j++;
            }
            for (int k = 0; k < max_thread; ++k) {
                pthread_join(tid[k], NULL);
            }
            sleep(5);
            int size = 0;
            for (int l = 0; l < max_thread; ++l) {
                size += thread_interval[l].count;
            }
            int *p = (int *)shmat(shmid[i-1],0,0);

            *p = size;
            p++;

            for (int l = 0; l < max_thread; ++l) {
                for (int k = 0; k < thread_interval[l].count; ++k) {
                    *p = thread_interval[l].numbers[k];
                    p++;
                }
            }
            printf("Proses %d: sonlandi.\n", i);
            int z = 0;
            while (z < max_thread){
                free(thread_interval[z].numbers);
                thread_interval[z].numbers = NULL;
                z++;
            }
            _exit(0);
        }else{
            if(i == 1){
                //parent process
                printf("Anne proses: basladi.\n");
            }
            wait(NULL);
        }
    }
    wait(NULL);
    if(c > 0){
        printf("Ana Proses: Sonlandi. Bulunan asal sayilar: ");
        for (int k = 0; k < max_process; ++k){
            int* parent = (int*)shmat(shmid[k], 0,0);
            parent++;
            for (int j = 1; j <= parent[0]; ++j) {
                printf("%d, ", *parent);
                parent++;
            }
            //detach from shm
            sleep(5);
        }

        printf("\n");

        for (int l = 0; l < max_process; ++l) {
            shmctl(shmid[l], IPC_RMID,0);
        }
    }

    pthread_mutex_destroy(&lock);
    return 0;
}