CXX = g++
CXXFLAGS = -Wall -std=c++11
TARGET = myshell
OBJS = command.o tokenizer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

command.o: command.cc command.h tokenizer.h
	$(CXX) $(CXXFLAGS) -c command.cc

tokenizer.o: tokenizer.cc tokenizer.h
	$(CXX) $(CXXFLAGS) -c tokenizer.cc

install-compiler:
	@echo "Installing dependencies..."
	@if [ -f install_compiler.sh ]; then \
		bash install_compiler.sh; \
	else \
		echo "ERROR: install_compiler.sh not found"; \
	fi

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean install-compiler
    