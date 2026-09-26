#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	int size = strlen(str);
	char tmp;
	for (int i = 0; i < size / 2; i++) {
		tmp = str[i];
		str[i] = str[size - i - 1];
		str[size - i - 1] = tmp;
	}
}

