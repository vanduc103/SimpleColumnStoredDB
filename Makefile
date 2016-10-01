CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=c++11 -lstdc++ -lsqlparser -L/usr/local/lib/ -I/home/duclv/work/sql-parser/src/ -I/usr/local/boost_1_61_0

OBJS =		App.o Table.o Dictionary.o Column.o ColumnBase.o PackedArray.o

LIBS =		-L/usr/local/lib/ -lsqlparser

TARGET =	App

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
