/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2002 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+

  $Id$ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <string.h>

#include <stdlib.h>
#include <stdio.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_libsms.h"



#include "sms.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif



/* If you declare any globals in php_libsms.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(libsms)
*/

/* True global resources - no need for thread safety here */
static int le_libsms;

/* {{{ libsms_functions[]
 *
 * Every user visible function must have an entry in libsms_functions[].
 */
function_entry libsms_functions[] = {
	PHP_FE(confirm_libsms_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(sendsms,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in libsms_functions[] */
};
/* }}} */

/* {{{ libsms_module_entry
 */
zend_module_entry libsms_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"libsms",
	libsms_functions,
	PHP_MINIT(libsms),
	PHP_MSHUTDOWN(libsms),
	NULL,/* Replace with NULL if there's nothing to do at request start */
	NULL,/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(libsms),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LIBSMS
ZEND_GET_MODULE(libsms)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("libsms.global_value",      "42", PHP_INI_ALL, OnUpdateInt, global_value, zend_libsms_globals, libsms_globals)
    STD_PHP_INI_ENTRY("libsms.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_libsms_globals, libsms_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_libsms_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_libsms_init_globals(zend_libsms_globals *libsms_globals)
{
	libsms_globals->global_value = 0;
	libsms_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(libsms)
{
	/* If you have INI entries, uncomment these lines 
	ZEND_INIT_MODULE_GLOBALS(libsms, php_libsms_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(libsms)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(libsms)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "libsms support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_libsms_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_libsms_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char string[256];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = sprintf(string, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "libsms", arg);
	RETURN_STRINGL(string, len, 1);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

/* {{{ proto long sendsms(string host, string port, string ID, string targetNo, string Content)
   Send SMS to Target Mobile Phone 
	Return Value:
	-1: port number error;
	-2: host addr error;
	-3: get socket error;
	-4: connect failed;
	-5: get tcp protocol error;

*/


PHP_FUNCTION(sendsms)
{
	char *targetNo = NULL;
	char *ID = NULL;
	char *Content = NULL;
	char *host = NULL;
	char *port = NULL; 
	int argc = ZEND_NUM_ARGS();
	int host_len;
	int port_len;
	int targetNo_len;
	int Content_len;
	int ID_len;
	struct hostent * phe;
	struct servent * pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	int s,type;
	int lenPack;
	char* buffer;
	PSMSChildProtocolPassword pw;
	PSMSChildProtocolSendMessage ps;


	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;

	if (zend_parse_parameters(argc TSRMLS_CC, "sssss", &host, &host_len, &port, &port_len, &ID, &ID_len, &targetNo, &targetNo_len, &Content, &Content_len) == FAILURE) 
		return;
	if (pse=getservbyname(port,"tcp")){
		sin.sin_port=pse->s_port;
	} else if ( (sin.sin_port=htons((unsigned short)atoi(port) ))==0)	{
		RETURN_LONG(-1);
	}

	if (phe = gethostbyname(host) )	{
		memcpy(&sin.sin_addr,phe->h_addr, phe->h_length);
	} else if (( sin.sin_addr.s_addr = inet_addr (host)) == INADDR_NONE)
	{
		RETURN_LONG(-2);
	}
	if ( ( ppe=getprotobyname("tcp")) == 0)
	{
		RETURN_LONG(-5);
	}
	type=SOCK_STREAM;
	s=socket(PF_INET, type,ppe->p_proto);
	if (s<0)
	{
		RETURN_LONG(-3);
	}
	if (connect(s,(struct sockaddr * )&sin,sizeof(sin))<0)
	{
		RETURN_LONG(-5);
	}

	lenPack=sizeof(SMSChildProtocolPassword);
	buffer=(char *)malloc(lenPack);
	pw=(PSMSChildProtocolPassword)buffer;
	memset(pw,0,lenPack);
	pw->head.msgTypeID=MSGTYPE_PWD;
	sms_longToByte(pw->head.SMSSerialNo,3);
	sms_longToByte(pw->head.msgLength,sizeof(SMSChildProtocolPassword)-sizeof(SMSChildProtocolHead));
	strncpy(pw->user,USER,CONNECTION_USER_LEN);
	strncpy(pw->password,PASSWORD,CONNECTION_PASSWORD_LEN);
	write(s,pw,lenPack);

	free(buffer);

	lenPack= sizeof(SMSChildProtocolSendMessage)+Content_len;
	buffer=(char *)malloc(lenPack);

	ps=(PSMSChildProtocolSendMessage)buffer;
	memset(ps,0,lenPack);
	ps->head.msgTypeID=MSGTYPE_SM;
	sms_longToByte(ps->head.SMSSerialNo,2);
	sms_longToByte(ps->head.msgLength,sizeof(SMSChildProtocolSendMessage)-sizeof(SMSChildProtocolHead));
	snprintf(ps->senderNo,MOBILENUMBERLENGTH,"11%s",ID);
	strncpy(ps->targetNo,targetNo,MOBILENUMBERLENGTH); 
	sms_longToByte(ps->smsBodyLength,Content_len);
	memcpy(ps->smsBody,Content,Content_len);

    write(s,ps,lenPack);
    free(buffer);
									
	close(s);
	RETURN_LONG(0);	
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
