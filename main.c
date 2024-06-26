#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define TEST 0
#define ATMEGA_328P_EEPROM_MAX 1024

/* Supposed to create a single line of .eep of .hex file like:
* 
:100000000C9434000C9451000C9451000C94510049
:100010000C9451000C9451000C9451000C9451001C
...
:00000001FF
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

    FILE* fr = fopen(argv[argc - 1], "r");
    FILE* fw = fopen("EEPROM.eep", "w");

    uint8_t eeprom_buff[ATMEGA_328P_EEPROM_MAX] = {0};

    assert(fr != NULL);
    assert(fw != NULL);

    char c = 0;
    int length = 0;

    while (c != '\n' && c != EOF)
    {
        c = getc(fr);
        eeprom_buff[length++] = c;
    }

    printf("Length is: %d bytes\n", length);

    // SIZE ADR TYPE DATA CHEKSUM

    int size = (length < 16) ? length : 16;
    int type = 00;
    int adr = 0x0000;
    int processed_length = length;
    int byte = 0;
    for (byte = 0; byte < length; byte++)
    {
        if (byte % 16 == 0)
        {
            if (byte != 0)
            {
                uint8_t calc_buffer[20] = { 0 };

                calc_buffer[0] = (uint8_t)size;
                calc_buffer[1] = (uint8_t)((adr & 0xFF00) >> 8);
                calc_buffer[2] = (uint8_t)(adr & 0x00FF);
                calc_buffer[3] = (uint8_t)type;
                memcpy(calc_buffer + 4, eeprom_buff + byte - size, size);

                fprintf(fw, "%02x", intel2scomplement_checksum(calc_buffer, size + 4));
                fprintf(fw, "\n");
                adr+=16;
                processed_length -= size;
                size = (processed_length < 16) ? processed_length : 16;
                if (size < 16)
                {
                    type = 0x00;
                }
            }
            else
            {

            }
            fprintf(fw, ":%02x%04x%02x", size, adr, type);
        }
        fprintf(fw, "%02x", (char)eeprom_buff[byte]);
    }

    uint8_t calc_buffer[20] = { 0 };

    calc_buffer[0] = (uint8_t)size;
    calc_buffer[1] = (uint8_t)((adr & 0xFF00) >> 8);
    calc_buffer[2] = (uint8_t)(adr & 0x00FF);
    calc_buffer[3] = (uint8_t)type;
    memcpy(calc_buffer + 4, eeprom_buff + byte - size, size);

    fprintf(fw, "%02x", intel2scomplement_checksum(calc_buffer, size + 4));

    fprintf(fw, "\n:00000001FF\n");

    fclose(fr);
    fclose(fw);

    printf("Program------------------Done\n");

    return 0;

}