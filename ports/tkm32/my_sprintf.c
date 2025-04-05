
#include <stdarg.h>
#include <stdio.h>
//#include<ctype.h>


#include "my_sprintf.h"
#include "global.h"

#define my_isdigit(c) ((c) >= '0' && (c) <= '9')
//#define USHRT_MAX     ((uint16_t)(~0U))
//#define SHRT_MAX     ((signed short int)(USHRT_MAX>>1))
#define likely(x)  __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

/*功能：向字符串 格式化打印一个字符串
*参数：格式化的字符串
*注意：这个是简易版本 (%02x 完成)
* %-3s不行， %f也不行， %X不行
*/
int sprintf(char * str, const char *fmt, ...)
{
	int count = 0;
	char c;
	char *s;
	int n;
	
	int index = 0;
	int ret = 2;
	
	char buf[65];
	char digit[16];
	int num = 0;
	int len = 0;
	
	memset(buf, 0, sizeof(buf));
	memset(digit, 0, sizeof(digit));

	va_list ap;
	
	va_start(ap, fmt);
	
	while(*fmt != '\0')
	{
//		printf("*fmt=[%c]\n", *fmt);
		if(*fmt == '%')
		{
			fmt++;
			switch(*fmt)
			{
				case 'd': /*整型*/
				{
					n = va_arg(ap, int);
					if(n < 0)
					{
						*str = '-';
						str++;
						n = -n;
					}
					//printf("case d n=[%d]\n", n);
					//itoa(n, buf);
					itoa(n,buf,10);
					//printf("case d buf=[%s]\n", buf);
					memcpy(str, buf, strlen(buf));
					str += strlen(buf);
					break;
				}    
				case 'c': /*字符型*/
				{
					c = va_arg(ap, int);
					*str = c;
					str++;
					
					break;
				}
				case 'x': /*16进制*/
				{
					n = va_arg(ap, int);
					//xtoa(n, buf);
					itoa(n,buf,16);
					memcpy(str, buf, strlen(buf));
					str += strlen(buf);
					break;
				}
				case 's': /*字符串*/
				{
					s = va_arg(ap, char *);
					memcpy(str, s, strlen(s));
					str += strlen(s);
					break;
				}
				case '%': /*输出%*/
				{
					*str = '%';
					str++;
					
					break;
				}
				case '0': /*位不足的左补0*/
				{
					index = 0;
					num = 0;
					memset(digit, 0, sizeof(digit));
					
					while(1)
					{
						fmt++;
						ret = my_isdigit(*fmt);
						if(ret == 1) //是数字
						{
							digit[index] = *fmt;
							index++;
						}
						else
						{
							num = atoi(digit);
							break;
						}
					}
					switch(*fmt)
					 {
							case 'd': /*整型*/
							{
								n = va_arg(ap, int);
								if(n < 0)
								{
									*str = '-';
									str++;
									n = -n;
								}    
								//itoa(n, buf);
								itoa(n,buf,10);
								len = strlen(buf);
								if(len >= num)
								{
									memcpy(str, buf, strlen(buf));
									str += strlen(buf);
								}
								else
								{
									memset(str, '0', num-len);
									str += num-len;
									memcpy(str, buf, strlen(buf));
									str += strlen(buf);
								}
								break;
							}    
							case 'x': /*16进制*/
							{
								n = va_arg(ap, int);
								//xtoa(n, buf);
								itoa(n,buf,16);
								len = strlen(buf);
								if(len >= num)
								{
									memcpy(str, buf, len);
									str += len;
								}            
								else
								{
									memset(str, '0', num-len);
									str += num-len;
									memcpy(str, buf, len);
									str += len;
								}
								break;
							}
							case 's': /*字符串*/
							{
								s = va_arg(ap, char *);
								len = strlen(s);
								if(len >= num)
								{
									memcpy(str, s, strlen(s));
									str += strlen(s);
								}
								else
								{
									memset(str, '0', num-len);
									str += num-len;
									memcpy(str, s, strlen(s));
									str += strlen(s);
								}
								break;
							}
							default:
								break;
							}
				}
				default:
				break;
				}
		}
		else
		{
			*str = *fmt;
			str++;
			
			if(*fmt == '\n')
			{
							
			}
		}
		fmt++;
	}

	va_end(ap);

	return count;
}
//==================================================================

/* Convert a character to lower case */
static char my_tolower (char c)
{
  if ((c >= 'A') && (c <= 'Z'))
    {
      c = (c - 'A') + 'a';
    }
  return c;
}

//检查当前cp是否是个十六进制数值，不是直接返回0
int my_isxdigit( int ch)
{
	return (unsigned int)( ch-'0') < 10U || (unsigned int)((ch|0x20)-'a') < 6U;
}


int skip_atoi(const char **s)
{
	int i=0;
	while(my_isdigit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}
//检查所传的字符是否是空白字符。
int my_isspace( int ch )
{
    return (unsigned long)(ch - 9) < 5u || ' ' == ch;
}
const char *skip_spaces(const char *fmt)
{
	while (my_isspace(*fmt)) fmt++;
	return fmt;
}
static unsigned int simple_guess_base(const char *cp)
{
 if (cp[0] == '0') {
  if (my_tolower(cp[1]) == 'x' && my_isxdigit(cp[2]))
   return 16;
  else
   return 8;
 } else {
  return 10;
 }
}

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
 unsigned long long result = 0;

 if (!base)
  base = simple_guess_base(cp);

 if (base == 16 && cp[0] == '0' && my_tolower(cp[1]) == 'x')
  cp += 2;

 while (my_isxdigit(*cp)) {//检查当前cp是否是个十六进制数值，不是直接返回0
  unsigned int value;

  value = my_isdigit(*cp) ? *cp - '0' : my_tolower(*cp) - 'a' + 10;
  if (value >= base)
   break;
  result = result * base + value;
  cp++;
 }
 if (endp)
  *endp = (char *)cp;

 return result;
}

long simple_strtol(const char * restrict nptr, char ** restrict endptr, int base) 
{ 
    const char *s; 
    unsigned long acc; 
    char c; 
    unsigned long cutoff; 
    int neg, any, cutlim; 
        s = nptr; 
    do { 
        c = *s++; 
    } while (my_isspace((unsigned char)c)); 
    if (c == '-') { 
        neg = 1; 
        c = *s++;         //去掉前导空格和+ - 符号 
    } else { 
        neg = 0; 
        if (c == '+') 
            c = *s++; 
    } 
    //判断进制,并去除前导0x或者0 
    if ((base == 0 || base == 16) && 
        c == '0' && (*s == 'x' || *s == 'X') && 
        ((s[1] >= '0' && s[1] <= '9') || 
        (s[1] >= 'A' && s[1] <= 'F') || 
        (s[1] >= 'a' && s[1] <= 'f'))) { 
        c = s[1]; 
        s += 2; 
        base = 16; 
    } 
    if (base == 0) 
        base = c == '0' ? 8 : 10; 
    acc = any = 0; 
    if (base < 2 || base > 36) 
        goto noconv; 

    cutoff = neg ? (unsigned long)-(LONG_MIN + LONG_MAX) + LONG_MAX 
        : LONG_MAX; 
    cutlim = cutoff % base; 
    cutoff /= base; 
    for ( ; ; c = *s++) { 
        if (c >= '0' && c <= '9') 
            c -= '0'; 
        else if (c >= 'A' && c <= 'Z') 
            c -= 'A' - 10; 
        else if (c >= 'a' && c <= 'z') 
            c -= 'a' - 10; 
        else 
            break; 
        if (c >= base) 
            break;                       
        //如果溢出则设置any为负数
        
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) 
            any = -1; 
        else { 
            any = 1; 
            acc *= base; 
            acc += c; 
        } 
    } 
    if (any < 0) {         //如果溢出就返回最大能表示的值
        acc = neg ? LONG_MIN : LONG_MAX; 
    } else if (!any) { 
noconv: 
		;
    } else if (neg) 
        acc = -acc; 
    if (endptr != NULL)   
        *endptr = (char *)(any ? s - 1 : nptr); 
    return (acc); 
}
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
 unsigned long result = 0;

 if (!base)
  base = simple_guess_base(cp);

 if (base == 16 && cp[0] == '0' && my_tolower(cp[1]) == 'x')
  cp += 2;

 while (my_isxdigit(*cp)) {//检查当前cp是否是个十六进制数值，不是直接返回0
 unsigned int value;

  value = my_isdigit(*cp) ? *cp - '0' : my_tolower(*cp) - 'a' + 10;
  if (value >= base)
   break;
  result = result * base + value;
  cp++;
 }
 if (endp)
  *endp = (char *)cp;

 return result;
}


/**
* simple_strtoll - convert a string to a signed long long
* @cp: The start of the string
* @endp: A pointer to the end of the parsed string will be placed here
* @base: The number base to use
*/
long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
     if (*cp == '-')
          return -simple_strtoull(cp + 1, endp, base);
 
     return simple_strtoull(cp, endp, base);
}
 

/**
* vsscanf - Unformat a buffer into a list of arguments
* @buf:     input buffer
* @fmt:     format of buffer
* @args:     arguments
*/
int vsscanf(const char *buf, const char *fmt, va_list args)
{
	 const char *str = buf;
	 char *next;
	 char digit;
	 int num = 0;
	 uint8_t qualifier;
	 uint8_t base;
	 
	 signed short int field_width;
	 
	 char is_sign;

	 while (*fmt && *str) {
			/* skip any white space in format */
			/* white space in format matchs any amount of
			* white space, including none, in the input.
			*/
			//检查空白字符
			if (my_isspace(*fmt)) {
					 fmt = skip_spaces(++fmt);
					 str = skip_spaces(str);
			}

			/* anything that is not a conversion must match exactly */
			if (*fmt != '%' && *fmt) {
					 if (*fmt++ != *str++)
								break;
					 continue;
			}

			if (!*fmt)
					 break;
			++fmt;

			/* skip this conversion.
			* advance both strings to next white space
			*/
			if (*fmt == '*') {
				 while (!my_isspace(*fmt) && *fmt != '%' && *fmt)
							fmt++;
				 while (!my_isspace(*str) && *str)
							str++;
				 continue;
			}

			/* get field width */
			field_width = -1;
			if (my_isdigit(*fmt))
				field_width = skip_atoi(&fmt);

			/* get conversion qualifier */
			qualifier = -1;
			if (*fmt == 'h' || my_tolower(*fmt) == 'l' ||
					my_tolower(*fmt) == 'z') {
					 qualifier = *fmt++;
					 if (unlikely(qualifier == *fmt)) {
								if (qualifier == 'h') {
										 qualifier = 'H';
										 fmt++;
								} else if (qualifier == 'l') {
										 qualifier = 'L';
										 fmt++;
								}
					 }
			}

			if (!*fmt || !*str)
					 break;

			base = 10;
			is_sign = 0;

			switch (*fmt++) {
			case 'c':
			{
					 char *s = (char *)va_arg(args, char*);
					 if (field_width == -1)
								field_width = 1;
					 do {
								*s++ = *str++;
					 } while (--field_width > 0 && *str);
					 num++;
			}
			continue;
			case 's':
			{
					 char *s = (char *)va_arg(args, char *);
					 if (field_width == -1)
								field_width = SHRT_MAX;
					 /* first, skip leading white space in buffer */
					 str = skip_spaces(str);

					 /* now copy until next white space */
					 while (*str && !my_isspace(*str) && field_width--)
								*s++ = *str++;
					 *s = '\0';
					 num++;
			}
			continue;
			case 'n':
					 /* return number of characters read so far */
			{
					 int *i = (int *)va_arg(args, int*);
					 *i = str - buf;
			}
			continue;
			case 'o':
					 base = 8;
					 break;
			case 'x':
			case 'X':
					 base = 16;
					 break;
			case 'i':
					 base = 0;
			case 'd':
					 is_sign = 1;
			case 'u':
					 break;
			case '%':
					 /* looking for '%' in str */
					 if (*str++ != '%')
								return num;
					 continue;
			default:
					 /* invalid format; stop here */
					 return num;
			}

			/* have some sort of integer conversion.
			* first, skip white space in buffer.
			*/
			str = skip_spaces(str);

			digit = *str;
			if (is_sign && digit == '-')
					 digit = *(str + 1);

			if (!digit
					|| (base == 16 && !my_isxdigit(digit))
					|| (base == 10 && !my_isdigit(digit))
					|| (base == 8 && (!my_isdigit(digit) || digit > '7'))
					|| (base == 0 && !my_isdigit(digit)))
					 break;

			switch (qualifier) {
			case 'H':     /* that's 'hh' in format */
					 if (is_sign) {
								signed char *s = (signed char *)va_arg(args, signed char *);
								*s = (signed char)simple_strtol(str, &next, base);
					 } else {
								unsigned char *s = (unsigned char *)va_arg(args, unsigned char *);
								*s = (unsigned char)simple_strtoul(str, &next, base);
					 }
					 break;
			case 'h':
					 if (is_sign) {
								short *s = (short *)va_arg(args, short *);
								*s = (short)simple_strtol(str, &next, base);
					 } else {
								unsigned short *s = (unsigned short *)va_arg(args, unsigned short *);
								*s = (unsigned short)simple_strtoul(str, &next, base);
					 }
					 break;
			case 'l':
					 if (is_sign) {
								long *l = (long *)va_arg(args, long *);
								*l = simple_strtol(str, &next, base);
					 } else {
								unsigned long *l = (unsigned long *)va_arg(args, unsigned long *);
								*l = simple_strtoul(str, &next, base);
					 }
					 break;
			case 'L':
					 if (is_sign) {
								long long *l = (long long *)va_arg(args, long long *);
								*l = simple_strtoll(str, &next, base);
					 } else {
								unsigned long long *l = (unsigned long long *)va_arg(args, unsigned long long *);
								*l = simple_strtoull(str, &next, base);
					 }
					 break;
			case 'Z':
			case 'z':
			{
					 size_t *s = (size_t *)va_arg(args, size_t *);
					 *s = (size_t)simple_strtoul(str, &next, base);
			}
			break;
			default:
					 if (is_sign) {
								int *i = (int *)va_arg(args, int *);
								*i = (int)simple_strtol(str, &next, base);
					 } else {
								unsigned int *i = (unsigned int *)va_arg(args, unsigned int*);
								*i = (unsigned int)simple_strtoul(str, &next, base);
					 }
					 break;
			}
			num++;

			if (!next)
					 break;
			str = next;
	 }

	 /*
	 * Now we've come all the way through so either the input string or the
	 * format ended. In the former case, there can be a %n at the current
	 * position in the format that needs to be filled.
	 */
	 if (*fmt == '%' && *(fmt + 1) == 'n') {
				int *p = (int *)va_arg(args, int *);
				*p = str - buf;
	 }

	 return num;
}
/**
* sscanf - Unformat a buffer into a list of arguments
* @buf:     input buffer
* @fmt:     formatting of buffer
* @...:     resulting arguments
*/
int sscanf(const char *buf, const char *fmt, ...)
{
     va_list args;
     int i;
     va_start(args, fmt);
     i = vsscanf(buf, fmt, args);
     va_end(args);
 
     return i;
}




