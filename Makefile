CXX=g++
CXXFLAGS=-g -I/usr/include/cc++2 -I/usr/include/mysql -DDEBUG -Wno-deprecated
HEADERS=18dx.h  deliver.h smschildprotocol.h sms.h app.h  sms18dxprotocol.h \
	smsdaemon.h smsprotocol.h  testmsg.h childprotocol.h  smschildprivilegechecker.h \
	smsdiskstorage.h smsstorage.h smsbbschildprotocol.h smsbbsprotocoldefine.h smsbbschildprivilegechecker.h \
	smslogger.h smstcpstream.h smsfeecodegetter.h smsmysqlfeecodegetter.h \
	sms5168protocol.h md5.h
LIBS= -lccgnu2 -lpthread -ldl -lsqlplus

all: sms_p_18dx sms_p_5168 sms_deliver_c2p sms_c_aka sms_deliver_p2c sms_c_bbs sms_money_cleaner
sms_deliver_c2p: app.o sms_deliver_c2p.o
	$(CXX) -o $@ $^ $(LIBS)
sms_c_aka:sms_c_aka.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o 
	$(CXX) -o $@ $^ $(LIBS)
sms_deliver_p2c:  app.o sms_deliver_p2c.o
	$(CXX) -o $@ $^ $(LIBS)
sms_p_18dx: sms_p_18dx.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
sms_p_5168: sms_p_5168.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS) -lgwapi
sms_c_bbs:sms_c_bbs.o app.o smsdiskstorage.o smsstorage.o smsdaemon.o
	$(CXX) -o $@ $^ $(LIBS)
sms_money_cleaner: sms_money_cleaner.o
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp ${HEADERS}
	$(CXX) $(CXXFLAGS)  -c $< -o $@
.PHONY: clean
clean:
	rm -rf *.o sms_p_18dx sms_deliver_c2p sms_c_aka sms_deliver_p2c sms_c_bbs

