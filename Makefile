EXE_NAME = notGDC.exe
CC_ARGS = -std=gnu99 -Os -nostdlib -m32 -march=i386 -ffreestanding
LD_ARGS = -Wl,--nmagic,--script=dos.ld

all:
	#gcc ${CC_ARGS} -fverbose-asm -S main.c # dump assembly
	gcc ${CC_ARGS} ${LD_ARGS} -o ${EXE_NAME} main.c

run: all
	# '-exit' to quit after program ends
	dosbox ${EXE_NAME}
