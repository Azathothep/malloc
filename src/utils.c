#include <stdint.h>
#include <unistd.h>

#define UINT64_MAX_CHAR		20
#define UINT64_MAX_CHAR_HEX	16

size_t	ft_strlen(const char *s)
{
	int	i;

	i = 0;
	if (!s)
		return (0);
	while (s[i])
		i++;
	return (i);
}

void	write_uint64(int fd, uint64_t n) {
	char str[UINT64_MAX_CHAR];

	int i = UINT64_MAX_CHAR - 1;
	str[i] = '0';

	int d = 0;
	while (n > 0) {
		d = n;
		n = n / 10;	
		d -= (n * 10);
		str[i] = '0' + d;
		i--;
	}

	if (i < UINT64_MAX_CHAR - 1)
		i++;

	write(fd, &str[i], UINT64_MAX_CHAR - i);
}

void	write_uint64_hex(int fd, uint64_t n) {
	char hex[16] = "0123456789ABCDEF";	
	char str[UINT64_MAX_CHAR_HEX + 2];
	
  str[0] = '0';
  str[1] = 'x';

  int i = 2;
  while (i < UINT64_MAX_CHAR_HEX + 2) {
	  str[i] = '0';
    i++;
  }

	i = UINT64_MAX_CHAR_HEX + 2 - 1;
	int d = 0;
	while (n > 0) {
		d = n;
		n = n / 16;
		d -= (n * 16);
		str[i] = hex[d];
		i--;
	}

	write(fd, str, UINT64_MAX_CHAR_HEX + 2);
}
