#include <sqlplus.hh>
#include "sms.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

int main() {
	try{
		Connection conn(DB_NAME,DB_HOST,DB_USER,DB_PASSWORD);
		Query query=conn.query();
		stringstream sql;
		sql<<"update SMSRegister_TB set todayMoney=0, RegisterDate=RegisterDate";
		query.exec(sql.str());
	} catch (BadQuery er) {
		cout<<"error: "<<er.error<<endl;
		return -1;
	}

	return 0;
}

