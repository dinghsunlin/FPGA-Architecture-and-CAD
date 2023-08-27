CXX			= g++

CXXFLAGS	= -O3 -std=c++11

SRC			= 111062684_fpga_final.cpp

RM			= rm

EXE			= ./topart

VER			= ./verify

OUT			= ./output/*.txt

all :: opt
opt: $(SRC)
	make -s clean && $(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)
clean:
	$(RM) -rf $(EXE) $(OUT)
test: opt
	@read -p "Which testcase to run? (B1 ~ B8): " CASE; \
	echo "Running testcase $$CASE ..."; \
	$(EXE) ./input/$$CASE.txt ./output/$$CASE.txt; \
	echo "Running verification on $$CASE ..."; \
	$(VER) ./input/$$CASE.txt ./output/$$CASE.txt
out: opt
	echo "Running testcase B1 ..."; \
	$(EXE) ./input/B1.txt ./output/B1.txt; \
	echo "Running testcase B2 ..."; \
	$(EXE) ./input/B2.txt ./output/B2.txt; \
	echo "Running testcase B3 ..."; \
	$(EXE) ./input/B3.txt ./output/B3.txt; \
	echo "Running testcase B4 ..."; \
	$(EXE) ./input/B4.txt ./output/B4.txt; \
	echo "Running testcase B5 ..."; \
	$(EXE) ./input/B5.txt ./output/B5.txt; \
	echo "Running testcase B6 ..."; \
	$(EXE) ./input/B6.txt ./output/B6.txt; \
	echo "Running testcase B7 ..."; \
	$(EXE) ./input/B7.txt ./output/B7.txt; \
	echo "Running testcase B8 ..."; \
	$(EXE) ./input/B8.txt ./output/B8.txt; \
	echo "Running verification on B1 ..."; \
	$(VER) ./input/B1.txt ./output/B1.txt; \
	echo "Running verification on B2 ..."; \
	$(VER) ./input/B2.txt ./output/B2.txt; \
	echo "Running verification on B3 ..."; \
	$(VER) ./input/B3.txt ./output/B3.txt; \
	echo "Running verification on B4 ..."; \
	$(VER) ./input/B4.txt ./output/B4.txt; \
	echo "Running verification on B5 ..."; \
	$(VER) ./input/B5.txt ./output/B5.txt; \
	echo "Running verification on B6 ..."; \
	$(VER) ./input/B6.txt ./output/B6.txt; \
	echo "Running verification on B7 ..."; \
	$(VER) ./input/B7.txt ./output/B7.txt; \
	echo "Running verification on B8 ..."; \
	$(VER) ./input/B8.txt ./output/B8.txt; \