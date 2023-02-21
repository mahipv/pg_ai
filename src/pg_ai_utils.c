#include "pg_ai_utils.h"

int
escape_encode(const char* src, const size_t src_len, char *dst, const size_t max_dst_len)
{
    const char	*p = src;
    char		*q = dst;
	size_t		src_index = 0;
	size_t		dst_index = 0;
	int			byte_count;

    while (src_index++ < src_len)
	{
        if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~')
		{
            *q++ = *p;
			dst_index++;
        }
		else
		{
			byte_count = sprintf(q, "%%%02X", (unsigned char) *p);

			/* if dst buffer cannot handle the new length return error */
			if (dst_index + byte_count > max_dst_len)
				return 1;

            q += byte_count;
			dst_index += byte_count;
        }
        p++;
    }
    *q = '\0';
	return 0;
}
