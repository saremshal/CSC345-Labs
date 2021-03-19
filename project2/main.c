#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>

/* structure for passing data to threads */
typedef struct
{
    int row;
    int column;
} parameters;

typedef enum
{
    CHECK_TYPE_ROW = 0,
    CHECK_TYPE_COLUMN,
    CHECK_TYPE_SQUARE
} check_types;

typedef struct
{
    check_types type;
    parameters *input;
    int *result;
} arguments;

int board[9][9];
int check_results[11];

uint8_t check_row(void)
{
    uint8_t is_value_present;
    for(int i=0;i<9;i++)
    {
        for(int k=1;k<10;k++)
        {
            is_value_present = 0;
            for(int j=0;j<9;j++)
            {
                if(board[i][j] == k)
                {
                    is_value_present = 1;
                }
            }
            if(!is_value_present)
            {
                return 0;
            }
        }
    }
    return 1;
}

uint8_t check_column(void)
{
    uint8_t is_value_present;
    for(int i=0;i<9;i++)
    {
        for(int k=1;k<10;k++)
        {
            is_value_present = 0;
            for(int j=0;j<9;j++)
            {
                if(board[j][i] == k)
                {
                    is_value_present = 1;
                }
            }
            if(!is_value_present)
            {
                return 0;
            }
        }
    }
    return 1;
}

uint8_t check_square(parameters *start_pos)
{
    uint8_t is_value_present;

    for(int k=1;k<10;k++)
    {
        is_value_present = 0;
        for(int i=start_pos->row;i<start_pos->row+3;i++)
        {
            for(int j=start_pos->column;j<start_pos->column+3;++j)
            {
                if(board[i][j] == k)
                {
                    is_value_present = 1;
                }
            }
        }
        if(!is_value_present)
        {
            return 0;
        }
    }

    return 1;
}

void *runner(void *param)
{
    arguments *args = param;
    switch (args->type)
    {
        case CHECK_TYPE_ROW:
            *(args->result) = check_row();
            break;
        case CHECK_TYPE_COLUMN:
            *(args->result) = check_column();
            break;
        case CHECK_TYPE_SQUARE:
            *(args->result) = check_square(args->input);
            break;
    }

    pthread_exit(0);
}

uint8_t check_final_result(int results[11])
{
    for(int i=0;i<11;i++)
    {
        if(results[i] == 0)
        {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char** argv)
{
    clock_t start, end;
    float cpu_time_used;
    start = clock();

    FILE * fp;
    char str[19];
    int mode = atoi(argv[1]);

    printf("BOARD STATE IN input.txt:\n");
    fp = fopen("input.txt", "r");

    for(int i=0; i<9; i++)
    {
        fgets (str, 19, fp);
        printf("%s",str);
        for(int j=0; j<9; j++)
        {
            board[i][j] = atoi(&str[(j*2)]);
        }
    }

    if(mode == 1)
    {
        parameters *data = (parameters*) malloc(sizeof(parameters));
        for(int i=0;i<9;i++)
        {
            data->row = (i / 3)*3; // 0,0,0,3,3,3,6,6,6
            data->column = (i % 3)*3; //0,3,6,0,3,6,0,3,6
            check_results[i] = check_square(data);
          //  printf("%d\n", check_square(data));
        }
        check_results[9] = check_row();
        //printf("%d\n", check_row());
        check_results[10] = check_column();
        //printf("%d\n", check_column());
    }
    else if(mode == 2)
    {
        pthread_t tids[11];
        arguments inputs[11];

        for(int i=0;i<9;i++)
        {
            inputs[i].input = (parameters*) malloc(sizeof(parameters));
            inputs[i].input->row = (i / 3)*3; // 0,0,0,3,3,3,6,6,6
            inputs[i].input->column = (i % 3)*3; //0,3,6,0,3,6,0,3,6
            inputs[i].result = &check_results[i];
            inputs[i].type = CHECK_TYPE_SQUARE;
            pthread_create(&tids[i],0,runner,(void *)&inputs[i]);
        }

        inputs[9].result = &check_results[9];
        inputs[9].type = CHECK_TYPE_ROW;
        pthread_create(&tids[9],0,runner,(void *)&inputs[9]);

        inputs[10].result = &check_results[10];
        inputs[10].type = CHECK_TYPE_COLUMN;
        pthread_create(&tids[10],0,runner,(void *)&inputs[10]);

        for(int i=0;i<11;i++)
        {
            pthread_join(tids[i],NULL);
        }
    }
    else if(mode == 3)
    {
        int fd;
        const int SIZE = 4096;
        char *ptr;
        const int ptr_size = 1024;
        const char *name = "COLLATZ";
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        ftruncate(fd, SIZE);
        ptr= mmap(0, SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
        pid_t pids[11];
        parameters *data = (parameters*) malloc(sizeof(parameters));

        /* Child Creation Loop */
        for (int i = 0; i < 11; ++i)
        {

            pids[i] = fork();
            if (pids[i] < 0)
            {
                printf("Piping Failed\n");
                exit (1);
            }
            else if (pids[i] == 0)
            {
                if(i<9)
                {
                    int check;
                    data->row = (i / 3)*3; // 0,0,0,3,3,3,6,6,6
                    data->column = (i % 3)*3; //0,3,6,0,3,6,0,3,6

                    sprintf(ptr,"%u", check_square(data));
                    ptr += 1;
                }
                else if(i==9)
                {
                    sprintf(ptr,"%u", check_row());
                    ptr += 1;
                }
                else
                {
                    sprintf(ptr,"%u", check_column());
                    ptr += 1;
                }
                exit(0);
          }
      }

      /* Parent Waiting Outside For Loop */
      wait(NULL);
      fd = shm_open(name, O_RDONLY, 0666);
      ptr = mmap(0,SIZE,PROT_READ,MAP_SHARED, fd, 0);
      printf("%s", (char *)ptr);
      shm_unlink(name);
    }
    else
    {
        printf("Mode is not valid, please use either 1, 2, or 3\n");
        return 0;
    }

    int final_result = check_final_result(check_results);
    end = clock();
    cpu_time_used = ((float) (end - start)) / CLOCKS_PER_SEC;
  //  printf("SOLUTION: %s (%0.4f seconds)\n",final_result ? "YES" : "NO", cpu_time_used);
    //printf("SOLUTION: %s (%0.4f milliseconds)\n",final_result ? "YES" : "NO", cpu_time_used*1000);

    return 0;
}
