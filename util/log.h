
/**
 * @file log.h
 *
 * @brief log handler
 */
#ifndef _LOG_H_INCLUDED
#define _LOG_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

/**
 * color outputs
 */
#define RED(x)			"\x1b[31m" \
						x \
						"\x1b[39m"
#define GREEN(x)		"\x1b[32m" \
						x \
						"\x1b[39m"
#define YELLOW(x)		"\x1b[33m" \
						x \
						"\x1b[39m"
#define BLUE(x)			"\x1b[34m" \
						x \
						"\x1b[39m"
#define MAGENTA(x)		"\x1b[35m" \
						x \
						"\x1b[39m"
#define CYAN(x)			"\x1b[36m" \
						x \
						"\x1b[39m"
#define WHITE(x)		"\x1b[37m" \
						x \
						"\x1b[39m"

/**
 * @macro DEBUG
 */
// #define DEBUG 			( 1 )

/**
 * @macro dbprintf
 */
#ifdef DEBUG

	#define dbprintf(fmt, ...) { \
		fprintf(stderr, fmt, __VA_ARGS__); \
	}

#else

	#define dbprintf(fmt, ...) {}

#endif

/**
 * @macro debug
 */
#ifdef DEBUG

	#define debug(...) { \
		debug_impl(__VA_ARGS__, ""); \
	}
	#define debug_impl(fmt, ...) { \
		dbprintf("[%s: %s(%d)] " fmt "%s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
	}

#else

	#define debug(...) {}

#endif

/**
 * @macro print_lane
 */
#ifdef DEBUG

	#define print_lane(p1, p2) { \
		cell_t *p = p1, *t = p2; \
		char *str = NULL; \
		int len = 0, size = 256; \
		str = malloc(size); \
		len += sprintf(str+len, "["); \
		while(p != t) { \
			if(*--t <= CELL_MIN) { \
				len += sprintf(str+len, "-oo,"); \
			} else if(*t >= CELL_MAX) { \
				len += sprintf(str+len, "oo,"); \
			} else { \
				len += sprintf(str+len, "%d,", *t); \
			} \
			if(len > (size - 20)) { \
				size *= 2; \
				str = realloc(str, size); \
			} \
		} \
		str[len == 1 ? 1 : len-1] = ']'; \
		str[len == 1 ? 2 : len] = '\0'; \
		debug("lane(%s)", str); \
		free(str); \
	}

#else

	#define print_lane(p1, p2) {}

#endif

/**
 * @macro logprintf
 */
#define logprintf(fmt, ...) { \
	fprintf(stderr, fmt, __VA_ARGS__); \
}

/**
 * @macro log
 */
#define log(...) { \
	log_impl(__VA_ARGS__, ""); \
}
#define log_impl(fmt, ...) { \
	logprintf("[%s] " fmt "%s\n", __func__, __VA_ARGS__); \
}
#define log_nr(...) { \
	log_nr_impl(__VA_ARGS__, ""); \
}
#define log_nr_impl(fmt, ...) { \
	logprintf("[%s] " fmt "%s", __func__, __VA_ARGS__); \
}

/**
 * @macro log_error
 */
#define log_error(...) { \
	log_error_impl(__VA_ARGS__, ""); \
}
#define log_error_impl(fmt, ...) { \
	logprintf("[%s]  ERROR: " fmt "%s\n", __func__, __VA_ARGS__); \
}

/**
 * @macro log_error_abort
 */
#define log_error_abort(...) { \
	log_error_impl(__VA_ARGS__, ""); \
	exit(1); \
}

/**
 * @macro msg
 */
#define msg(...) { \
	msg_impl(__VA_ARGS__, ""); \
}
#define msg_impl(fmt, ...) { \
	logprintf(fmt "%s\n", __VA_ARGS__); \
}

/**
 * @macro dump
 */
#if DEBUG

	#define dump(ptr, len) { \
		void *_p = (void *)(ptr); \
		int64_t _l = (int64_t)(len); \
		int64_t _i = 0; \
		/** header */ \
		fprintf(stderr, "                "); \
		for(_i = 0; _i < 16; _i++) { \
			fprintf(stderr, " %02x", (uint8_t)_i); \
		} \
		/** dump */ \
		_i = 0; \
		while(_i < _l) { \
			if((_i % 16) == 0) { \
				fprintf(stderr, "\n%016llx", (uint64_t)_p + _i); \
			} \
			fprintf(stderr, " %02x", ((uint8_t *)_p)[_i]); \
			_i++; \
		} \
		fprintf(stderr, "\n"); \
	}

#else

	#define dump(ptr, len) ;

#endif

#endif /* #ifndef _LOG_H_INCLUDED */
/**
 * end of log.h
 */
