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

#ifndef PHP_LIBSMS_H
#define PHP_LIBSMS_H

extern zend_module_entry libsms_module_entry;
#define phpext_libsms_ptr &libsms_module_entry

#ifdef PHP_WIN32
#define PHP_LIBSMS_API __declspec(dllexport)
#else
#define PHP_LIBSMS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(libsms);
PHP_MSHUTDOWN_FUNCTION(libsms);
PHP_MINFO_FUNCTION(libsms);

PHP_FUNCTION(confirm_libsms_compiled);	/* For testing, remove later. */
PHP_FUNCTION(sendsms);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(libsms)
	int   global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(libsms)
*/

/* In every utility function you add that needs to use variables 
   in php_libsms_globals, call TSRM_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMG_CC
   after the last function argument and declare your utility function
   with TSRMG_DC after the last declared argument.  Always refer to
   the globals in your function as LIBSMS_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define LIBSMS_G(v) TSRMG(libsms_globals_id, zend_libsms_globals *, v)
#else
#define LIBSMS_G(v) (libsms_globals.v)
#endif

#endif	/* PHP_LIBSMS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
