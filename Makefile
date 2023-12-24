CXX = g++
CXXFLAGS = -lcurl -lpthread

TARGET = multi_thread_download
DIRS = .
OBJS_FILES = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
HEADERS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.h))
OBJS = $(patsubst %.cpp, %.o, $(OBJS_FILES))
RM = rm -f

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

$(OBJS): %.o:%.cpp
	$(CXX) -g -c $< -o $@

clean:
	-$(RM) $(OBJS)

.PHONY: clean

