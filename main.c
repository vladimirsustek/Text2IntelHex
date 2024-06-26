#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define TEST 1

/* Supposed to create a single line of .eep of .hex file like:
* 
:100000000C9434000C9451000C9451000C94510049
:100010000C9451000C9451000C9451000C9451001C
*/
uint8_t intel2scomplement_checksum(uint8_t* data, uint8_t length)
{
    /* because max length 255* max data value 255 is 65025 
    accumulator can't get more than uin16_t max */
    uint16_t acc = 0;

    /* accumulate */
    for (uint8_t idx = 0; idx < length; idx++)
    {
        acc += data[idx];
    }

    /* use only LS-byte*/
    acc &= 0x00FF;

    /* do one's comlement */
    acc = ~acc;

    /* finilalize two's complement */
    acc += 1u;

    /* modulo 256 to finish checksum calc*/
    acc %= 256u;

    return (uint8_t)(acc);

}
int main(int argc, char** argv)
{
    printf("Program------------------Start\n");

#ifdef TEST
#define SAMPLE_DATA_SIZE 20u
    const uint8_t sample_data_1_checksum = 0x49;
    const uint8_t sample_data_1[SAMPLE_DATA_SIZE] =
    {
        0x10, 0x00, 0x00, 0x00, 0x0C, 0x94, 0x34, 0x00, 0x0C, 0x94,
        0x51, 0x00, 0x0C, 0x94, 0x51, 0x00, 0x0C, 0x94, 0x51, 0x00
    };
    assert(intel2scomplement_checksum(sample_data_1, SAMPLE_DATA_SIZE) == sample_data_1_checksum);
    printf("All fine!\n");
#endif

    FILE* fptr = fopen(argv[argc - 1], "r");
    assert(fptr != NULL);

    char c = 0;

    while (c != '\n' && c != EOF)
    {   
        c = getc(fptr);
        printf("%c", c);
    }


    fclose(fptr);

    printf("Program------------------Done\n");

}