#ifndef PHP_SIRIDB_H
#define PHP_SIRIDB_H 1
#define PHP_SIRIDB_WORLD_VERSION "1.0"
#define PHP_SIRIDB_WORLD_EXTNAME "php-siridblib"


PHP_MINIT_FUNCTION(siridb);
PHP_MSHUTDOWN_FUNCTION(siridb);
PHP_RINIT_FUNCTION(siridb);

PHP_FUNCTION(siridb_connect);
PHP_FUNCTION(siridb_close);
PHP_FUNCTION(siridb_query);
PHP_FUNCTION(siridb_insert);

extern zend_module_entry siridb_module_entry;
#define phpext_siridb_ptr &siridb_module_entry

#endif