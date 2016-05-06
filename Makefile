$(shell mkdir -p .build)

CPPFLAGS=-I vendor/include -Ilib/compiler -Wall -Ilib/runtime -Ilib/support -std=gnu++14
TEST_CPPFLAGS=-Itest/runners

LDFLAGS=-Lvendor/osx \
				-lc++ -lsoundio -lncurses -luv \
				-framework Accelerate \
				-framework CoreFoundation \
				-framework CoreAudio \
				-framework AudioToolbox
CXX=clang++

SRCS        := $(shell find lib -name *.cpp)
OBJS        := $(SRCS:.cpp=.o)
TEST_SRCS   := $(shell find test -name main.cpp)
TESTS       := $(TEST_SRCS:/main.cpp=.testcase)


# Dependencies

depend: .depend

clean:
	rm -rf .depend
	rm -f lib/**/*.o
	rm -f test/**/*.testcase

.depend:
	$(CXX) $(CPPFLAGS) -MM $(SRCS) > .depend;


# Tests

test/%.testcase : test/%/main.cpp $(OBJS)
	$(CXX) $(CPPFLAGS) $(TEST_CPPFLAGS) $(LDFLAGS) -o $@ $(OBJS) $<

test/%.run : test/%.testcase
	$< test/$*/examples/*

test : $(TESTS:.testcase=.run)

###

.PHONY : test clean
.PRECIOUS : %.o

include ./.depend
