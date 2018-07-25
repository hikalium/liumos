#include <efi.h>
#include <common.h>

#define MAX_STR_BUF	100

void putc(unsigned short c)
{
	unsigned short str[2] = L" ";
	str[0] = c;
	ST->ConOut->OutputString(ST->ConOut, str);
}

void puts(unsigned short *s)
{
	ST->ConOut->OutputString(ST->ConOut, s);
}

void puth(unsigned long long val, unsigned char num_digits)
{
	int i;
	unsigned short unicode_val;
	unsigned short str[MAX_STR_BUF];

	for (i = num_digits - 1; i >= 0; i--) {
		unicode_val = (unsigned short)(val & 0x0f);
		if (unicode_val < 0xa)
			str[i] = L'0' + unicode_val;
		else
			str[i] = L'A' + (unicode_val - 0xa);
		val >>= 4;
	}
	str[num_digits] = L'\0';

	puts(str);
}

unsigned short getc(void)
{
	struct EFI_INPUT_KEY key;
	unsigned long long waitidx;

	ST->BootServices->WaitForEvent(1, &(ST->ConIn->WaitForKey),
				       &waitidx);
	while (ST->ConIn->ReadKeyStroke(ST->ConIn, &key));

	return (key.UnicodeChar) ? key.UnicodeChar
		: (key.ScanCode + SC_OFS);
}

unsigned int gets(unsigned short *buf, unsigned int buf_size)
{
	unsigned int i;

	for (i = 0; i < buf_size - 1;) {
		buf[i] = getc();
		putc(buf[i]);
		if (buf[i] == L'\r') {
			putc(L'\n');
			break;
		}
		i++;
	}
	buf[i] = L'\0';

	return i;
}

int strcmp(const unsigned short *s1, const unsigned short *s2)
{
	char is_equal = 1;

	for (; (*s1 != L'\0') && (*s2 != L'\0'); s1++, s2++) {
		if (*s1 != *s2) {
			is_equal = 0;
			break;
		}
	}

	if (is_equal) {
		if (*s1 != L'\0') {
			return 1;
		} else if (*s2 != L'\0') {
			return -1;
		} else {
			return 0;
		}
	} else {
		return (int)(*s1 - *s2);
	}
}

void strncpy(unsigned short *dst, unsigned short *src, unsigned long long n)
{
	while (n--)
		*dst++ = *src++;
}

unsigned long long strlen(unsigned short *str)
{
	unsigned long long len = 0;

	while (*str++ != L'\0')
		len++;

	return len;
}

int hexchartoint(char ch)
{
	if (('0' <= ch) && (ch <= '9'))
		return ch - '0';
	if (('A' <= ch) && (ch <= 'F'))
		return ch - 'A' + 0xA;
	if (('a' <= ch) && (ch <= 'f'))
		return ch - 'a' + 0xa;

	return -1;
}

unsigned long long hexstrtoull(char *str)
{
	unsigned long long res = 0;
	unsigned char digit_num;

	for (digit_num = 0; digit_num < 16; digit_num++) {
		int v = hexchartoint(*str++);
		if (v < 0)
			break;
		res = (res << 4) + v;
	}

	return res;
}

unsigned char check_warn_error(unsigned long long status, unsigned short *message)
{
	if (status) {
		puts(message);
		puts(L":");
		puth(status, 16);
		puts(L"\r\n");
	}

	return !status;
}

void assert(unsigned long long status, unsigned short *message)
{
	if (!check_warn_error(status, message))
		while (1);
}
