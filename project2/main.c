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

uint8_t check_square(row_position, column_position)
{
    int square[9] = {0};
    uint8_t is_value_present;

    for( int m = row_position; m < row_position + 3; ++m)
    {
        for( int n = column_position; n < column_position + 3; ++n)
        {
            int square_num = board[m][n];
            if(square[square_num-1]==0)
            {
                is_value_present = 1;
            }
            else
            {
                return 0;
            }
        }
    }
    return 1;
}


/*
void *check_squares(void *params)
{
    int square[9] = {0};
    for( int i = beginning_of_col; i < beginning_of_col + 3; ++i){
        for( int j = beginning_of_row; j < beginning_of_row + 3; ++j){

            int check = board[j][i];

            if( value is not duplicate){
            //  check next value
          } else {
            //exit thread
          }

      }
    }
}
*/

int main(int argc, char** argv)
{
    FILE * fp;
    char str[19];
    int fd;
		char *ptr;
		const int ptr_size = 1024;
    pid_t pids[11];
    int i;
    int n = 11;

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


    /*
    printf("\n");
    for(int i=0; i<9; i++)
    {
        for(int j=0; j<9; j++)
        {
            printf("%d", board[i][j]);
        }
        printf("\n");
    }
    */

    parameters *data = (parameters*) malloc(sizeof(parameters));
    data->row = 1;
    data->column = 1;
    /* Now create the thread passing it data as a parameter */


    /* children processes */
    for (i = 0; i < n; ++i) {
        if ((pids[i] = fork()) < 0)
        {
          printf("Piping Failed\n");
          exit (1);
        }
        else if (pids[i] == 0)
        {
            //DoWorkInChild();
            printf("[son] pid %d from [parent] pid %d\n",getpid(),getppid());
            exit(0);
        }
        else
        {
          /*Parent waits for child */
          wait(NULL);
        }
    }



    return 0;
}
