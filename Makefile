CXX=g++
CXXFLAGS=-g -I/usr/include/cc++2 -I/usr/include/mysql -DDEBUG -Wno-deprecated
HEADERS=18dx.h  deliver.h smschildprotocol.h sms.h sms18dxrcvprotocol.h app.h  sms18dxprotocol.h \
	smsdaemon.h smsprotocol.h  testmsg.h childprotocol.h  smschildprivilegechecker.h \
	smsdiskstorage.h smsstorage.h smsbbschildprotocol.h smsbbsprotocoldefine.h smsbbschildprivilegechecker.h \
	smslogger.h smstcpstream.h smsfeecodegetter.h smsmysqlfeecodegetter.h
LIBS= -lccgnu2 -lpthread -ldl -lsqlplus

all: sms_p_18dx_sender sms_deliver_c2p sms_c_aka sms_deliver_p2c sms_p_18dx_receiver sms_c_bbs_9sharp sms_c_bbs_zixia sms_c_bbs_smth  sms_c_bbs_test sms_c_bbs_hightman sms_c_bbs_wangmm
sms_deliver_c2p: app.o sms_deliver_c2p.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_aka:sms_c_aka.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_deliver_p2c:  app.o sms_deliver_p2c.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_p_18dx_receiver: sms_p_18dx_receiver.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_p_18dx_sender: sms_p_18dx_sender.o app.o smsdiskstorage.o smsdaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_9sharp: sms_c_bbs_9sharp.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_zixia: sms_c_bbs_zixia.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_smth: sms_c_bbs_smth.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_test: sms_c_bbs_test.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_hightman: sms_c_bbs_hightman.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@
sms_c_bbs_wangmm: sms_c_bbs_wangmm.o app.o smsdiskstorage.o smschilddaemon.o smsstorage.o
	$(CXX) -o $@ $^ $(LIBS)
	killall $@
	./$@

%.o: %.cpp ${HEADERS}
	$(CXX) $(CXXFLAGS)  -c $< -o $@
.PHONY: clean
clean:
	rm -rf *.o sms_p_18dx_sender sms_deliver_c2p sms_c_aka sms_deliver_p2c sms_p_18dx_receiver sms_c_bbs_9sharp sms_c_bbs_zixia sms_c_bbs_smth sms_c_bbs_test sms_c_bbs_hightman sms_c_bbs_wangmm

