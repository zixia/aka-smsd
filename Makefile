CXX=g++
CXXFLAGS=-g -I/usr/include/cc++2 -I/usr/include/mysql -DDEBUG -Wno-deprecated
HEADERS=18dx.h  deliver.h smschildprotocol.h sms.h smstestchildprotocol.h app.h  sms18dxprotocol.h \
	smsdaemon.h smsprotocol.h  testmsg.h childprotocol.h  smschildprivilegechecker.h \
	smsdiskstorage.h smsstorage.h smsbbschildprotocol.h smsbbsprotocoldefine.h smsbbschildprivilegechecker.h \
	smslogger.h
LIBS= -lccgnu2 -lpthread -ldl -lsqlplus

all: test18dx deliver testchild deliver2 testrcv18dx testbbssmsd
deliver: app.o deliver.o
	$(CXX) -o $@ $^ $(LIBS)
testchild:testchild.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
deliver2:  app.o deliver2.o
	$(CXX) -o $@ $^ $(LIBS)
testrcv18dx: testrec18dx.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
test18dx: test18dx.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
testbbssmsd: testbbssmsd.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp ${HEADERS}
	$(CXX) $(CXXFLAGS)  -c $< -o $@
.PHONY: clean
clean:
	rm -rf *.o test18dx deliver testchild deliver2 testrcv18dx testbbssmsd

