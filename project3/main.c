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

/*searching TLB for page number*/
int search_tlb(int find_value)
{
    for(int i = 0;i < TLB_SIZE;i++)
    {
        /*if page number found in TLB return page number*/
        if(tlb[i].page_number == find_value)
        {
            return i;
        }
    }

    return -1;
}

/*searching page table for page number*/
int search_page_table(int find_value)
{
    for(int i = 0;i < PAGE_TABLE_SIZE;i++)
    {
        /*if page number found in page table return page number*/
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

    /*opening up files to read from and write to*/
    FILE *fp = fopen("addresses.txt","r");
    FILE *file_back_store = fopen("BACKING_STORE.bin","r");
    FILE *file_out_1 = fopen("out1.txt", "w");
    FILE *file_out_2 = fopen("out2.txt", "w");
    FILE *file_out_3 = fopen("out3.txt", "w");

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

        /* search if TLB has page number*/
        tlb_position = search_tlb(page_number);

        /*TLB Miss*/
        if(tlb_position == -1)
        {
            /*search if page table has position*/
            page_position = search_page_table(page_number);

            /*page table miss*/
            if(page_position == -1)
            {
                /*updating page table with page number*/
                page_table[next_available_page_entry] = page_number;
                frame_number = next_available_page_entry;
                next_available_page_entry = (next_available_page_entry + 1) % PAGE_TABLE_SIZE;
                page_faults++; /*increment # of page faults*/
            }
            /* page table hit*/
            else
            {
                /*obtaining frame number from page table*/
                frame_number = page_position;
            }

            /*obtaing physical address from frame number*/
            physical_address = (frame_number << 8) | offset;
            /*updating TLB with page number and frame number*/
            tlb[next_available_tlb_entry].page_number = page_number;
            tlb[next_available_tlb_entry].frame_number = frame_number;
            /*going to next row of TLB*/
            next_available_tlb_entry = (next_available_tlb_entry + 1) % TLB_SIZE;

        }
        /* TLB Hit*/
        else
        {
            /*obtaining physical adress from TLB*/
            physical_address = (tlb[tlb_position].frame_number << 8) | offset;
            tlb_hits++; /*increment # of TLB hits*/
        }

        /* Debug Line */
        //printf("Virtual address: %d Physical address: %d Value: %d\n", virtual_address, physical_address, value);

        /*storing results of addresses in text files*/
        fprintf(file_out_1, "%d\n", virtual_address);
        fprintf(file_out_2, "%d\n", physical_address);
        fprintf(file_out_3, "%d\n", value);
    }

    /*printing page fault rate and TLB hit rate*/
    printf("Page faults = %d / %d, %0.2f\n", page_faults, address_count, (float)page_faults/address_count);
    printf("TLB hits = %d / %d, %0.3f\n", tlb_hits, address_count, (float)tlb_hits/address_count);

    /*closing files*/
    fclose(fp);
    fclose(file_back_store);

    return 0;
}
