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

/* structure for passing data to threads */
typedef struct
{
    int row;
    int column;
} parameters;

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

uint8_t check_square(parameters start_pos)
{
    uint8_t is_value_present;

    for(int k=1;k<10;k++)
    {
        is_value_present = 0;
        for(int i=start_pos.row;i<start_pos.row+3;i++)
        {
            for(int j=start_pos.column;j<start_pos.column+3;++j)
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
    FILE * fp;
    char str[19];
    int fd;
	//char *ptr;
	//const int ptr_size = 1024;

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
        parameters data;
        for(int i=0;i<9;i++)
        {
            data.row = (i / 3)*3; // 0,0,0,3,3,3,6,6,6
            data.column = (i % 3)*3; //0,3,6,0,3,6,0,3,6
            check_results[i] = check_square(data);
        }
        check_results[9] = check_row();
        check_results[10] = check_column();
    }
    else if(mode == 2)
    {
        parameters *data = (parameters*) malloc(sizeof(parameters));
        data->row = 1;
        data->column = 1;
        /* Now create the thread passing it data as a parameter */
    }
    else if(mode == 3)
    {
        pid_t pids[11];
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
                //DoWorkInChild();
                printf("child pid %d from parent pid %d\n",getpid(),getppid());
                exit(0);
            }
        }
        wait(NULL); /* Parent Waiting Outside For Loop */
    }
    else
    {
        printf("Mode is not valid, please use either 1, 2, or 3\n");
        return 0;
    }

    int final_result = check_final_result(check_results);
    printf("SOLUTION: %s (%f seconds)\n",final_result ? "YES" : "NO", 5.1234);

    return 0;
}
