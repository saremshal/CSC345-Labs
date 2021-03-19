#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
typedef u_int8_t uint8_t;

/* structure for passing data to check_square */
typedef struct
{
    int row;
    int column;
} parameters;

/* enum that threads use to determine what process to run */
typedef enum
{
    CHECK_TYPE_ROW = 0,
    CHECK_TYPE_COLUMN,
    CHECK_TYPE_SQUARE
} check_types;

/* structure for passing data to threads */
typedef struct
{
    check_types type;
    parameters *input;
    int *result;
} arguments;

/* array for storing sudoku board*/
int board[9][9];
/*array for storing row, cloumn, and squares validation*/
int check_results[11];

/*function for checking all 9 rows of board*/
uint8_t check_row(void)
{
    uint8_t is_value_present;
    /*looping through each of 9 rows on board*/
    for(int i=0;i<9;i++)
    {
      /*looping through sudoku solution numbers*/
        for(int k=1;k<10;k++)
        {
            is_value_present = 0;
            /*looping through each number in row*/
            for(int j=0;j<9;j++)
            {
              /*checking if row has valid sudoku solution*/
                if(board[i][j] == k)
                {
                    is_value_present = 1;
                }
            }
            /*if row does not have number return 0*/
            if(!is_value_present)
            {
                return 0;
            }
        }
    }
    /*if row has all numbers of solution return 1*/
    return 1;
}

/*function for checking all 9 columns of board*/
uint8_t check_column(void)
{
    uint8_t is_value_present;
    /*looping through each of 9 columns on board*/
    for(int i=0;i<9;i++)
    {
      /*looping through sudoku solution numbers*/
        for(int k=1;k<10;k++)
        {
            is_value_present = 0;
            /*looping through each number in column*/
            for(int j=0;j<9;j++)
            {
              /*checking if column has valid sudoku solution*/
                if(board[j][i] == k)
                {
                    is_value_present = 1;
                }
            }
            /*if cloumn does not have number return 0*/
            if(!is_value_present)
            {
                return 0;
            }
        }
    }
    /*if cloumn has all numbers of solution return 1*/
    return 1;
}

/*function for checking a square of board*/
uint8_t check_square(parameters *start_pos)
{
    uint8_t is_value_present;
    /*looping through sudoku solution numbers*/
    for(int k=1;k<10;k++)
    {
        is_value_present = 0;
        /*take starting position of row and looping through 3 rows*/
        for(int i=start_pos->row;i<start_pos->row+3;i++)
        {
          /*take starting position of column and looping through 3 columns*/
            for(int j=start_pos->column;j<start_pos->column+3;++j)
            {
              /*checking if square has valid sudoku solution*/
                if(board[i][j] == k)
                {
                    is_value_present = 1;
                }
            }
        }
        /*if square does not have number return 0*/
        if(!is_value_present)
        {
            return 0;
        }
    }
    /*if square has all numbers of solution return 1*/
    return 1;
}

void *runner(void *param)
{
    /* converts param into arguments variable */
    arguments *args = param;

    /* Looks at which function it should run */
    switch (args->type)
    {
        /* Runs Row Function and stores result in designated variable */
        case CHECK_TYPE_ROW:
            *(args->result) = check_row();
            break;
        /* Runs Column Function and stores result in designated variable */
        case CHECK_TYPE_COLUMN:
            *(args->result) = check_column();
            break;
        /* Runs Square Function and stores result in designated variable */
        case CHECK_TYPE_SQUARE:
            *(args->result) = check_square(args->input);
            break;
    }

    pthread_exit(0);
}

/* Function to validate that all the results are "1" */
uint8_t check_final_result(int results[11])
{
    /* Looks through all the values in the array */
    for(int i=0;i<11;i++)
    {
        /* If any values are 0, returns 0 */
        if(results[i] == 0)
        {
            return 0;
        }
    }
    /* If all values are 1, returns 1 */
    return 1;
}

int main(int argc, char** argv)
{
    /* Sets up timer and records current time */
    clock_t start, end;
    float cpu_time_used;
    start = clock();

    /* Initializes file, file buffer, and reads the mode */
    FILE * fp;
    char str[19];
    int mode = atoi(argv[1]);

    /* Prints out beginging line */
    printf("BOARD STATE IN input.txt:\n");

    /* Opens input file in reading mode */
    fp = fopen("input.txt", "r");

    /* Runs through all the lines of the input file */
    for(int i=0; i<9; i++)
    {
        /* Reads the line, prints the line, and stores values into board array */
        fgets (str, 19, fp);
        printf("%s",str);
        for(int j=0; j<9; j++)
        {
            board[i][j] = atoi(&str[(j*2)]);
        }
    }

    /* Single Process Mode */
    if(mode == 1)
    {
        /* Initalizes Square data input value */
        parameters *data = (parameters*) malloc(sizeof(parameters));
        /* Runs through all 9 squares of the board */
        for(int i=0;i<9;i++)
        {
            /* Picks correct row value (0,3, or 6) */
            data->row = (i / 3)*3;
            /* Picks correct column value (0,3, or 6) */
            data->column = (i % 3)*3;
            /* Saves result into check_results array */
            check_results[i] = check_square(data);
        }
        /* Finds result of row and columns */
        check_results[9] = check_row();
        check_results[10] = check_column();
    }
    /* Multithread Mode */
    else if(mode == 2)
    {
        /* Initalizes threads and inputs */
        pthread_t tids[11];
        arguments inputs[11];

        /* Goes through all 9 squares */
        for(int i=0;i<9;i++)
        {
            /* Initalizes Square data input values are respective row & column values */
            inputs[i].input = (parameters*) malloc(sizeof(parameters));
            inputs[i].input->row = (i / 3)*3;
            inputs[i].input->column = (i % 3)*3;
            /* points the result storage to respective array storage */
            inputs[i].result = &check_results[i];
            /* sets the function to run the square check */
            inputs[i].type = CHECK_TYPE_SQUARE;
            /* Creates the thread */
            pthread_create(&tids[i],0,runner,(void *)&inputs[i]);
        }

        /* Creates thread to run row function and point to row result storage */
        inputs[9].result = &check_results[9];
        inputs[9].type = CHECK_TYPE_ROW;
        pthread_create(&tids[9],0,runner,(void *)&inputs[9]);

        /* Creates thread to run column function and point to column result storage */
        inputs[10].result = &check_results[10];
        inputs[10].type = CHECK_TYPE_COLUMN;
        pthread_create(&tids[10],0,runner,(void *)&inputs[10]);

        /* Makes all the threads join */
        for(int i=0;i<11;i++)
        {
            pthread_join(tids[i],NULL);
        }
    }
    /* Multiprocess Mode */
    else if(mode == 3)
    {
        const char *name = "COLLATZ";
        const int SIZE = 4096;
        const int ptr_size = 1024;
        /* shared memory file descriptor*/
        int fd;
        /*pointer to shared memory object*/
        char *ptr;
        /*create shared memory*/
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        /*configure shared memory size*/
        ftruncate(fd, SIZE);
        /*memory map shared memory object*/
        ptr= mmap(0, SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
        pid_t pids[11];
        parameters *data = (parameters*) malloc(sizeof(parameters));

        /* Child Creation Loop */
        for (int i = 0; i < 11; ++i)
        {
            /*fork a child process*/
            pids[i] = fork();

            /*error occured*/
            if (pids[i] < 0)
            {
                printf("Piping Failed\n");
                exit (1);
            }
            else if (pids[i] == 0)
            {
                /*have the first 9 child processes check each sqaure*/
                if(i<9)
                {
                    int check;
                    data->row = (i / 3)*3; // 0,0,0,3,3,3,6,6,6
                    data->column = (i % 3)*3; //0,3,6,0,3,6,0,3,6

                    /*call check_sqaure function and store results in shared memory*/
                    sprintf(ptr,"%u", check_square(data));
                }
                /*10th child process will check rows of board*/
                else if(i==9)
                {
                    /*call check_row function and store results in shared memory*/
                    sprintf(ptr,"%u", check_row());
                }
                /*Last child process will check columns of board*/
                else
                {
                    /*call check_column function and store results in shared memory*/
                    sprintf(ptr,"%u", check_column());
                }
                exit(0);
          }
          /*parent waits for child process to complete*/
          wait(NULL);
          /*increment pointer*/
          ptr += 1;
      }
      /*opening shared memory*/
      fd = shm_open(name, O_RDONLY, 0666);
      ptr = mmap(0,SIZE,PROT_READ,MAP_SHARED, fd, 0);

      /* print contents of shared memory into check_results function*/
      for(int i = 0;i<11;i++)
      {
          check_results[i] = atoi(&ptr[i]);
      }
      /*remove shared memory object*/
      shm_unlink(name);
    }
    /*user did not enter valid mode*/
    else
    {
        printf("Mode is not valid, please use either 1, 2, or 3\n");
        return 0;
    }

    /* Verifies the final result of all the checks (sees if any of the tests fail) */
    int final_result = check_final_result(check_results);
    /* Reads the click again and computes the run time */
    end = clock();
    cpu_time_used = ((float) (end - start)) / CLOCKS_PER_SEC;
    /* Prints the result */
    printf("SOLUTION: %s (%0.4f seconds)\n",final_result ? "YES" : "NO", cpu_time_used);

    return 0;
}
