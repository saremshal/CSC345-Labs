#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PAGE_TABLE_SIZE (256)
#define TLB_SIZE (16)
#define OFFSET_MASK (0x00FF)
#define FRAME_SIZE (256)

/* Structure For all the TLB Data */
typedef struct
{
    int page_number;
    int frame_number;
} tlb_data;

/* Structure For all the page table data */
typedef struct
{
    int page_number;
    int valid_bit;
} page_data;

/* Page table infomation that lets it act as a fifo (when needed) */
page_data page_table[PAGE_TABLE_SIZE];
int next_available_page_entry = 0;

/* TLB infomation that lets it act as a fifo (when needed) */
tlb_data tlb[TLB_SIZE];
int next_available_tlb_entry = 0;

/* Physical Memory & Frame infomation that lets it act as a fifo (when needed) */
int physical_memory_frame_fifo[FRAME_SIZE];
int next_available_frame_entry = 0;
/* Additional size variable to help keep track for when valid bits need to be false */
int physical_memory_size = 0;

/* function that looks through the whole tlb for a page # and returns the location */
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

/* function that looks through the whole page table for a page # and returns the location */
int search_page_table(int find_value)
{
    for(int i = 0;i < PAGE_TABLE_SIZE;i++)
    {
        /*if page number found in page table return page number*/
        if(page_table[i].page_number == find_value)
        {
            return i;
        }
    }

    return -1;
}

int main(int argc, char** argv)
{
    /* Clears out the page table */
    for(int i = 0;i < PAGE_TABLE_SIZE;i++)
    {
        page_table[i].page_number = -1;
        page_table[i].valid_bit = 0;
    }

    /* Clears out the TLB */
    for(int i = 0;i < TLB_SIZE;i++)
    {
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
    }

    /* Opens all input and output files */
    FILE *fp = fopen("addresses.txt","r");
    FILE *file_back_store = fopen("BACKING_STORE.bin","r");
    FILE *file_out_1 = fopen("out1.txt", "w");
    FILE *file_out_2 = fopen("out2.txt", "w");
    FILE *file_out_3 = fopen("out3.txt", "w");

    /* Variables to help with storing outputs in out#.txt files */
    int virtual_address;
    int physical_address;
    char value;

    /* Variables that store the output of their respective search methods */
    int tlb_position;
    int page_position;

    /* Variables that help with formating data about the virtual memory address */
    int page_number;
    int offset;
    int frame_number;

    /* Variables that keep track about the statistics of the simulation */
    int page_faults = 0;
    int tlb_hits = 0;
    int address_count = 0;

    /* buffer to read addresses.txt */
    char line[8] = {0};
    /* Reads through every line of addresses.txt until it finishes */
    while (fgets(line, sizeof(line), fp))
    {
        /* Increments the addresses */
        address_count++;

        virtual_address = atoi(line);
        /*finding address in back store file and storing in value*/
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

            /* Page table misses */
            if(page_position == -1)
            {
                page_table[next_available_page_entry].page_number = page_number;
                page_table[next_available_page_entry].valid_bit = 1;
                frame_number = next_available_page_entry;

                if(physical_memory_size == FRAME_SIZE)
                {
                    page_table[physical_memory_frame_fifo[next_available_frame_entry]].valid_bit = 0;
                    physical_memory_frame_fifo[next_available_frame_entry] = frame_number;
                    next_available_frame_entry = (next_available_frame_entry + 1) % FRAME_SIZE;
                }
                else
                {
                    physical_memory_frame_fifo[next_available_frame_entry] = frame_number;
                    next_available_frame_entry = (next_available_frame_entry + 1) % FRAME_SIZE;
                    physical_memory_size++;
                }

                next_available_page_entry = (next_available_page_entry + 1) % PAGE_TABLE_SIZE;
                page_faults++; /*increment # of page faults*/
            }
            /* page table hit*/
            else
            {
                /*obtaining frame number from page table*/
                frame_number = page_position;

                if(page_table[page_position].valid_bit == 0)
                {
                    page_faults++;

                    page_table[physical_memory_frame_fifo[next_available_frame_entry]].valid_bit = 0;
                    physical_memory_frame_fifo[next_available_frame_entry] = frame_number;
                    next_available_frame_entry = (next_available_frame_entry + 1) % FRAME_SIZE;
                    page_table[frame_number].valid_bit = 1;
                }
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
