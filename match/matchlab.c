#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int matchA(char *str)
{
	int length = strlen(str);
	if (length < 11 || length > 15)
	{
		return 0;
	}

	if (str[0] != 'd' || str[1] != 'd' || str[2] != 'd' || str[3] != 'd')
	{
		return 0;
	}

	if (str[4] != 'd' && str[4] != '_')
	{
		return 0;
	}

	if (str[4] == 'd')
	{
		if (str[5] != '_' || str[6] != '_')
		{
			return 0;
		}

		if (str[7] != 'u' || str[8] != 'u')
		{
			return 0;
		}

		if (str[9] != 'u' && str[9] != '_')
		{
			return 0;
		}

		if (str[9] == 'u')
		{
			if (str[10] != '_')
			{
				return 0;
			}

			if (length >= 12)
			{
				if (str[11] != '_')
				{
					return 0;
				}
			}

			if (length >= 13)
			{
				if (str[12] < 48 || str[12] > 57)
				{
					return 0;
				}
			}
			else
			{
				return 0;
			}

			if (length >= 14)
			{
				if (str[13] < 48 || str[13] > 57)
				{
					return 0;
				}
			}

			if (length == 15)
			{
				if (str[14] < 48 || str[14] > 57)
				{
					return 0;
				}
			}
		}

		if (str[9] == '_')
		{
			if (str[10] != '_')
			{
				return 0;
			}

			if (length >= 12)
			{
				if (str[11] < 48 || str[11] > 57)
				{
					return 0;
				}
			}
			else
			{
				return 0;
			}

			if (length >= 13)
			{
				if (str[12] < 48 || str[12] > 57)
				{
					return 0;
				}
			}

			if (length >= 14)
			{
				if (str[13] < 48 || str[13] > 57)
				{
					return 0;
				}
			}

			if (length == 15)
			{
				return 0;
			}
		}
	}

	if (str[4] == '_')
	{
		if (str[5] != '_')
		{
			return 0;
		}

		if (str[6] != 'u' || str[7] != 'u')
		{
			return 0;
		}

		if (str[8] != 'u' && str[8] != '_')
		{
			return 0;
		}

		if (str[8] == 'u')
		{
			if (str[9] != '_' || str[10] != '_')
			{
				return 0;
			}

			if (length >= 12)
			{
				if (str[11] < 48 || str[11] > 57)
				{
					return 0;
				}
			}
			else
			{
				return 0;
			}

			if (length >= 13)
			{
				if (str[12] < 48 || str[12] > 57)
				{
					return 0;
				}
			}

			if (length >= 14)
			{
				if (str[13] < 48 || str[13] > 57)
				{
					return 0;
				}
			}

			if (length == 15)
			{
				return 0;
			}
		}

		if (str[8] == '_')
		{
			if (str[9] != '_')
			{
				return 0;
			}

			if (str[10] < 48 || str[10] > 57)
			{
				return 0;
			}

			if (length >= 12)
			{
				if (str[11] < 48 || str[11] > 57)
				{
					return 0;
				}
			}

			if (length >= 13)
			{
				if (str[12] < 48 || str[12] > 57)
				{
					return 0;
				}
			}

			if (length >= 14)
			{
				return 0;
			}
		}
	}

	return 1;
}

int matchB(char *str)
{
	int length = strlen(str);
	int dCount = 0;
	int ACount = 0;
	int xCount = 0;
	int i = 0;

	if (length < 6)
	{
		return 0;
	}

	if (str[0] == 'd')
		dCount++;
	if (str[1] == 'd')
		dCount++;
	if (str[2] == 'd')
		dCount++;

	if (dCount > 3)
	{
		return 0;
	}

	if (str[dCount] != '=' || str[dCount + 1] != '=')
	{
		return 0;
	}

	for (i = dCount + 2; i < length; i++)
	{
		if (str[i] >= 65 && str[i] <= 90)
		{
			ACount++;
		}
		else
		{
			break;
		}
	}

	if (ACount % 2 == 0)
	{
		return 0;
	}

	for (i = dCount + 2 + ACount; i < length; i++)
	{
		if (str[i] == 'x')
		{
			xCount++;
		}
		else
		{
			break;
		}
	}

	if (xCount % 2 == 0)
	{
		return 0;
	}

	if (dCount + 2 + ACount + xCount > length - 1)
	{
		return 0;
	}

	if (str[dCount + 2 + ACount + xCount] != '_')
	{
		return 0;
	}

	int j = dCount + 2 + ACount + xCount + 1;

	for (i = dCount + 2; i < dCount + 2 + ACount; i += 2)
	{
		if (j > length - 1)
		{
			return 0;
		}
		if (str[i] != str[j])
		{
			return 0;
		}
		j++;
	}

	if (length >= j + 1)
	{
		if (str[j] < 48 || str[j] > 57)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}

	if (length >= j + 2)
	{
		if (str[j + 1] < 48 || str[j + 1] > 57)
		{
			return 0;
		}
	}

	if (length >= j + 3)
	{
		if (str[j + 2] < 48 || str[j + 2] > 57)
		{
			return 0;
		}
	}

	if (length >= j + 4)
	{
		return 0;
	}

	return 1;
}

int matchC(char *str)
{
	int length = strlen(str);
	int jCount = 0;
	int ACount = 0;
	int vCount = 0;
	int i = 0;

	if (length < 9)
	{
		return 0;
	}

	for (i = 0; i < length; i++)
	{
		if (str[i] == 'j')
		{
			jCount++;
		}
	}

	if (jCount % 2 == 0)
	{
		return 0;
	}

	if (jCount + 1 > length - 1)
	{
		return 0;
	}

	if (str[jCount] != '=' || str[jCount + 1] != '=')
	{
		return 0;
	}

	for (i = jCount + 2; i < length; i++)
	{
		if (str[i] >= 65 && str[i] <= 90)
		{
			ACount++;
		}
		else
		{
			break;
		}
	}

	if (ACount % 2 == 0)
	{
		return 0;
	}

	for (i = jCount + 2 + ACount; i < length; i++)
	{
		if (str[i] == 'v')
		{
			vCount++;
		}
		else
		{
			break;
		}
	}

	if (vCount % 2 == 0)
	{
		return 0;
	}

	if (jCount + 2 + ACount + vCount > length - 1)
	{
		return 0;
	}

	if (str[jCount + 2 + ACount + vCount] != '_')
	{
		return 0;
	}

	int j = jCount + 2 + ACount + vCount + 1;
	for (i = jCount + 2; i < jCount + 2 + ACount; ++i)
	{
		if (j > length - 1)
		{
			return 0;
		}
		if (str[i] != str[j])
		{
			return 0;
		}
		j++;
	}

	j = jCount + 2 + ACount + vCount + 1 + ACount;
	for (i = jCount + 2; i < jCount + 2 + ACount; ++i)
	{
		if (j > length - 1)
		{
			return 0;
		}
		if (str[i] != str[j])
		{
			return 0;
		}
		j++;
	}

	if (length >= j + 1)
	{
		if (str[j] < 48 || str[j] > 57)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}

	if (length >= j + 2)
	{
		if (str[j + 1] < 48 || str[j + 1] > 57)
		{
			return 0;
		}
	}

	if (length >= j + 3)
	{
		if (str[j + 2] < 48 || str[j + 2] > 57)
		{
			return 0;
		}
	}

	if (length >= j + 4)
	{
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	int i;
	int mode = 1;
	int tMode = 0;
	char **str = NULL;
	int strCount = 0;

	if (argc == 1)
	{
		return 0;
	}

	str = malloc((argc - 1) * sizeof(char *));

	for (i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		if (arg[0] == '-' && strlen(arg) == 2)
		{
			switch (arg[1])
			{
			case 'a':
				mode = 1;
				break;
			case 'b':
				mode = 2;
				break;
			case 'c':
				mode = 3;
				break;
			case 't':
				tMode = 1;
				break;
			default:
				break;
			}
		}
		else
		{
			str[strCount] = arg;
			strCount++;
		}
	}

	int j = 0;
	switch (mode)
	{
	case 1:
		for (j = 0; j < strCount; j++)
		{
			if (matchA(str[j]))
			{
				if (tMode)
				{
					char first = str[j][0];
					str[j]++;
					printf("%s%c\n", str[j], first);
				}
				else
				{
					printf("yes\n");
				}
			}
			else
			{
				if (!tMode)
				{
					printf("no\n");
				}
			}
		}
		break;
	case 2:
		for (j = 0; j < strCount; j++)
		{
			if (matchB(str[j]))
			{
				if (tMode)
				{
					int i = 0;
					for (i = 0; i < strlen(str[j]); i++)
					{
						if (str[j][i] == 'A')
						{
							printf("AC");
						}
						else
						{
							printf("%c", str[j][i]);
						}
					}
					printf("\n");
				}
				else
				{
					printf("yes\n");
				}
			}
			else
			{
				if (!tMode)
				{
					printf("no\n");
				}
			}
		}
		break;
	case 3:
		for (j = 0; j < strCount; j++)
		{
			if (matchC(str[j]))
			{
				if (tMode)
				{
					int i = 0;
					for (i = 0; i < strlen(str[j]); i++)
					{
						if (!(str[j][i] == 'E' || str[j][i] == 'G'))
						{
							printf("%c", str[j][i]);
						}
					}
					printf("\n");
				}
				else
				{
					printf("yes\n");
				}
			}
			else
			{
				if (!tMode)
				{
					printf("no\n");
				}
			}
		}
		break;
	default:
		break;
	}

	free(str);

	return 0;
}
