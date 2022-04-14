all: kvmsample smallkern

kvmtest: kvmsample.o
	gcc kvmsample.c -o kvmsample -g

smallkern: smallkern.o
	ld -m elf_i386 -Ttext 0x0000 --oformat binary -o smallkern smallkern.o

smallkern.o: smallkern.s
	as --32 -o smallkern.o smallkern.s

clean:
	rm smallkern.o
	rm smallkern
	rm kvmsample
	rm kvmsample.o
