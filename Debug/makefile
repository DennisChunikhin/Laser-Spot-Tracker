MAIN_DIR := ..

testGaussian: testGaussian.o $(MAIN_DIR)/processing.o
	gcc -o testGaussian $(MAIN_DIR)/processing.o testGaussian.o

testGaussian.o: testGaussian.c $(MAIN_DIR)/processing.h
	gcc -c testGaussian.c

$(MAIN_DIR)/processing.o: $(MAIN_DIR)/processing.c $(MAIN_DIR)/processing.h
	gcc -c -o $(MAIN_DIR)/processing.o $(MAIN_DIR)/processing.c