#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#define PROGRAM_VERSION "Text2IntelHex_0_0_1"
#define TEST 0
#define WRITING_TO_FILES_DIRECTLY 0
#define FILE_WRITE_NAME "EEPROM.txt"

#define ATMEGA_328P_EEPROM_MAX (1024u)
#define ATMEGA_328P_EERPM_MIN (0u)

#define EEPROM_TYPE_DATA (0u)
#define EEPROM_TYPE_END (1u)

/* Without ':' in the beginning and <CR><LF> */
#define EEPROM_FILE_DATA_MAX_LENGTH (16u)
#define EEPROM_FILE_LINE_MAX_LENGTH (20u)
#define EEPROM_FILE_ENDLINE_LENGTH (5u)
#define EEPROM_REC_SIZE_IDX (0u)
#define EEPROM_REC_ADR_IDX_0 (1u)
#define EEPROM_REC_ADR_IDX_1 (2u)
#define EEPROM_REC_TYPE_IDX (3u)
#define EEPROM_REC_HEADER_SIZE (4u)

/* Needed to pass stdout as am argument */
FILE* output_stream = NULL;

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

void checksum_test()
{
#ifdef TEST
#define SAMPLE_DATA_SIZE 20u
    const uint8_t sample_data_1_checksum = 0x49;
    const uint8_t sample_data_1[SAMPLE_DATA_SIZE] =
    {
        0x10, 0x00, 0x00, 0x00, 0x0C, 0x94, 0x34, 0x00, 0x0C, 0x94,
        0x51, 0x00, 0x0C, 0x94, 0x51, 0x00, 0x0C, 0x94, 0x51, 0x00
    };
    assert(intel2scomplement_checksum(sample_data_1, SAMPLE_DATA_SIZE) == sample_data_1_checksum);
#endif
}

void help()
{
    const char* help_text = \
        "\nProgram creates an intel-8-hex file like:\n" \
        ":100000000C9434000C9451000C9451000C94510049<CR><LF>\n" \
        ":100010000C9451000C9451000C9451000C9451001C<CR><LF>\n" \
        "..down to..<CR><LF>\n" \
        ":00000001FF<CR><LF>\n\n" \
        "Writes to alligned 16-data bytes lines (20 total),\n"
        "and is capable only to write data (type 0x00) and terminate them (0x01).\n"
        "Program expects two arguments:\n"
        "1) path to text file XXX.txt with any string terminated by <EOF>.\n"
        "2) address where should be record stored (only 0x0000 - 0x01FF).\n"
        "Multiple records with different addresses not supported.\n"
        "To be called like \"Text2IntelHex myFile.txt 0000 > myHex.eep\".\n"
        ;
    printf(help_text);
    
}

int write_eeprom_file(FILE* file, uint8_t* read_buff, uint16_t record_length, uint16_t buff_length, uint8_t type, uint16_t address)
{
#if WRITING_TO_FILES_DIRECTLY
    if (NULL == file)
    {
        printf("write_eeprom_file: nullptr");
        return -1;
    }
#endif
    if ( NULL == read_buff)
    {
        printf("write_eeprom_file: nullptr");
        return -1;
    }
    if ((record_length > buff_length) || (address > ATMEGA_328P_EEPROM_MAX - 1) || (address + record_length > ATMEGA_328P_EEPROM_MAX))
    {
        printf("write_eeprom_file: Attempt to write beyond the EEPROM max: %04x %04x\n", ATMEGA_328P_EERPM_MIN, ATMEGA_328P_EEPROM_MAX-1);
        return -1;
    }

    uint16_t size = (record_length < EEPROM_FILE_DATA_MAX_LENGTH) ? record_length : EEPROM_FILE_DATA_MAX_LENGTH;
    uint16_t processed_length = record_length;
    uint16_t byte = 0;
    uint8_t calc_buffer[EEPROM_FILE_LINE_MAX_LENGTH];

    for (byte = 0; byte < record_length; byte++)
    {
        /* If already 16-bytes was written to the .eep file line */
        if (byte % EEPROM_FILE_DATA_MAX_LENGTH == 0)
        {
            /* And if it's not first line => append line with checksum */
            if (byte != 0)
            {

                calc_buffer[EEPROM_REC_SIZE_IDX] = (uint8_t)size;
                calc_buffer[EEPROM_REC_ADR_IDX_0] = (uint8_t)((address & 0xFF00) >> 8);
                calc_buffer[EEPROM_REC_ADR_IDX_1] = (uint8_t)(address & 0x00FF);
                calc_buffer[EEPROM_REC_TYPE_IDX] = (uint8_t)type;
                memcpy(calc_buffer + EEPROM_REC_HEADER_SIZE, read_buff + byte - size, size);

                fprintf(file, "%02x\n", intel2scomplement_checksum(calc_buffer, size + EEPROM_REC_HEADER_SIZE));

                /* Increment size */
                address += size;
                processed_length -= size;
                size = (processed_length < EEPROM_FILE_DATA_MAX_LENGTH) ? processed_length : EEPROM_FILE_DATA_MAX_LENGTH;
            }

            fprintf(file, ":%02x%04x%02x", size, address, type);
        }
        fprintf(file, "%02x", (char)read_buff[byte]);
    }

    calc_buffer[EEPROM_REC_SIZE_IDX] = (uint8_t)size;
    calc_buffer[EEPROM_REC_ADR_IDX_0] = (uint8_t)((address & 0xFF00) >> 8);
    calc_buffer[EEPROM_REC_ADR_IDX_1] = (uint8_t)(address & 0x00FF);
    calc_buffer[EEPROM_REC_TYPE_IDX] = (uint8_t)type;
    memcpy(calc_buffer + EEPROM_REC_HEADER_SIZE, read_buff + byte - size, size);

    fprintf(file, "%02x\n", intel2scomplement_checksum(calc_buffer, size + EEPROM_REC_HEADER_SIZE));

    return 0;

}

void terminate_eeprom_file(FILE* file)
{
    uint8_t calc_buffer[EEPROM_FILE_ENDLINE_LENGTH];
    calc_buffer[EEPROM_REC_SIZE_IDX] = 0u;
    calc_buffer[EEPROM_REC_ADR_IDX_0] = 0u;
    calc_buffer[EEPROM_REC_ADR_IDX_1] = 0u;
    calc_buffer[EEPROM_REC_TYPE_IDX] = (uint8_t)EEPROM_TYPE_END;

    fprintf(file, ":");

    for (uint16_t byte = 0; byte < EEPROM_FILE_ENDLINE_LENGTH - 1; byte++)
    {
        fprintf(file, "%02x", calc_buffer[byte]);
    }

    fprintf(file, "%02x\n", intel2scomplement_checksum(calc_buffer, EEPROM_FILE_ENDLINE_LENGTH - 1));

}

int main(int argc, char** argv)
{
    FILE* fr = NULL;
    FILE* fw = NULL;
    FILE* faux = NULL;
    uint8_t eeprom_buff[ATMEGA_328P_EEPROM_MAX] = { 0 };
    char c = 0;
    int length = 0;
    uint16_t address;

    printf("%s\n", PROGRAM_VERSION);

    if (argc != 3)
    {
        printf("Not enought arguments\n");
        help();
        return -1;
    }

    fr = fopen(argv[argc - 2], "r");

    if (NULL == fr)
    {
        printf("Cant open file: %s\n", argv[argc - 2]);
        help();
        return -1;
    }
    output_stream = stdout;

    checksum_test();

#if WRITING_TO_FILES_DIRECTLY
    fw = fopen("EEPROM.eep", "w");
    if (fw == NULL)
    {
        printf("Cant create EEPROM.eep: %s\n", argv[argc - 2]);
        help();
        return -1;
    }
    output_stream = fw;
#else
    output_stream = stdout;
#endif

    address = strtol(argv[argc - 1], NULL, 16);

    for (c = getc(fr); c != EOF; c = getc(fr))
    {
        eeprom_buff[length++] = c;
    }

    fprintf(output_stream, ";Record length %d bytes:\n", length);

    if (-1 == write_eeprom_file(output_stream, eeprom_buff, length, ATMEGA_328P_EEPROM_MAX, EEPROM_TYPE_DATA, address))
    {
        fclose(fr);
        help();
        return -1;
    }
    terminate_eeprom_file(output_stream);

    fclose(fr);
#if WRITING_TO_FILES_DIRECTLY
    fclose(fw);
#endif

    return 0;

}
