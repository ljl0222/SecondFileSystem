src = $(wildcard *.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

CC = g++

target = SecondFS

$(target): $(obj)
	$(CC) -o $(target) $(obj)

%.o: %.cpp
	$(CC) -c $^

.PHONY: clean
clean:
	rm -f $(target) $(obj) *.img