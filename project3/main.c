#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PAGE_TABLE_SIZE (255)
#define TLB_SIZE (16)
#define OFFSET_MASK (0x00FF)

typedef struct
{
    int page_number;
    int frame_number;
} tlb_data;

int page_table[PAGE_TABLE_SIZE];
int next_available_page_entry = 0;

int search_page_table(int find_value)
{
    for(int i = 0;i < PAGE_TABLE_SIZE;i++)
    {
        if(page_table[i] == find_value)
        {
            return i;
        }
    }

    return -1;
}

int main(int argc, char** argv)
{
    for(int i = 0;i < PAGE_TABLE_SIZE;i++)
    {
        page_table[i] = -1;
    }

    FILE* fp = fopen("addresses.txt","r");
    FILE* file_back_store = fopen("BACKING_STORE.bin","r");

    char line[8] = {0};
    int virtual_address;
    int physical_address;
    char value;
    int page_position;
    int page_number;
    int offset;
    while (fgets(line, sizeof(line), fp))
    {
        virtual_address = atoi(line);
        fseek(file_back_store, virtual_address, SEEK_SET );
        value = fgetc(file_back_store);

        page_number = virtual_address >> 8;
        offset = virtual_address & OFFSET_MASK;

        page_position = search_page_table(page_number);

        if(page_position == -1)
        {
            page_table[next_available_page_entry] = page_number;
            physical_address = (next_available_page_entry << 8) | offset;
            next_available_page_entry++;
        }
        else
        {
            physical_address = (page_position << 8) | offset;
        }

        printf("Virtual address: %d Physical address: %d Value: %d\n", virtual_address, physical_address, value);
    }

    fclose(fp);
    fclose(file_back_store);

    return 0;
}
