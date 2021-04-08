#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PAGE_TABLE_SIZE (256)
#define TLB_SIZE (16)
#define OFFSET_MASK (0x00FF)

typedef struct
{
    int page_number;
    int frame_number;
} tlb_data;

int page_table[PAGE_TABLE_SIZE];
int next_available_page_entry = 0;
tlb_data tlb[TLB_SIZE];
int next_available_tlb_entry = 0;

int search_tlb(int find_value)
{
    for(int i = 0;i < TLB_SIZE;i++)
    {
        if(tlb[i].page_number == find_value)
        {
            return i;
        }
    }

    return -1;
}

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

    int virtual_address;
    int physical_address;
    char value;

    int tlb_position;
    int page_position;
    int page_number;
    int offset;
    int frame_number;

    int page_faults = 0;
    int tlb_hits = 0;
    int address_count = 0;

    char line[8] = {0};
    while (fgets(line, sizeof(line), fp))
    {
        address_count++;

        virtual_address = atoi(line);
        fseek(file_back_store, virtual_address, SEEK_SET );
        value = fgetc(file_back_store);

        page_number = virtual_address >> 8;
        offset = virtual_address & OFFSET_MASK;

        tlb_position = search_tlb(page_number);
        if(tlb_position == -1)
        {
            page_position = search_page_table(page_number);

            if(page_position == -1)
            {
                page_table[next_available_page_entry] = page_number;
                frame_number = next_available_page_entry;
                next_available_page_entry = (next_available_page_entry + 1) % PAGE_TABLE_SIZE;
                page_faults++;
            }
            else
            {
                frame_number = page_position;
            }

            physical_address = (frame_number << 8) | offset;
            tlb[next_available_tlb_entry].page_number = page_number;
            tlb[next_available_tlb_entry].frame_number = frame_number;
            next_available_tlb_entry = (next_available_tlb_entry + 1) % TLB_SIZE;

        }
        else
        {
            physical_address = (tlb[tlb_position].frame_number << 8) | offset;
            tlb_hits++;
        }

        printf("Virtual address: %d Physical address: %d Value: %d\n", virtual_address, physical_address, value);
    }

    printf("Page faults = %d / %d, %0.2f\n", page_faults, address_count, (float)page_faults/address_count);
    printf("TLB hits = %d / %d, %0.3f\n", tlb_hits, address_count, (float)tlb_hits/address_count);

    fclose(fp);
    fclose(file_back_store);

    return 0;
}
