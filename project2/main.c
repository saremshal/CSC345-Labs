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

/* structure for passing data to threads */
typedef struct
{
    int row;
    int column;
} parameters;

int board[9][9];

int main(int argc, char** argv)
{
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

    return 0;
}
