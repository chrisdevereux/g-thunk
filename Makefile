$(shell mkdir -p .build)

CPPFLAGS=-Ilib/compiler -Wall -Ilib/support -std=gnu++14
LDFLAGS=-lc++
CXX=clang++

SRCS        := $(shell find lib -name *.cpp)
OBJS        := $(SRCS:.cpp=.o)


# Dependencies

depend: .depend

clean:
	rm -rf .depend
	rm -f lib/**/*.o

.depend:
	$(CXX) $(CPPFLAGS) -MM $(SRCS) > .depend;

###

.PHONY : clean
.PRECIOUS : %.o

include ./.depend
