g++ -o test18dx -g test18dx.cpp app.cpp smsdiskstorage.cpp smsdaemon.cpp smsstorage.cpp
g++ -o deliver -g app.cpp deliver.cpp
g++ -o testchild -g -I/usr/include/cc++2 testchild.cpp app.cpp smsdiskstorage.cpp smschilddaemon.cpp smsstorage.cpp -lccgnu2 -lpthread -ldl
g++ -o deliver2 -g app.cpp deliver2.cpp
g++ -o testrcv18dx -g  testrec18dx.cpp app.cpp smsdiskstorage.cpp smsdaemon.cpp smsstorage.cpp   