#include "PointerAnalysis/External/ExternalModTable.h"

using namespace llvm;

namespace tpa
{

ModEffect EffectTrait<ModEffect>::DefaultEffect = ModEffect::UnknownEffect;

void ExternalModTable::initializeTable()
{
	table = {
		{"_IO_getc", ModEffect::NoEffect},
		{"_IO_putc", ModEffect::NoEffect},
		{"_exit", ModEffect::NoEffect},
		{"_setjmp", ModEffect::NoEffect},
		{"__assert_fail", ModEffect::NoEffect},
		{"abs", ModEffect::NoEffect},
		{"access", ModEffect::NoEffect},

		{"execl", ModEffect::NoEffect},
		{"execlp", ModEffect::NoEffect},

		{"__isoc99_fscanf", ModEffect::ModAfterArg1},
		{"fscanf", ModEffect::ModAfterArg1},
		{"__isoc99_scanf", ModEffect::ModAfterArg0},
		{"scanf", ModEffect::ModAfterArg0},
		{"__isoc99_sscanf", ModEffect::ModAfterArg1},
		{"sscanf", ModEffect::ModAfterArg1},
		{"__isoc99_swscanf", ModEffect::ModAfterArg1},
		{"__ctype_b_loc", ModEffect::NoEffect},
		
		// More accurate: ModArg1Array and ModArg2
		{"accept", ModEffect::ModAfterArg0},

		
		{"alarm", ModEffect::NoEffect},
		{"atexit", ModEffect::NoEffect},
		{"atof", ModEffect::NoEffect},
		{"atoi", ModEffect::NoEffect},
		{"atol", ModEffect::NoEffect},
		{"btowc", ModEffect::NoEffect},

		{"fclose", ModEffect::NoEffect},
		{"feof", ModEffect::NoEffect},
		{"ferror", ModEffect::NoEffect},
		{"fflush", ModEffect::NoEffect},
		{"fgetc", ModEffect::NoEffect},
		{"fprintf", ModEffect::NoEffect},
		{"fwprintf", ModEffect::NoEffect},
		{"fputc", ModEffect::NoEffect},
		{"fputs", ModEffect::NoEffect},
		{"free", ModEffect::NoEffect},
		{"fseek", ModEffect::NoEffect},
		{"fseeko", ModEffect::NoEffect},
		{"fseeko64", ModEffect::NoEffect},
		
		{"fwrite", ModEffect::NoEffect},
		{"getchar", ModEffect::NoEffect},
		{"getegid", ModEffect::NoEffect},
		{"geteuid", ModEffect::NoEffect},
		{"getgid", ModEffect::NoEffect},
		{"getpgid", ModEffect::NoEffect},
		{"getpid", ModEffect::NoEffect},
		{"getppid", ModEffect::NoEffect},
		{"getsid", ModEffect::NoEffect},
		{"getuid", ModEffect::NoEffect},
		{"getresgid", ModEffect::NoEffect},
		{"getresuid", ModEffect::NoEffect},
		
		{"htonl", ModEffect::NoEffect},
		{"htons", ModEffect::NoEffect},
		
		{"isalnum", ModEffect::NoEffect},
		{"isalpha", ModEffect::NoEffect},
		{"isascii", ModEffect::NoEffect},
		{"isatty", ModEffect::NoEffect},
		{"isblank", ModEffect::NoEffect},
		{"iscntrl", ModEffect::NoEffect},
		{"isdigit", ModEffect::NoEffect},
		{"isgraph", ModEffect::NoEffect},
		{"islower", ModEffect::NoEffect},
		{"isprint", ModEffect::NoEffect},
		{"ispunct", ModEffect::NoEffect},
		{"isspace", ModEffect::NoEffect},
		{"isupper", ModEffect::NoEffect},
		{"iswalnum", ModEffect::NoEffect},
		{"iswalpha", ModEffect::NoEffect},
		{"iswctype", ModEffect::NoEffect},
		{"iswdigit", ModEffect::NoEffect},
		{"iswlower", ModEffect::NoEffect},
		{"iswspace", ModEffect::NoEffect},
		{"iswprint", ModEffect::NoEffect},
		{"iswupper", ModEffect::NoEffect},
		
		{"llvm.bswap.i16", ModEffect::NoEffect},
		{"llvm.bswap.i32", ModEffect::NoEffect},
		{"llvm.ctlz.i64", ModEffect::NoEffect},
		{"llvm.dbg.declare", ModEffect::NoEffect},
		{"llvm.dbg.value", ModEffect::NoEffect},
		{"llvm.lifetime.start", ModEffect::NoEffect},
		{"llvm.lifetime.end", ModEffect::NoEffect},
		{"llvm.stackrestore", ModEffect::NoEffect},
		{"llvm.trap", ModEffect::NoEffect},
		{"llvm.umul.with.overflow.i64", ModEffect::NoEffect},
		
		{"ntohl", ModEffect::NoEffect},
		{"ntohs", ModEffect::NoEffect},
		{"open", ModEffect::NoEffect},
		{"printf", ModEffect::NoEffect},
		{"wprintf", ModEffect::NoEffect},
		{"puts", ModEffect::NoEffect},
		{"srand", ModEffect::NoEffect},
		{"srandom", ModEffect::NoEffect},
		
		{"sprintf", ModEffect::ModArg0},
		{"snprintf", ModEffect::NoEffect},
		
		{"strcasecmp", ModEffect::NoEffect},
		{"strcmp", ModEffect::NoEffect},
		{"strlen", ModEffect::NoEffect},
		{"wcslen", ModEffect::NoEffect},
		{"strncasecmp", ModEffect::NoEffect},
		{"strncmp", ModEffect::NoEffect},
		{"syslog", ModEffect::NoEffect},
		{"tolower", ModEffect::NoEffect},
		{"toupper", ModEffect::NoEffect},
		{"towlower", ModEffect::NoEffect},
		{"towupper", ModEffect::NoEffect},
		{"vfprintf", ModEffect::NoEffect},
		{"vfwprintf", ModEffect::NoEffect},
		{"vprintf", ModEffect::NoEffect},
		{"vwprintf", ModEffect::NoEffect},
		{"vsnprintf", ModEffect::NoEffect},
		{"vsprintf", ModEffect::NoEffect},
		{"write", ModEffect::NoEffect},

		{"sin", ModEffect::NoEffect},
		{"cos", ModEffect::NoEffect},
		{"sinf", ModEffect::NoEffect},
		{"cosf", ModEffect::NoEffect},
		{"asin", ModEffect::NoEffect},
		{"acos", ModEffect::NoEffect},
		{"tan", ModEffect::NoEffect},
		{"atan", ModEffect::NoEffect},
		{"fabs", ModEffect::NoEffect},
		{"pow", ModEffect::NoEffect},
		{"floor", ModEffect::NoEffect},
		{"floorf", ModEffect::NoEffect},
		{"ceil", ModEffect::NoEffect},
		{"seed48", ModEffect::NoEffect},
		{"lrand48", ModEffect::NoEffect},
		{"abort", ModEffect::NoEffect},
		{"exit", ModEffect::NoEffect},
		{"log", ModEffect::NoEffect},
		{"log10", ModEffect::NoEffect},
		{"sqrt", ModEffect::NoEffect},
		{"sqrtf", ModEffect::NoEffect},
		{"exp", ModEffect::NoEffect},
		{"exp2", ModEffect::NoEffect},
		{"exp10", ModEffect::NoEffect},
		{"fabsf", ModEffect::NoEffect},
		{"hypot", ModEffect::NoEffect},
		{"rand", ModEffect::NoEffect},
		{"random", ModEffect::NoEffect},
		{"_ZdaPv", ModEffect::NoEffect},
		{"putchar", ModEffect::NoEffect},

		{"malloc", ModEffect::ModArg0},
		{"calloc", ModEffect::ModArg0},
		{"valloc", ModEffect::ModArg0},
		{"shmat", ModEffect::ModArg0},
		{"strdup", ModEffect::ModArg0},
		{"strndup", ModEffect::ModArg0},
		{"memalign", ModEffect::ModArg0},
		// These functions will implicitly call malloc
		{"ctime", ModEffect::ModArg0},
		{"fopen", ModEffect::ModArg0},
		{"getlogin", ModEffect::ModArg0},
		{"opendir", ModEffect::ModArg0},
		{"tempnam", ModEffect::ModArg0},
		{"textdomain", ModEffect::ModArg0},
		{"tgetstr", ModEffect::ModArg0},
		{"ttyname", ModEffect::ModArg0},
		{"tmpfile", ModEffect::ModArg0},
		{"strerror", ModEffect::ModArg0},
		// The mangled name of C++ new operators
		{"_Znwj", ModEffect::ModArg0},
		{"_ZnwjRKSt9nothrow_t", ModEffect::ModArg0},
		{"_Znwm", ModEffect::ModArg0},
		{"_ZnwmRKSt9nothrow_t", ModEffect::ModArg0},
		{"_Znaj", ModEffect::ModArg0},
		{"_ZnajRKSt9nothrow_t", ModEffect::ModArg0},
		{"_Znam", ModEffect::ModArg0},
		{"_ZnamRKSt9nothrow_t", ModEffect::ModArg0},

		{"getcwd", ModEffect::ModArg0},
		{"mmap", ModEffect::ModArg0},
		{"realloc", ModEffect::ModArg0},
		{"strtok", ModEffect::ModArg0},
		{"strtok_r", ModEffect::ModArg0},

		{"fgets", ModEffect::ModArg0},
		{"fgetws", ModEffect::ModArg0},
		{"gets", ModEffect::ModArg0},
		{"memchr", ModEffect::NoEffect},

		{"strcat", ModEffect::ModArg0},
		{"wcscat", ModEffect::ModArg0},
		{"strncat", ModEffect::ModArg0},
		{"wcsncat", ModEffect::ModArg0},
		{"strcpy", ModEffect::ModArg0},
		{"wcscpy", ModEffect::ModArg0},
		{"strncpy", ModEffect::ModArg0},
		{"strchr", ModEffect::NoEffect},
		{"wcschr", ModEffect::NoEffect},
		{"strrchr", ModEffect::NoEffect},
		{"strstr", ModEffect::NoEffect},
		{"strpbrk", ModEffect::NoEffect},
		{"strerror_r", ModEffect::ModArg1},

		{"strtod", ModEffect::ModArg1},
		{"strtof", ModEffect::ModArg1},
		{"strtol", ModEffect::ModArg1},
		{"strtold", ModEffect::ModArg1},
		{"strtoll", ModEffect::ModArg1},
		{"strtoul", ModEffect::ModArg1},
		
		{"freopen", ModEffect::NoEffect},
		

		{"bcopy", ModEffect::ModArg0Array},
		{"llvm.memcpy.i32", ModEffect::ModArg0Array},
		{"llvm.memcpy.p0i8.p0i8.i32", ModEffect::ModArg0Array},
		{"llvm.memcpy.i64", ModEffect::ModArg0Array},
		{"llvm.memcpy.p0i8.p0i8.i64", ModEffect::ModArg0Array},
		{"llvm.memmove.i32", ModEffect::ModArg0Array},
		{"llvm.memmove.p0i8.p0i8.i32", ModEffect::ModArg0Array},
		{"llvm.memmove.i64", ModEffect::ModArg0Array},
		{"llvm.memmove.p0i8.p0i8.i64", ModEffect::ModArg0Array},
		{"memccpy", ModEffect::ModArg0Array},
		{"memcpy", ModEffect::ModArg0Array},
		{"memmove", ModEffect::ModArg0Array},
		{"wmemcpy", ModEffect::ModArg0Array},

		{"memset", ModEffect::ModArg0Array},
		{"llvm.memset.p0i8.i64", ModEffect::ModArg0Array},
		{"llvm.memset.p0i8.i32", ModEffect::ModArg0Array},

		{"times", ModEffect::ModArg0Array},
		{"fread", ModEffect::ModArg0Array},
		{"qsort", ModEffect::ModArg0Array},

		{"socket", ModEffect::NoEffect},
		{"time", ModEffect::ModArg0Array},
		{"inet_addr", ModEffect::NoEffect},
		{"connect", ModEffect::NoEffect},
		{"recv", ModEffect::ModArg1},
		{"close", ModEffect::NoEffect},
		{"listen", ModEffect::NoEffect},
		{"bind", ModEffect::NoEffect},

		{"llvm.va_start", ModEffect::NoEffect},
		{"llvm.va_end", ModEffect::NoEffect},
		{"iswxdigit", ModEffect::NoEffect},

		{"getenv", ModEffect::NoEffect},
		{"popen", ModEffect::NoEffect},
		{"pclose", ModEffect::NoEffect},
		{"system", ModEffect::NoEffect},
	};
}

}