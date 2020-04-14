#include <stdio.h>

int main(void) {
  char string[] = "3 172 0 228 0 8483 leonardo 20 15 root 30";
  unsigned a, b, c, d, e, offset = 0, scanf_result = 1;
  long pid = 0;
  char user[10];
  char *data = string;

  sscanf(string, "%u %u %u %u %u ", &a, &b, &c, &d, &e);

  printf("a = %u, b = %u, c = %u, d = %u, e = %u\n", a,b,c,d,e);

  sscanf(string, "%*u %*u %*u %*u %*u%n", &offset);
  printf("offset : %d\n", offset);
  data += offset;

  do
  {
    // retorna o nmr de args lidos fora o %n que guarda o offset
    scanf_result = sscanf(data, " %lu %s %d%n", &pid, user, &a, &offset);
    printf("scan result = %d\n", scanf_result);
    data += offset;
    printf("%ld %s offset: %d\n", pid, user, offset);
  } while (scanf_result == 3);
  // enquanto scanf n chegar ao fim da string;
  return 0;
}
