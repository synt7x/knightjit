
#include <stdio.h>
#include <stdlib.h>

#include "../luajit/dynasm/dasm_proto.h"
#include "../luajit/dynasm/dasm_x86.h"

#if _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

| .arch x64