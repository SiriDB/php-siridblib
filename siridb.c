#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "zend_exceptions.h"
#include "php-siridb.h"
#include <netdb.h> 
#include <libsiridb/siridb.h>
#include <sys/socket.h> 
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define SA struct sockaddr 

static zend_function_entry siridb_client_functions[] = {
    PHP_FE(siridb_connect, NULL)
    PHP_FE(siridb_close, NULL)
    PHP_FE(siridb_query, NULL)
    PHP_FE(siridb_insert, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry siridb_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SIRIDB_WORLD_EXTNAME,
    siridb_client_functions,
    PHP_MINIT(siridb),
    PHP_MSHUTDOWN(siridb),
    PHP_RINIT(siridb),
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SIRIDB_WORLD_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SIRIDB
ZEND_GET_MODULE(siridb)
#endif

PHP_RINIT_FUNCTION(siridb)
{
    return SUCCESS;
}

PHP_MINIT_FUNCTION(siridb)
{

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(siridb)
{
    // UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}

PHP_FUNCTION(siridb_connect)
{
    char *hostname;
    size_t hostname_len;
    char *username;
    size_t username_len;
    char *password;
    size_t password_len;
    char *dbname;
    size_t dbname_len;
    long port;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "slsss", &hostname, &hostname_len, &port, &username, &username_len, &password, &password_len, &dbname, &dbname_len) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(), "Incorrect method parameters for siridb_connect", 0);
        return;
    }
    
    int sockfd;
    ssize_t n; 
    struct sockaddr_in servaddr; 
  
    // socket create and varification 
    // SIRIDBSOCKFD_G(siridbsockfd) = socket(AF_INET, SOCK_STREAM, 0); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        zend_throw_exception(zend_exception_get_default(), "Failed setting up socket to SiriDB", 0);
        return;
    } 

    bzero((char *) &servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(estrndup(hostname, hostname_len)); 
    servaddr.sin_port = htons(port); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        zend_throw_exception(zend_exception_get_default(), "Failed connecting to SiriDB", 0);
        return;
    }

    const char * siridbusername = estrndup(username, username_len);
    const char * siridbpassword = estrndup(password, password_len);
    const char * siridbdbname = estrndup(dbname, dbname_len);

    siridb_pkg_t * auth_pkg = siridb_pkg_auth(2, siridbusername, siridbpassword, siridbdbname);
    n = write(sockfd, (char *) auth_pkg, sizeof(siridb_pkg_t) + auth_pkg->len); 

    char *buffer;
    buffer = emalloc(sizeof(siridb_pkg_t));

    if (n < 0) {
         zend_throw_exception(zend_exception_get_default(), "SiriDB authentication failed", 0);
         return;
    }

    n = read(sockfd,buffer,sizeof(siridb_pkg_t));

    if (n < 0) {
         zend_throw_exception(zend_exception_get_default(), "SiriDB authentication failed", 0);
         return;
    }


    siridb_pkg_t * auth_resp = (siridb_pkg_t *) buffer;

    if (auth_resp->tp != CprotoResAuthSuccess) {
        efree(buffer);
        zend_throw_exception(zend_exception_get_default(), "SiriDB authentication failed, please check used credentials", 0);
        return;
    } else {
        efree(buffer);
    }

    RETURN_LONG(sockfd);
}

ssize_t recv_all(int socket, char *buffer_ptr, size_t bytes_to_recv, int flag)
{
    size_t original_bytes_to_recv = bytes_to_recv;

    // Continue looping while there are still bytes to receive
    while (bytes_to_recv > 0)
    {
        // ssize_t ret = recv(socket, buffer_ptr, bytes_to_recv, flag);
        ssize_t ret = recv(socket, buffer_ptr, bytes_to_recv, flag);

        if (ret <= 0)
        {
            // Error or connection closed
            return ret;
        }

        // We have received ret bytes
        bytes_to_recv -= ret;  // Decrease size to receive for next iteration
        buffer_ptr += ret;     // Increase pointer to point to the next part of the buffer
    }

    return original_bytes_to_recv;  // Now all data have been received
}

bool check_connection(long sockfd) {
    int error = 0;
    socklen_t len = sizeof (error);
    int retval = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (retval != 0) {
        /* there was a problem getting the error code */
        return false;
    }

    if (error != 0) {
        /* socket has a non zero error status */
        return false;
    }

    return true;
}

zval * first_series_value(zval * ar) {
    zval * first_value;
    HashTable *arr_hash;
    HashTable *series_hash;
    zval **data;
    long num_key;
    zval * val;
    zend_string *key;
    zval * point_ts;
    zval * point_value;

    arr_hash = Z_ARRVAL_P(ar);

    // Loop through series
    val = zend_hash_index_find(arr_hash, 0);
    if(Z_TYPE_P(val) == IS_ARRAY) {
        printf("IS POINT\n");
        series_hash = Z_ARRVAL_P(val);
        point_ts = zend_hash_index_find(series_hash, 0);
        if (Z_TYPE_P(point_ts) == IS_LONG) {
            // ts value is valid
            point_value = zend_hash_index_find(series_hash, 1);
            return point_value;
        } else {
            zend_throw_exception(zend_exception_get_default(), "Incorrect value for series to be inserted", 0);
            return NULL;
        }
    } else {
        zend_throw_exception(zend_exception_get_default(), "Incorrect value for series to be inserted", 0);
        return NULL;
    }

    return NULL;
}

char * get_siridb_response_data(siridb_resp_t *resp, siridb_pkg_t *pkg) {
    char *json = "";
    switch (resp->tp)
    {
        case SIRIDB_RESP_TP_SELECT:
            json = qp_sprint(pkg->data, pkg->len);
            break;
        case SIRIDB_RESP_TP_LIST:
            json = qp_sprint(pkg->data, pkg->len);
            break;
        default:
            json = qp_sprint(pkg->data, pkg->len);
    }
    return json;
}

PHP_FUNCTION(siridb_close)
{
    long sockfd;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &sockfd) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(), "Incorrect method parameters for siridb_close", 0);
        return;
    }

    RETURN_BOOL(close(sockfd));
}

PHP_FUNCTION(siridb_query)
{
    long sockfd;
    char *query;
    size_t query_len;
    ssize_t n, m;
 

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &sockfd, &query, &query_len) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(), "Incorrect method parameters for siridb_query", 0);
        return;
    }

    if (!check_connection(sockfd)) {
        zend_throw_exception(zend_exception_get_default(), "Invalid SiriDB connection", 0);
        return;
    }

    siridb_pkg_t *  query_pkg = siridb_pkg_query(1, estrndup(query, query_len));
    n = write(sockfd, (char *) query_pkg, sizeof(siridb_pkg_t) + query_pkg->len); 

    char *buffer = emalloc(sizeof(siridb_pkg_t));
    
    if (n < 0)
         RETURN_BOOL(false);

    n = recv_all(sockfd, buffer, sizeof(siridb_pkg_t), MSG_PEEK);
    siridb_pkg_t * query_resp_pkg = (siridb_pkg_t *) buffer;

    if (n < 0) {
        efree(buffer);
        zend_throw_exception(zend_exception_get_default(), "Did not receive a complete response from SiriDB", 0);
        return;
    }

    char *fullbuffer = emalloc(sizeof(siridb_pkg_t) + query_resp_pkg->len);
    m = recv_all(sockfd, fullbuffer, sizeof(siridb_pkg_t) + query_resp_pkg->len, 0);

    if (m < 0) {
        efree(buffer);
        efree(fullbuffer);
        zend_throw_exception(zend_exception_get_default(), "Did not receive a complete response from SiriDB", 0);
        return;
    }

    if (m == sizeof(siridb_pkg_t) + query_resp_pkg->len) { 
        siridb_pkg_t * query_resp_pkg2 = (siridb_pkg_t *) fullbuffer;

        if (query_resp_pkg2->tp == CprotoErrQuery) {
            efree(buffer);
            efree(fullbuffer);
            RETURN_BOOL(false);
        } else if (query_resp_pkg2->tp == CprotoResQuery) {
            int rc;
            siridb_resp_t * resp = siridb_resp_create(query_resp_pkg2, &rc);
            if (rc) {
                efree(buffer);
                efree(fullbuffer);
                zend_throw_exception(zend_exception_get_default(), "Received corrupted data in response from SiriDB", 0);
                return;
            } else {
                char * json = get_siridb_response_data(resp, query_resp_pkg2);
                /* cleanup response */
                siridb_resp_destroy(resp);
                efree(buffer);
                efree(fullbuffer);
                RETURN_STRING(json);
            }
            
        }
    }
    efree(buffer);
    efree(fullbuffer);
    RETURN_BOOL(false);
}

PHP_FUNCTION(siridb_insert)
{

    long sockfd;
    HashTable *arr_hash;
    zval **data;
    long num_key;
    zval * val;
    zval * ar;
    zend_string *serie_name;
    zval * series_type;
    ssize_t n;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "la", &sockfd, &ar) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(), "Incorrect method parameters for siridb_insert", 0);
        return;
    }

    if( Z_TYPE_P(ar) != IS_ARRAY) {
        zend_throw_exception(zend_exception_get_default(), "Incorrect value for series to be inserted", 0);
        return;
    }

    printf("h0");

    arr_hash = Z_ARRVAL_P(ar);

    siridb_series_t * insert_series[zend_hash_num_elements(arr_hash)];

    printf("h1");
    
    int serie_num = 0;
    // Loop through series
    ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, serie_name, val) {
        //key == seriename
        if (serie_name) {
            printf("h2");

            if(Z_TYPE_P(val) == IS_ARRAY) {

                series_type = first_series_value(val);

                printf("h3");

                if (series_type == NULL)
                    return;

                printf("h4");

                HashTable *points_hash;
                points_hash = Z_ARRVAL_P(val);
                int count = zend_hash_num_elements(points_hash);

                printf("h5");
                

                siridb_series_tp tp;

                if (Z_TYPE_P(series_type) == IS_STRING) {
                    printf("Value is str!\n");
                    tp = SIRIDB_SERIES_TP_STR;
                } else if (Z_TYPE_P(series_type) == IS_DOUBLE) {
                    printf("Value is real!\n");
                    tp = SIRIDB_SERIES_TP_REAL;
                } else if (Z_TYPE_P(series_type) == IS_LONG) {
                    printf("Value is int!\n");
                    tp = SIRIDB_SERIES_TP_INT64;
                } else {
                    zend_throw_exception(zend_exception_get_default(), "Incorrect value for series to be inserted", 0);
                    return;
                }
                
                insert_series[serie_num] = siridb_series_create(
                tp,     /* type integer */
                ZSTR_VAL(serie_name),   /* name for the series */
                count);

                for (size_t i = 0; i < insert_series[serie_num]->n; i++)
                {
                    HashTable * pval;

                    siridb_point_t * point = insert_series[serie_num]->points + i;
                    pval = Z_ARRVAL_P(zend_hash_index_find(points_hash, i));

                    /* set time-stamps for the last n seconds */
                    point->ts = Z_LVAL_P(zend_hash_index_find(pval, 0));

                    if (tp == SIRIDB_SERIES_TP_STR) {
                        point->via.str = (char *) zend_hash_index_find(pval, 1);
                    } else if (tp == SIRIDB_SERIES_TP_REAL) {
                        point->via.real = (double) Z_DVAL_P(zend_hash_index_find(pval, 1));
                    } else if (tp == SIRIDB_SERIES_TP_INT64) {
                        point->via.int64 = (int64_t) zend_hash_index_find(pval, 1);
                    }
                }
                serie_num++;
                efree(points_hash);
            }
        }
    } ZEND_HASH_FOREACH_END();

    siridb_pkg_t *  series_pkg = siridb_pkg_series(1, insert_series, zend_hash_num_elements(arr_hash));
    n = write(sockfd, (char *) series_pkg, sizeof(siridb_pkg_t) + series_pkg->len); 

    char *buffer = emalloc(sizeof(siridb_pkg_t));
    
    if (n < 0)
         RETURN_BOOL(false);

    n = recv_all(sockfd, buffer, sizeof(siridb_pkg_t), MSG_PEEK);
    siridb_pkg_t * query_resp_pkg = (siridb_pkg_t *) buffer;

    if (n < 0) {
        efree(buffer);
        zend_throw_exception(zend_exception_get_default(), "Did not receive a complete response from SiriDB", 0);
        return;
    }

    if (query_resp_pkg->tp == CprotoResInsert) {
        RETURN_BOOL(true);
    }

    RETURN_BOOL(false);
}
