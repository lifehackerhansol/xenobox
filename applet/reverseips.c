#include "../xenobox.h"
#include <stdint.h>

int reverseips(const int argc, const char** argv)
{
    printf("reverseips\n");
    if (argc < 2)
    {
        printf("Usage: reverseips input.ips output.txt\n");
        return -1;
    }
    FILE* input = fopen(argv[1], "rb");
    FILE* output = fopen(argv[2], "w");
    fseek(input, 0L, SEEK_END);
    unsigned int inputsize = ftell(input);
    uint8_t* ips = malloc(inputsize + 1);
    fseek(input, 0, SEEK_SET);
    fread(ips, inputsize, 1, input);
    fclose(input);
    fprintf(output, "PATCH\n");
    unsigned int ipsFilePosition = 5; // skip "PATCH"
    while (ipsFilePosition < inputsize - 3)
    { // skip "EOF"
        fprintf(output, "offset 0x%06X",
                ((ips[ipsFilePosition]) << 16) | ((ips[ipsFilePosition + 1]) << 8) | (ips[ipsFilePosition + 2]));
        unsigned int payloadLength = ((ips[ipsFilePosition + 3]) << 8) | (ips[ipsFilePosition + 4]);
        ipsFilePosition = ipsFilePosition + 5;
        fprintf(output, ", length 0x%04X, payload:", payloadLength);
        if (payloadLength == 0)
        {
            unsigned int realPayloadLength = ((ips[ipsFilePosition]) << 8) | (ips[ipsFilePosition + 1]);
            for (int i = 0; i < realPayloadLength; i++)
            {
                char offset[10];
                if (i == 0)
                    sprintf(offset, " 0x%02X", ips[ipsFilePosition + 2]);
                else
                    sprintf(offset, ", 0x%02X", ips[ipsFilePosition + 2]);
                fprintf(output, "%s", offset);
            }
            ipsFilePosition += realPayloadLength + 2;
        }
        else
        {

            for (int i = 0; i < payloadLength; i++)
            {
                char offset[10];
                if (i == 0)
                    sprintf(offset, " 0x%02X", ips[ipsFilePosition + i]);
                else
                    sprintf(offset, ", 0x%02X", ips[ipsFilePosition + i]);
                fprintf(output, "%s", offset);
            }
            ipsFilePosition += payloadLength;
        }
        fprintf(output, "\n");
    }
    fprintf(output, "EOF");
    fclose(output);
    free(ips);
    printf("Complete.\n");
    return 0;
}
