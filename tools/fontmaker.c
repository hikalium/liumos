#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t font[0x100][16];
uint8_t font[0x100][16];

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("%s <in>\n", argv[0]);
    return 1;
  }
  FILE* fp = fopen(argv[1], "rb");
  if (!fp) {
    perror(argv[1]);
    return 1;
  }
  char buf[32];
  int font_index = -1;
  int row_index = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    if (buf[0] == '0') {
      font_index = (int)strtol(buf, NULL, 0);
      row_index = 0;
    } else if (buf[0] == '.' || buf[0] == '*') {
      if (row_index < 0 || 15 < row_index) {
        printf("Row index is out of bound for font 0x%02X\n", font_index);
        return 1;
      }
      uint8_t row_data = 0;
      for (int i = 0; i < 8; i++) {
        row_data <<= 1;
        if (buf[i] == '.')
          continue;
        if (buf[i] != '*') {
          printf("Unexpected char %c\n", buf[i]);
          return 1;
        }
        row_data |= 1;
      }
      if (font_index < 0 || 0xff < font_index) {
        printf("Font index is out of bound (0x%02X)\n", font_index);
        return 1;
      }
      font[font_index][row_index] = row_data;
      row_index++;
    }
  }
  printf("#include <stdint.h>\n");
  printf("extern uint8_t font[0x100][16];\n");
  printf("uint8_t font[0x100][16] = {\n");
  for (int i = 0; i < 0x100; i++) {
    printf("  {");
    for (int x = 0; x < 16; x++) {
      printf("0x%02X, ", font[i][x]);
    }
    printf("},\n");
  }
  printf("};\n");
  return 0;
}
