#define PARAMINFO_SWITCH_CASE(paramType, enumType, passType) \
		case enumType: \
			paramInfo[i].flags = info->params[i].flag; \
			paramInfo[i].size = sizeof(paramType); \
			paramInfo[i].type = passType; \
			size += sizeof(paramType); \
			break;

#define VSTK_PARAM_SWITCH(paramType, enumType) \
	case enumType: \
		*(paramType *)vptr = *(paramType *)(stack+offset); \
		if(i + 1 != info->paramcount) \
		{ \
			vptr += sizeof(paramType); \
		} \
		offset += sizeof(paramType); \
		break;
#ifndef __linux__

#define STACK_OFFSET 8
#define GET_STACK \
	unsigned long stack; \
	__asm \
	{ \
		mov stack, ebp \
	};
#else
#define STACK_OFFSET 4
#define GET_STACK \
	unsigned long stack = 0; \
	asm ("movl %%esp, %0;" : "=r" (stack));
#endif