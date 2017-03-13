SBT := libsbt.so

$(SBT): libobk.c
	gcc -Wall -fPIC -c $<
	gcc -shared -lpthread -o $(SBT) libobk.o
	sbttest backup_file -libname $(PWD)/$(SBT)

clean:
	rm -f $(SBT) *.o
